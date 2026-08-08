use crate::aggressive::{
    compound_assignments, eliminate_common_subexpressions, eliminate_dead_functions,
    eliminate_dead_locals, eliminate_dead_stores, factor_repeated_vector_args,
    fold_additive_constants, fold_additive_float_constants, fold_constants, fold_float_constants,
    fuse_statement_sequences, hoist_declarations, increment_decrement, merge_declarations, reduce_constant_vectors, shortest_scientific_form,
    simplify_algebraic_identities, strip_duplicate_precision, strip_redundant_braces,
    strip_redundant_parens, strip_trailing_void_return, ternary_from_if_else, AggressiveStats, Item,
};
use crate::macro_cse::eliminate_macro_common_subexpressions;
use crate::budget::estimate_budget;
use crate::lexer::{tokenize_spaced, Tok};
use crate::swizzle::{apply_swizzle_alphabet, SwizzleAlphabet};
use crate::vocab::{
    builtin_functions, builtin_variables, declaration_introducers, keywords, protected_host_names,
};
use std::collections::{HashMap, HashSet};

#[derive(Debug, Clone)]
pub struct GolfStats {
    pub input_chars: usize,
    pub output_chars: usize,
    pub reduction_pct: f64,
    pub renamed_count: usize,
    pub numbers_shortened: usize,
    pub aggressive: AggressiveStats,
}

#[derive(Debug, Clone)]
pub struct GolfResult {
    pub code: String,
    pub stats: GolfStats,
}

#[derive(Debug, Clone)]
pub struct PassTraceStep {
    pub pass_name: &'static str,
    pub before: String,
    pub after: String,
    pub count: usize,
}

#[derive(Debug, Clone, Default)]
pub struct GolferTrace {
    pub steps: Vec<PassTraceStep>,
}

impl GolferTrace {
    pub fn new() -> Self {
        Self::default()
    }
}

fn trace_before_snapshot(trace: &Option<&mut GolferTrace>, items: &[Item]) -> Option<String> {
    trace.as_ref().map(|_| layout(items))
}

fn trace_push_step(
    trace: &mut Option<&mut GolferTrace>,
    pass_name: &'static str,
    before: Option<String>,
    items: &[Item],
    count: usize,
) {
    if let (Some(trace), Some(before)) = (trace.as_deref_mut(), before) {
        trace.steps.push(PassTraceStep {
            pass_name,
            before,
            after: layout(items),
            count,
        });
    }
}

fn shorten_number(raw: &str) -> String {
    let mut mantissa = raw;
    let mut suffix = String::new();
    while let Some(last) = mantissa.chars().last() {
        if last == 'u' || last == 'U' || last == 'f' || last == 'F' {
            suffix.insert(0, last);
            mantissa = &mantissa[..mantissa.len() - 1];
        } else {
            break;
        }
    }
    let (mantissa, exponent) = match mantissa.find(['e', 'E']) {
        Some(idx) => (&mantissa[..idx], mantissa[idx..].to_string()),
        None => (mantissa, String::new()),
    };

    if mantissa.starts_with("0x") || mantissa.starts_with("0X") {
        return raw.to_string();
    }

    let mut result = mantissa.to_string();
    if let Some(dot) = result.find('.') {
        let (int_part, frac_part) = result.split_at(dot);
        let frac_part = &frac_part[1..];
        let trimmed_frac = frac_part.trim_end_matches('0');
        let int_part = if int_part == "0" { "" } else { int_part };
        if int_part.is_empty() && trimmed_frac.is_empty() {
            result = "0.".to_string();
        } else {
            result = format!("{int_part}.{trimmed_frac}");
        }
    }

    if exponent.is_empty() && mantissa.contains('.') {
        if let Ok(value) = mantissa.parse::<f32>() {
            if let Some(sci) = shortest_scientific_form(value) {
                if sci.len() < result.len() {
                    result = sci;
                }
            }
        }
    }

    format!("{result}{exponent}{suffix}")
}

struct NameGen {
    len: usize,
    counter: usize,
}
impl NameGen {
    fn new() -> Self {
        Self { len: 1, counter: 0 }
    }
}
impl Iterator for NameGen {
    type Item = String;
    fn next(&mut self) -> Option<String> {
        const ALPHABET: &[u8] = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
        let base = ALPHABET.len();
        let total_for_len: usize = (0..self.len).fold(1, |acc, _| acc * base);
        if self.counter >= total_for_len {
            self.len += 1;
            self.counter = 0;
        }
        let mut n = self.counter;
        let mut chars = Vec::with_capacity(self.len);
        for _ in 0..self.len {
            chars.push(ALPHABET[n % base] as char);
            n /= base;
        }
        chars.reverse();
        self.counter += 1;
        Some(chars.into_iter().collect())
    }
}

fn candidate_collides(
    candidate: &str,
    scope: &Scope,
    taken: &HashSet<String>,
    local_taken: &HashMap<usize, HashSet<String>>,
    block_scopes: &[BlockScope],
) -> bool {
    taken.contains(candidate)
        || match scope {
            Scope::Local(indices) => local_taken.iter().any(|(other_idx, names)| {
                names.contains(candidate)
                    && indices
                        .iter()
                        .any(|idx| !mutually_disjoint(&[*idx, *other_idx], block_scopes))
            }),
            Scope::Global => local_taken.values().any(|s| s.contains(candidate)),
        }
}

fn register_candidate(
    candidate: &str,
    original: &str,
    scope: &Scope,
    taken: &mut HashSet<String>,
    local_taken: &mut HashMap<usize, HashSet<String>>,
    rename_map: &mut HashMap<String, String>,
) {
    match scope {
        Scope::Global => {
            taken.insert(candidate.to_string());
        }
        Scope::Local(indices) => {
            for idx in indices {
                local_taken.entry(*idx).or_default().insert(candidate.to_string());
            }
        }
    }
    rename_map.insert(original.to_string(), candidate.to_string());
}

fn shortened_token_text(tok: &Tok, rename_map: &HashMap<String, String>, preceded_by_dot: bool) -> String {
    match tok {
        Tok::Ident(name) if preceded_by_dot => name.clone(),
        Tok::Ident(name) => rename_map.get(name).cloned().unwrap_or_else(|| name.clone()),
        Tok::Number(raw) => shorten_number(raw),
        Tok::Punct(c) => c.to_string(),
        Tok::Preproc(_) => String::new(),
    }
}

fn build_items(tokens: &[Tok], had_space: &[bool], rename_map: &HashMap<String, String>) -> (Vec<Item>, usize) {
    let mut numbers_shortened = 0usize;
    let mut items: Vec<Item> = Vec::with_capacity(tokens.len());
    for (idx, tok) in tokens.iter().enumerate() {
        let preceded_by_dot = idx > 0 && matches!(tokens[idx - 1], Tok::Punct('.'));
        let text = shortened_token_text(tok, rename_map, preceded_by_dot);
        if let Tok::Number(raw) = tok {
            if text != *raw {
                numbers_shortened += 1;
            }
        }
        items.push(Item {
            tok: tok.clone(),
            text,
            space_before: had_space[idx],
        });
    }
    (items, numbers_shortened)
}

fn render_code(tokens: &[Tok], had_space: &[bool], rename_map: &HashMap<String, String>) -> String {
    let (items, _) = build_items(tokens, had_space, rename_map);
    layout(&items)
}

fn collect_char_and_bigram_frequency(source: &str) -> (HashMap<char, usize>, HashMap<(char, char), usize>) {
    let mut chars = HashMap::new();
    let mut bigrams = HashMap::new();
    let mut previous = None;
    for ch in source.chars() {
        *chars.entry(ch).or_insert(0) += 1;
        if let Some(prev) = previous {
            *bigrams.entry((prev, ch)).or_insert(0) += 1;
        }
        previous = Some(ch);
    }
    (chars, bigrams)
}

fn candidate_frequency_score(
    candidate: &str,
    char_frequency: &HashMap<char, usize>,
    bigram_frequency: &HashMap<(char, char), usize>,
) -> usize {
    let chars: Vec<char> = candidate.chars().collect();
    let mut score = 0usize;
    for ch in &chars {
        score += char_frequency.get(ch).copied().unwrap_or(0);
    }
    for pair in chars.windows(2) {
        score += bigram_frequency.get(&(pair[0], pair[1])).copied().unwrap_or(0) * 2;
    }
    score
}

fn available_candidates(
    scope: &Scope,
    taken: &HashSet<String>,
    local_taken: &HashMap<usize, HashSet<String>>,
    block_scopes: &[BlockScope],
    max_len: usize,
) -> Vec<(usize, String)> {
    let mut gen = NameGen::new();
    let mut order = 0usize;
    let mut out = Vec::new();
    loop {
        let candidate = gen.next().unwrap();
        if candidate.len() > max_len {
            break;
        }
        if !candidate_collides(&candidate, scope, taken, local_taken, block_scopes) {
            out.push((order, candidate));
        }
        order += 1;
    }
    out
}

fn first_available_candidate(
    scope: &Scope,
    taken: &HashSet<String>,
    local_taken: &HashMap<usize, HashSet<String>>,
    block_scopes: &[BlockScope],
) -> (usize, String) {
    let mut gen = NameGen::new();
    let mut order = 0usize;
    loop {
        let candidate = gen.next().unwrap();
        if !candidate_collides(&candidate, scope, taken, local_taken, block_scopes) {
            return (order, candidate);
        }
        order += 1;
    }
}

// Finds a placeholder token guaranteed not to collide with anything already
// present in `code` -- used by choose_frequency_aware_candidate below so it
// can render the source exactly once per identifier decision instead of once
// per *candidate* (previously up to ~700 full renders per identifier; see
// that function's doc comment). All-uppercase and deliberately unlike any
// valid golfed GLSL identifier a NameGen-produced candidate could ever equal,
// so a `None` result here should be unreachable on real shader source; the
// caller falls back to the always-correct reference implementation rather
// than risk a wrong answer if it ever does happen.
fn unique_placeholder(code: &str) -> Option<String> {
    const CANDIDATES: &[&str] = &[
        "USHADERFREQPLACEHOLDER",
        "USHADERFREQPLACEHOLDERQ",
        "USHADERFREQPLACEHOLDERQQ",
        "USHADERFREQPLACEHOLDERQQQ",
        "USHADERFREQPLACEHOLDERQQQQ",
    ];
    CANDIDATES.iter().find(|p| !code.contains(*p)).map(|s| s.to_string())
}

fn choose_frequency_aware_candidate(
    original: &str,
    scope: &Scope,
    tokens: &[Tok],
    had_space: &[bool],
    rename_map: &HashMap<String, String>,
    taken: &HashSet<String>,
    local_taken: &HashMap<usize, HashSet<String>>,
    block_scopes: &[BlockScope],
    char_frequency: &HashMap<char, usize>,
    bigram_frequency: &HashMap<(char, char), usize>,
) -> String {
    let (naive_order, naive_candidate) = first_available_candidate(scope, taken, local_taken, block_scopes);
    let candidates = available_candidates(scope, taken, local_taken, block_scopes, 2);
    if candidates.is_empty() {
        return naive_candidate;
    }

    // Perf fix (previously documented, unfixed, known cost: O(identifiers x
    // candidates x file-size), up to ~1s on a 250-byte fixture, multiplied
    // further by every caller -- e.g. golf_harder -- that re-golfs the whole
    // file many times over). The original implementation called
    // render_code (a full token walk) and estimate_budget (a full DEFLATE
    // pass) once per candidate, for every renamable identifier. Instead,
    // render the source exactly once with a unique placeholder standing in
    // for `original`'s occurrences, then score every candidate by a plain
    // string substitution of that placeholder in the one rendered string.
    // This is safe because build_items/layout's spacing decisions are
    // token-kind-based (identifier vs number vs punctuation), never based on
    // a specific identifier's spelling or length, so substituting the
    // placeholder text after the fact reproduces byte-identical output to a
    // full re-render with that candidate -- verified, not just assumed, by
    // `frequency_aware_candidate_optimized_path_matches_the_reference_implementation_on_every_fixture`
    // below, which checks this function's choice against the original
    // full-render implementation (kept below as
    // `choose_frequency_aware_candidate_reference`) across every fixture in
    // the corpus for every AggressiveOptions combination Phase 30 already
    // exercises.
    let base_code = render_code(tokens, had_space, rename_map);
    let Some(placeholder) = unique_placeholder(&base_code) else {
        return choose_frequency_aware_candidate_reference(
            original, scope, tokens, had_space, rename_map, taken, local_taken, block_scopes,
            char_frequency, bigram_frequency,
        );
    };
    let mut placeholder_map = rename_map.clone();
    placeholder_map.insert(original.to_string(), placeholder.clone());
    let placeholder_code = render_code(tokens, had_space, &placeholder_map);

    let score_budget = |candidate: &str| -> usize {
        estimate_budget(&placeholder_code.replace(placeholder.as_str(), candidate)).deflate_bytes
    };

    let naive_budget = score_budget(&naive_candidate);
    let naive_score = candidate_frequency_score(&naive_candidate, char_frequency, bigram_frequency);

    let mut best_candidate = naive_candidate.clone();
    let mut best_budget = naive_budget;
    let mut best_score = naive_score;
    let mut best_order = naive_order;

    for (order, candidate) in candidates {
        let budget = score_budget(&candidate);
        let score = candidate_frequency_score(&candidate, char_frequency, bigram_frequency);
        let is_better = budget < best_budget
            || (budget == best_budget && score > best_score)
            || (budget == best_budget && score == best_score && order < best_order);
        if is_better {
            best_candidate = candidate;
            best_budget = budget;
            best_score = score;
            best_order = order;
        }
    }

    if best_budget == naive_budget && best_score == naive_score {
        naive_candidate
    } else {
        best_candidate
    }
}

// The original, always-correct-but-slow implementation. Kept for two
// purposes: (1) a runtime fallback for the practically-unreachable case
// where `unique_placeholder` cannot find a collision-free token, and (2) a
// reference the regression test suite checks the optimized path above
// against, so "faster" is never shipped without also being verified
// "identical".
fn choose_frequency_aware_candidate_reference(
    original: &str,
    scope: &Scope,
    tokens: &[Tok],
    had_space: &[bool],
    rename_map: &HashMap<String, String>,
    taken: &HashSet<String>,
    local_taken: &HashMap<usize, HashSet<String>>,
    block_scopes: &[BlockScope],
    char_frequency: &HashMap<char, usize>,
    bigram_frequency: &HashMap<(char, char), usize>,
) -> String {
    let (naive_order, naive_candidate) = first_available_candidate(scope, taken, local_taken, block_scopes);
    let candidates = available_candidates(scope, taken, local_taken, block_scopes, 2);
    if candidates.is_empty() {
        return naive_candidate;
    }

    let mut naive_map = rename_map.clone();
    naive_map.insert(original.to_string(), naive_candidate.clone());
    let naive_budget = estimate_budget(&render_code(tokens, had_space, &naive_map)).deflate_bytes;
    let naive_score = candidate_frequency_score(&naive_candidate, char_frequency, bigram_frequency);

    let mut best_candidate = naive_candidate.clone();
    let mut best_budget = naive_budget;
    let mut best_score = naive_score;
    let mut best_order = naive_order;

    for (order, candidate) in candidates {
        let mut trial_map = rename_map.clone();
        trial_map.insert(original.to_string(), candidate.clone());
        let budget = estimate_budget(&render_code(tokens, had_space, &trial_map)).deflate_bytes;
        let score = candidate_frequency_score(&candidate, char_frequency, bigram_frequency);
        let is_better = budget < best_budget
            || (budget == best_budget && score > best_score)
            || (budget == best_budget && score == best_score && order < best_order);
        if is_better {
            best_candidate = candidate;
            best_budget = budget;
            best_score = score;
            best_order = order;
        }
    }

    if best_budget == naive_budget && best_score == naive_score {
        naive_candidate
    } else {
        best_candidate
    }
}


fn assign_rename_map(
    renamable: &[(String, Scope)],
    aggressive: AggressiveOptions,
    tokens: &[Tok],
    had_space: &[bool],
    initial_taken: &HashSet<String>,
    block_scopes: &[BlockScope],
    char_frequency: &HashMap<char, usize>,
    bigram_frequency: &HashMap<(char, char), usize>,
) -> HashMap<String, String> {
    let mut taken = initial_taken.clone();
    let mut local_taken: HashMap<usize, HashSet<String>> = HashMap::new();
    let mut rename_map: HashMap<String, String> = HashMap::new();

    for (original, scope) in renamable {
        let candidate = if aggressive.frequency_aware_renaming {
            choose_frequency_aware_candidate(
                original,
                scope,
                tokens,
                had_space,
                &rename_map,
                &taken,
                &local_taken,
                block_scopes,
                char_frequency,
                bigram_frequency,
            )
        } else {
            first_available_candidate(scope, &taken, &local_taken, block_scopes).1
        };
        register_candidate(
            &candidate,
            original,
            scope,
            &mut taken,
            &mut local_taken,
            &mut rename_map,
        );
    }

    rename_map
}

fn struct_body_ranges(tokens: &[Tok]) -> Vec<(usize, usize)> {
    let mut ranges = Vec::new();
    let mut i = 0;
    while i < tokens.len() {
        let is_struct_kw = matches!(&tokens[i], Tok::Ident(s) if s == "struct");
        if is_struct_kw {
            let mut j = i + 1;
            while j < tokens.len() && !matches!(tokens[j], Tok::Punct('{') | Tok::Punct(';')) {
                j += 1;
            }
            if matches!(tokens.get(j), Some(Tok::Punct('{'))) {
                let mut depth = 0i32;
                let mut k = j;
                loop {
                    match tokens.get(k) {
                        Some(Tok::Punct('{')) => depth += 1,
                        Some(Tok::Punct('}')) => {
                            depth -= 1;
                            if depth == 0 {
                                ranges.push((j, k));
                                break;
                            }
                        }
                        None => break,
                        _ => {}
                    }
                    k += 1;
                }
                i = k;
                continue;
            }
        }
        i += 1;
    }
    ranges
}

fn strictly_inside_any(idx: usize, ranges: &[(usize, usize)]) -> bool {
    ranges.iter().any(|(open, close)| idx > *open && idx < *close)
}

fn top_level_brace_ranges(tokens: &[Tok]) -> Vec<(usize, usize)> {
    let mut ranges = Vec::new();
    let mut i = 0;
    while i < tokens.len() {
        if matches!(tokens[i], Tok::Punct('{')) {
            let mut depth = 0i32;
            let mut k = i;
            loop {
                match tokens.get(k) {
                    Some(Tok::Punct('{')) => depth += 1,
                    Some(Tok::Punct('}')) => {
                        depth -= 1;
                        if depth == 0 {
                            break;
                        }
                    }
                    None => break,
                    _ => {}
                }
                k += 1;
            }
            if k < tokens.len() {
                ranges.push((i, k));
                i = k + 1;
                continue;
            }
        }
        i += 1;
    }
    ranges
}

fn extend_left_to_params(tokens: &[Tok], body_open: usize) -> usize {
    if body_open == 0 || !matches!(tokens[body_open - 1], Tok::Punct(')')) {
        return body_open;
    }
    let mut depth = 0i32;
    let mut k = body_open - 1;
    loop {
        match &tokens[k] {
            Tok::Punct(')') => depth += 1,
            Tok::Punct('(') => {
                depth -= 1;
                if depth == 0 {
                    return k;
                }
            }
            _ => {}
        }
        if k == 0 {
            break;
        }
        k -= 1;
    }
    body_open
}

pub(crate) struct BlockScope {
    pub(crate) open: usize,
    pub(crate) close: usize,
}

fn matching_close_brace(tokens: &[Tok], open: usize) -> Option<usize> {
    if !matches!(tokens.get(open), Some(Tok::Punct('{'))) {
        return None;
    }
    let mut depth = 0i32;
    let mut k = open;
    loop {
        match tokens.get(k) {
            Some(Tok::Punct('{')) => depth += 1,
            Some(Tok::Punct('}')) => {
                depth -= 1;
                if depth == 0 {
                    return Some(k);
                }
            }
            None => return None,
            _ => {}
        }
        k += 1;
    }
}

pub(crate) fn block_scope_tree(tokens: &[Tok]) -> Vec<BlockScope> {
    let struct_bodies = struct_body_ranges(tokens);
    let mut scopes: Vec<BlockScope> = Vec::new();

    fn register(tokens: &[Tok], brace_open: usize, brace_close: usize, scopes: &mut Vec<BlockScope>) {
        let open = extend_left_to_params(tokens, brace_open);
        scopes.push(BlockScope {
            open,
            close: brace_close,
        });
        let mut i = brace_open + 1;
        while i < brace_close {
            if matches!(tokens[i], Tok::Punct('{')) {
                if let Some(inner_close) = matching_close_brace(tokens, i) {
                    register(tokens, i, inner_close, scopes);
                    i = inner_close + 1;
                    continue;
                }
            }
            i += 1;
        }
    }

    for (open, close) in top_level_brace_ranges(tokens) {
        if struct_bodies.iter().any(|(s, _)| *s == open) {
            continue;
        }
        register(tokens, open, close, &mut scopes);
    }
    scopes
}

pub(crate) fn innermost_scope(pos: usize, scopes: &[BlockScope]) -> Option<usize> {
    scopes
        .iter()
        .enumerate()
        .filter(|(_, s)| pos > s.open && pos < s.close)
        .max_by_key(|(_, s)| s.open)
        .map(|(idx, _)| idx)
}

pub(crate) fn mutually_disjoint(indices: &[usize], scopes: &[BlockScope]) -> bool {
    for i in 0..indices.len() {
        for j in (i + 1)..indices.len() {
            let a = &scopes[indices[i]];
            let b = &scopes[indices[j]];
            if !(a.close < b.open || b.close < a.open) {
                return false;
            }
        }
    }
    true
}

#[derive(Clone, PartialEq, Eq, Debug)]
enum Scope {
    Global,
    Local(Vec<usize>),
}

fn identifiers_in_text(text: &str) -> HashSet<String> {
    let mut out = HashSet::new();
    let chars: Vec<char> = text.chars().collect();
    let mut i = 0;
    while i < chars.len() {
        if chars[i].is_ascii_alphabetic() || chars[i] == '_' {
            let start = i;
            while i < chars.len() && (chars[i].is_ascii_alphanumeric() || chars[i] == '_') {
                i += 1;
            }
            out.insert(chars[start..i].iter().collect());
        } else {
            i += 1;
        }
    }
    out
}

fn preproc_referenced_names(tokens: &[Tok]) -> HashSet<String> {
    let mut out = HashSet::new();
    for tok in tokens {
        if let Tok::Preproc(line) = tok {
            out.extend(identifiers_in_text(line));
        }
    }
    out
}

fn find_renamable(tokens: &[Tok]) -> Vec<(String, Scope)> {
    let kw = keywords();
    let declaration_kw = declaration_introducers();
    let builtins = builtin_functions();
    let builtin_vars = builtin_variables();
    let protected = protected_host_names();
    let struct_bodies = struct_body_ranges(tokens);
    let preproc_names = preproc_referenced_names(tokens);
    let block_scopes = block_scope_tree(tokens);

    let mut freq: HashMap<String, usize> = HashMap::new();
    let mut first_seen: HashMap<String, usize> = HashMap::new();
    let mut scopes_seen: HashMap<String, HashSet<Option<usize>>> = HashMap::new();

    for (idx, tok) in tokens.iter().enumerate() {
        if let Tok::Ident(name) = tok {
            *freq.entry(name.clone()).or_insert(0) += 1;
            first_seen.entry(name.clone()).or_insert(idx);
        }
    }

    for i in 0..tokens.len().saturating_sub(1) {
        if let (Tok::Ident(a), Tok::Ident(b)) = (&tokens[i], &tokens[i + 1]) {
            let a_is_type = declaration_kw.contains(a.as_str());
            let b_is_user = !kw.contains(b.as_str())
                && !builtins.contains(b.as_str())
                && !builtin_vars.contains(b.as_str())
                && !protected.contains(b.as_str());
            if a_is_type
                && b_is_user
                && !strictly_inside_any(i + 1, &struct_bodies)
                && !preproc_names.contains(b.as_str())
            {
                let scope_idx = innermost_scope(i + 1, &block_scopes);
                scopes_seen.entry(b.clone()).or_default().insert(scope_idx);
            }
        }
    }

    let mut list: Vec<(String, Scope)> = scopes_seen
        .into_iter()
        .map(|(name, tags)| {
            let all_local = tags.iter().all(|t| t.is_some());
            let scope = if all_local {
                let mut indices: Vec<usize> = tags.into_iter().flatten().collect();
                indices.sort_unstable();
                if mutually_disjoint(&indices, &block_scopes) {
                    Scope::Local(indices)
                } else {
                    Scope::Global
                }
            } else {
                Scope::Global
            };
            (name, scope)
        })
        .collect();
    list.sort_by(|(a, _), (b, _)| {
        let fa = freq.get(a).copied().unwrap_or(0);
        let fb = freq.get(b).copied().unwrap_or(0);
        fb.cmp(&fa)
            .then_with(|| first_seen.get(a).cmp(&first_seen.get(b)))
    });
    list
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AggressiveOptions {
    pub eliminate_dead_locals: bool,
    pub eliminate_dead_stores: bool,
    pub fold_constants: bool,
    pub reduce_constant_vectors: bool,
    pub strip_trailing_void_return: bool,
    pub compound_assignments: bool,
    pub increment_decrement: bool,
    pub ternary_from_if_else: bool,
    pub merge_declarations: bool,
    pub strip_redundant_braces: bool,
    pub strip_redundant_parens: bool,
    pub strip_duplicate_precision: bool,
    pub eliminate_dead_functions: bool,
    pub inline_single_call_functions: bool,
    pub simplify_algebraic_identities: bool,
    pub eliminate_common_subexpressions: bool,
    pub fuse_statement_sequences: bool,
    /// `golf.md` Phase 30.1 -- `Shader Minifier`-style aggressive,
    /// multi-call-site, multi-statement function inlining
    /// (`inline::inline_aggressive`). Unlike `inline_single_call_functions`
    /// above, this pass measures raw-character AND DEFLATE-estimated size
    /// before committing and can legitimately regress size if mis-tuned on
    /// an unusual shader, so it stays an explicit opt-in even in the
    /// `Maximum` built-in profile, exactly per golf.md 30.1's own stated
    /// rationale -- never flipped on by `all()`/`Maximum`, only by a test
    /// or UI toggle that opts in explicitly.
    pub aggressive_inlining: bool,
    /// `golf.md` Phase 30.2 -- whole-shader cross-statement common
    /// subexpression elimination via `#define` macro extraction
    /// (`macro_cse::eliminate_macro_common_subexpressions`), distinct from
    /// `eliminate_common_subexpressions` above (which is restricted to a
    /// single straight-line declaration-initializer run).
    pub macro_cse: bool,
    /// Whether the active budget preset is compression-based (has a
    /// `deflate_limit`, e.g. `budget::presets`' "4KB intro"/"8KB intro"/
    /// "JS13K-style 13KB"/"64KB intro"), as opposed to a raw-character-
    /// limit preset (e.g. "Shadertoy"/"X/Twitter shader"/the Twigl
    /// presets) or no preset at all. Per `golf.md` Phase 30.1's own stated
    /// rule -- "raw and, when the budget preset is compression-based,
    /// DEFLATE-estimated" -- `macro_cse`'s raw-character gate always
    /// applies, but its additional whole-buffer DEFLATE gate
    /// (`macro_cse::eliminate_macro_common_subexpressions`) is only
    /// meaningful, and is therefore only applied, when this is `true`.
    /// Defaults to `false`: a caller targeting a raw-character-limit
    /// preset, or with no preset selected, should not have a genuine
    /// raw-metric win vetoed by a DEFLATE estimate that reflects a metric
    /// it never asked to be measured against.
    pub macro_cse_compression_budget: bool,
    /// `golf.md` Phase 30.4 -- declaration hoisting / merge-across-function
    /// (`aggressive::hoist_declarations`), extending `merge_declarations`
    /// to also relocate a later same-type declaration backward across a
    /// conservatively-proven-safe straight-line gap.
    pub hoist_declarations: bool,
    /// `golf.md` Phase 31.1 -- folds a standalone float counter's
    /// increment (at the very top or very bottom of a `for` loop body)
    /// into the `for(...)` header itself (`loop_golf::golf_loop_headers`).
    /// The first pass in this document to restructure loop semantics
    /// rather than purely rename/reorder/fold, so -- same stability
    /// precedent as `fuse_statement_sequences`/`macro_cse`/
    /// `hoist_declarations` above -- it stays off in the shared `all()`
    /// helper and is only turned on explicitly by the `Maximum` built-in
    /// profile or by a test opting in per-value.
    pub loop_header_golf: bool,
    /// `golf.md` Phase 31.2 -- `do`/`while` -> `for` and `while` -> `for`
    /// normalization (`loop_golf::golf_loop_forms`), fired only when the
    /// resulting form is strictly shorter under both the raw-character and
    /// DEFLATE-estimated size guards already established by 30.1/31.1 --
    /// loop-form choice is purely cosmetic and reversible (unlike every
    /// other pass in this document, it has no correctness precondition
    /// beyond "never fires unless it strictly shrinks the output"), so it
    /// is safe to default on wherever it is enabled at all; it is instead
    /// gated off in `all()` purely for the same output-stability precedent
    /// already documented next to `loop_header_golf` above.
    pub loop_form_golf: bool,
    pub frequency_aware_renaming: bool,
    pub factor_repeated_vector_args: bool,
    pub swizzle_alphabet: SwizzleAlphabet,
}

impl AggressiveOptions {
    pub fn all() -> Self {
        Self {
            eliminate_dead_locals: true,
            eliminate_dead_stores: true,
            fold_constants: true,
            reduce_constant_vectors: true,
            strip_trailing_void_return: true,
            compound_assignments: true,
            increment_decrement: true,
            ternary_from_if_else: true,
            merge_declarations: true,
            strip_redundant_braces: true,
            strip_redundant_parens: true,
            strip_duplicate_precision: true,
            eliminate_dead_functions: true,
            inline_single_call_functions: true,
            simplify_algebraic_identities: true,
            eliminate_common_subexpressions: true,
            // golf.md Phase 30.3 has shipped (fixture, regression tests,
            // and the checkbox are all in place), but this field stays off
            // in the shared `all()` helper -- the same precedent already
            // set for `frequency_aware_renaming` above -- because `all()`
            // backs `golf(source, true)`, which roughly a hundred existing
            // exact-string regression tests across this module call
            // directly; flipping the shared default here would silently
            // change dozens of their expected outputs. The pass is instead
            // turned on explicitly by the C++ "Maximum" built-in profile
            // (`ui/golf_profile.cpp`) and by any test that opts in per
            // `AggressiveOptions` value, exactly like 29.1.
            fuse_statement_sequences: false,
            // golf.md Phase 30.1: never flipped on by `all()` even though
            // it has shipped -- see the field doc comment above, this is a
            // deliberate opt-in-only pass, not a stability-only exclusion
            // like the two below.
            aggressive_inlining: false,
            // golf.md Phase 30.2/30.4: same stability precedent as
            // `fuse_statement_sequences` above -- both passes have shipped
            // (fixture, regression tests, and checkbox all in place), but
            // stay off in the shared `all()` helper so the existing
            // exact-string regression tests across this module keep their
            // expected output; turned on explicitly by the `Maximum`
            // built-in profile or by a test opting in per-value.
            macro_cse: false,
            macro_cse_compression_budget: false,
            hoist_declarations: false,
            loop_header_golf: false,
            loop_form_golf: false,
            frequency_aware_renaming: false,
            factor_repeated_vector_args: true,
            swizzle_alphabet: SwizzleAlphabet::Auto,
        }
    }

    pub fn none() -> Self {
        Self {
            eliminate_dead_locals: false,
            eliminate_dead_stores: false,
            fold_constants: false,
            reduce_constant_vectors: false,
            strip_trailing_void_return: false,
            compound_assignments: false,
            increment_decrement: false,
            ternary_from_if_else: false,
            merge_declarations: false,
            strip_redundant_braces: false,
            strip_redundant_parens: false,
            strip_duplicate_precision: false,
            eliminate_dead_functions: false,
            inline_single_call_functions: false,
            simplify_algebraic_identities: false,
            eliminate_common_subexpressions: false,
            fuse_statement_sequences: false,
            aggressive_inlining: false,
            macro_cse: false,
            macro_cse_compression_budget: false,
            hoist_declarations: false,
            loop_header_golf: false,
            loop_form_golf: false,
            frequency_aware_renaming: false,
            factor_repeated_vector_args: false,
            swizzle_alphabet: SwizzleAlphabet::Xyzw,
        }
    }
}

pub fn golf_with_options(source: &str, aggressive: AggressiveOptions) -> GolfResult {
    golf_with_protected_names(source, aggressive, &[])
}

pub fn golf_with_protected_names(
    source: &str,
    aggressive: AggressiveOptions,
    protected_names: &[String],
) -> GolfResult {
    golf_with_protected_names_impl(source, aggressive, protected_names, &mut None)
}

pub fn golf_with_protected_names_traced(
    source: &str,
    aggressive: AggressiveOptions,
    protected_names: &[String],
) -> (GolfResult, GolferTrace) {
    let mut trace = GolferTrace::new();
    let result = golf_with_protected_names_impl(source, aggressive, protected_names, &mut Some(&mut trace));
    (result, trace)
}

fn golf_with_protected_names_impl(
    source: &str,
    aggressive: AggressiveOptions,
    protected_names: &[String],
    trace: &mut Option<&mut GolferTrace>,
) -> GolfResult {
    let input_chars = source.chars().count();
    let spaced = tokenize_spaced(source);
    let tokens: Vec<Tok> = spaced.iter().map(|(t, _)| t.clone()).collect();
    let had_space: Vec<bool> = spaced.iter().map(|(_, s)| *s).collect();

    let kw = keywords();
    let builtins = builtin_functions();
    let builtin_vars = builtin_variables();
    let protected = protected_host_names();

    let protected_names_set: HashSet<&str> = protected_names.iter().map(|s| s.as_str()).collect();
    let renamable: Vec<(String, Scope)> = find_renamable(&tokens)
        .into_iter()
        .filter(|(name, _)| !protected_names_set.contains(name.as_str()))
        .collect();

    let mut taken: HashSet<String> = HashSet::new();
    taken.extend(kw.iter().map(|s| s.to_string()));
    taken.extend(builtins.iter().map(|s| s.to_string()));
    taken.extend(builtin_vars.iter().map(|s| s.to_string()));
    taken.extend(protected.iter().map(|s| s.to_string()));
    // Caller-supplied protected names must be reserved from reuse, not just
    // exempted from being renamed themselves -- `protected_names_set` above
    // only filters `renamable` (stops an identifier already spelled e.g. "r"
    // from being renamed to something else), which silently does NOT stop a
    // *different* identifier from being renamed *to* "r". Confirmed as a
    // real bug via a user-reported shader: the Twigl Export panel passes
    // r/m/t/f (and conditionally o/b) here specifically so the golfer never
    // reuses those letters for an unrelated local, but without this line
    // that guarantee didn't actually hold whenever the identifier assigned
    // that letter wasn't itself already using it verbatim in the source --
    // i.e. almost always. See roadmap.md for the full incident writeup.
    taken.extend(protected_names.iter().cloned());
    let renamable_set: HashSet<&str> = renamable.iter().map(|(name, _)| name.as_str()).collect();
    for tok in &tokens {
        if let Tok::Ident(name) = tok {
            if !renamable_set.contains(name.as_str()) {
                taken.insert(name.clone());
            }
        }
    }
    taken.extend(preproc_referenced_names(&tokens));

    let base_code = render_code(&tokens, &had_space, &HashMap::new());
    let (char_frequency, bigram_frequency) = collect_char_and_bigram_frequency(&base_code);

    let block_scopes = block_scope_tree(&tokens);
    let naive_rename_map = assign_rename_map(
        &renamable,
        AggressiveOptions {
            frequency_aware_renaming: false,
            ..aggressive
        },
        &tokens,
        &had_space,
        &taken,
        &block_scopes,
        &char_frequency,
        &bigram_frequency,
    );
    let rename_map = if aggressive.frequency_aware_renaming {
        let freq_rename_map = assign_rename_map(
            &renamable,
            aggressive,
            &tokens,
            &had_space,
            &taken,
            &block_scopes,
            &char_frequency,
            &bigram_frequency,
        );
        let (naive_items, _, _) = run_aggressive_pipeline(
            &tokens,
            &had_space,
            &naive_rename_map,
            aggressive,
            &mut None,
        );
        let (freq_items, _, _) = run_aggressive_pipeline(
            &tokens,
            &had_space,
            &freq_rename_map,
            aggressive,
            &mut None,
        );
        let naive_budget = estimate_budget(&layout(&naive_items)).deflate_bytes;
        let freq_budget = estimate_budget(&layout(&freq_items)).deflate_bytes;
        if freq_budget < naive_budget {
            freq_rename_map
        } else {
            naive_rename_map
        }
    } else {
        naive_rename_map
    };

    let (items, numbers_shortened, mut aggressive_stats) =
        run_aggressive_pipeline(&tokens, &had_space, &rename_map, aggressive, trace);

    // Runs once, after the fixpoint pipeline: unlike the passes above,
    // recoloring a swizzle's letter set never creates or removes an
    // opportunity for any other pass, so it does not need to be part of
    // the fixpoint loop (golf.md Phase 29.2).
    let struct_bodies = struct_body_ranges(&tokens);
    let snapshot = trace_before_snapshot(trace, &items);
    let count_before = aggressive_stats.swizzles_recolored;
    let items = apply_swizzle_alphabet(
        items,
        aggressive.swizzle_alphabet,
        &struct_bodies,
        &mut aggressive_stats.swizzles_recolored,
        layout,
        |code| estimate_budget(code).deflate_bytes,
    );
    trace_push_step(trace, "apply_swizzle_alphabet", snapshot, &items, aggressive_stats.swizzles_recolored - count_before);

    let code = layout(&items);

    let output_chars = code.chars().count();
    let reduction_pct = if input_chars == 0 {
        0.0
    } else {
        (input_chars as f64 - output_chars as f64) / input_chars as f64 * 100.0
    };

    GolfResult {
        code,
        stats: GolfStats {
            input_chars,
            output_chars,
            reduction_pct,
            renamed_count: rename_map.len(),
            numbers_shortened,
            aggressive: aggressive_stats,
        },
    }
}

fn run_aggressive_pipeline(
    tokens: &[Tok],
    had_space: &[bool],
    rename_map: &HashMap<String, String>,
    aggressive: AggressiveOptions,
    trace: &mut Option<&mut GolferTrace>,
) -> (Vec<Item>, usize, AggressiveStats) {
    let (mut items, numbers_shortened) = build_items(tokens, had_space, rename_map);
    const MAX_FIXPOINT_ITERATIONS: usize = 10;
    let mut aggressive_stats = AggressiveStats::default();
    for _ in 0..MAX_FIXPOINT_ITERATIONS {
        let before = items.clone();
        if aggressive.eliminate_dead_locals {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.dead_locals_removed;
            items = eliminate_dead_locals(items, &mut aggressive_stats);
            trace_push_step(trace, "eliminate_dead_locals", snapshot, &items, aggressive_stats.dead_locals_removed - count_before);
        }
        if aggressive.eliminate_dead_stores {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.dead_stores_removed;
            items = eliminate_dead_stores(items, &mut aggressive_stats);
            trace_push_step(trace, "eliminate_dead_stores", snapshot, &items, aggressive_stats.dead_stores_removed - count_before);
        }
        if aggressive.eliminate_dead_functions {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.dead_functions_removed;
            items = eliminate_dead_functions(items, &mut aggressive_stats);
            trace_push_step(trace, "eliminate_dead_functions", snapshot, &items, aggressive_stats.dead_functions_removed - count_before);
        }
        if aggressive.inline_single_call_functions {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.functions_inlined;
            items = crate::inline::inline_single_call_functions(items, &mut aggressive_stats);
            trace_push_step(trace, "inline_single_call_functions", snapshot, &items, aggressive_stats.functions_inlined - count_before);
        }
        if aggressive.aggressive_inlining {
            // golf.md Phase 30.1: multi-call-site, multi-statement
            // function inlining. Run after `inline_single_call_functions`
            // so the always-beneficial single-call-site path already had
            // first refusal at each candidate; this pass only considers
            // whatever functions remain declared.
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.functions_inlined;
            items = crate::inline::inline_aggressive(items, &mut aggressive_stats);
            trace_push_step(trace, "inline_aggressive", snapshot, &items, aggressive_stats.functions_inlined - count_before);
        }
        if aggressive.fold_constants {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.constants_folded;
            items = fold_constants(items, &mut aggressive_stats);
            items = fold_additive_constants(items, &mut aggressive_stats);
            items = fold_float_constants(items, &mut aggressive_stats);
            items = fold_additive_float_constants(items, &mut aggressive_stats);
            trace_push_step(trace, "fold_constants", snapshot, &items, aggressive_stats.constants_folded - count_before);
        }
        if aggressive.reduce_constant_vectors {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.constant_vectors_reduced;
            items = reduce_constant_vectors(items, &mut aggressive_stats);
            trace_push_step(trace, "reduce_constant_vectors", snapshot, &items, aggressive_stats.constant_vectors_reduced - count_before);
        }
        if aggressive.factor_repeated_vector_args {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.vector_args_factored;
            items = factor_repeated_vector_args(items, &mut aggressive_stats);
            trace_push_step(trace, "factor_repeated_vector_args", snapshot, &items, aggressive_stats.vector_args_factored - count_before);
        }
        if aggressive.simplify_algebraic_identities {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.algebraic_identities_simplified;
            items = simplify_algebraic_identities(items, &mut aggressive_stats);
            trace_push_step(trace, "simplify_algebraic_identities", snapshot, &items, aggressive_stats.algebraic_identities_simplified - count_before);
        }
        if aggressive.eliminate_common_subexpressions {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.common_subexpressions_eliminated;
            items = eliminate_common_subexpressions(items, &mut aggressive_stats);
            trace_push_step(trace, "eliminate_common_subexpressions", snapshot, &items, aggressive_stats.common_subexpressions_eliminated - count_before);
        }
        if aggressive.macro_cse {
            // golf.md Phase 30.2: whole-shader cross-statement CSE via
            // `#define` macro extraction. Deliberately placed after
            // renaming (Phase 29.1, already applied earlier in this
            // pipeline) so the macro body benefits from the shortest
            // already-assigned identifier names, and after the
            // straight-line-only `eliminate_common_subexpressions` above
            // so that pass gets first refusal at any run it already
            // handles -- this pass only looks at what is left.
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.common_subexpressions_eliminated;
            items = eliminate_macro_common_subexpressions(items, &mut aggressive_stats, aggressive.macro_cse_compression_budget);
            trace_push_step(trace, "macro_cse", snapshot, &items, aggressive_stats.common_subexpressions_eliminated - count_before);
        }
        if aggressive.loop_header_golf {
            // golf.md Phase 31.1: deliberately runs before
            // `compound_assignments`/`increment_decrement` below, since
            // those two passes are free to rewrite a body's `i+=1.`/
            // `i=i+1.` into a *prefix* `++i` (side-effect-only statements
            // never need to preserve the pre-increment value, so prefix
            // and postfix are equivalent there) -- which is a shape this
            // pass does not special-case, since `golf.md` only documents
            // the three counter-increment shapes recognized here as they
            // appear in *un-golfed* Shadertoy source. Running first keeps
            // the recognized shapes exactly as authored.
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.loop_headers_golfed;
            items = crate::loop_golf::golf_loop_headers(items, &mut aggressive_stats);
            trace_push_step(trace, "golf_loop_headers", snapshot, &items, aggressive_stats.loop_headers_golfed - count_before);
        }
        if aggressive.loop_form_golf {
            // golf.md Phase 31.2: `do`/`while` -> `for` and `while` -> `for`
            // normalization. Runs after `loop_header_golf` above so a `for`
            // loop that pass just produced is never re-examined here (it
            // is already a `for` loop), and before `compound_assignments`/
            // `increment_decrement` below for the same reason documented
            // on `loop_header_golf`: this pass matches the increment/
            // condition shapes as they appear in *un-golfed* source.
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.loop_forms_normalized;
            items = crate::loop_golf::golf_loop_forms(items, &mut aggressive_stats);
            trace_push_step(trace, "golf_loop_forms", snapshot, &items, aggressive_stats.loop_forms_normalized - count_before);
        }
        if aggressive.compound_assignments {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.compound_assignments;
            items = compound_assignments(items, &mut aggressive_stats);
            trace_push_step(trace, "compound_assignments", snapshot, &items, aggressive_stats.compound_assignments - count_before);
        }
        if aggressive.increment_decrement {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.increments_decrements;
            items = increment_decrement(items, &mut aggressive_stats);
            trace_push_step(trace, "increment_decrement", snapshot, &items, aggressive_stats.increments_decrements - count_before);
        }
        if aggressive.ternary_from_if_else {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.ternaries_from_if_else;
            items = ternary_from_if_else(items, &mut aggressive_stats);
            trace_push_step(trace, "ternary_from_if_else", snapshot, &items, aggressive_stats.ternaries_from_if_else - count_before);
        }
        if aggressive.merge_declarations {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.declarations_merged;
            items = merge_declarations(items, &mut aggressive_stats);
            trace_push_step(trace, "merge_declarations", snapshot, &items, aggressive_stats.declarations_merged - count_before);
        }
        if aggressive.hoist_declarations {
            // golf.md Phase 30.4: relocates a later same-type declaration
            // backward across a conservatively-proven-safe gap to merge
            // with an earlier one. Run immediately after the
            // adjacent-only `merge_declarations` above so a chain this
            // pass builds is itself merged into a single statement using
            // the exact same comma-declarator machinery.
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.declarations_merged;
            items = hoist_declarations(items, &mut aggressive_stats);
            trace_push_step(trace, "hoist_declarations", snapshot, &items, aggressive_stats.declarations_merged - count_before);
        }
        if aggressive.fuse_statement_sequences {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.statement_sequences_fused;
            items = fuse_statement_sequences(items, &mut aggressive_stats);
            trace_push_step(trace, "fuse_statement_sequences", snapshot, &items, aggressive_stats.statement_sequences_fused - count_before);
        }
        if aggressive.strip_redundant_braces {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.braces_removed;
            items = strip_redundant_braces(items, &mut aggressive_stats);
            trace_push_step(trace, "strip_redundant_braces", snapshot, &items, aggressive_stats.braces_removed - count_before);
        }
        if aggressive.strip_redundant_parens {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.redundant_parens_removed;
            items = strip_redundant_parens(items, &mut aggressive_stats);
            trace_push_step(trace, "strip_redundant_parens", snapshot, &items, aggressive_stats.redundant_parens_removed - count_before);
        }
        if aggressive.strip_duplicate_precision {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.duplicate_precision_removed;
            items = strip_duplicate_precision(items, &mut aggressive_stats);
            trace_push_step(trace, "strip_duplicate_precision", snapshot, &items, aggressive_stats.duplicate_precision_removed - count_before);
        }
        if aggressive.strip_trailing_void_return {
            let snapshot = trace_before_snapshot(trace, &items);
            let count_before = aggressive_stats.trailing_void_returns_removed;
            items = strip_trailing_void_return(items, &mut aggressive_stats);
            trace_push_step(trace, "strip_trailing_void_return", snapshot, &items, aggressive_stats.trailing_void_returns_removed - count_before);
        }
        if items == before {
            break;
        }
    }
    (items, numbers_shortened, aggressive_stats)
}

pub fn golf(source: &str, aggressive: bool) -> GolfResult {
    golf_with_options(
        source,
        if aggressive {
            AggressiveOptions::all()
        } else {
            AggressiveOptions::none()
        },
    )
}

fn is_word_like(t: &Tok) -> bool {
    matches!(t, Tok::Ident(_) | Tok::Number(_))
}

const AMBIGUOUS_PAIRS: &[&str] = &[
    "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "%=",
    "&=", "|=", "^=", "//", "/*",
];

fn forms_ambiguous_pair(prev_char: char, next_char: char) -> bool {
    let mut s = String::with_capacity(2);
    s.push(prev_char);
    s.push(next_char);
    AMBIGUOUS_PAIRS.contains(&s.as_str())
}

pub(crate) fn layout(items: &[Item]) -> String {
    let capacity: usize = items
        .iter()
        .map(|it| match &it.tok {
            Tok::Preproc(line) => line.len() + 2,
            _ => it.text.len() + 1,
        })
        .sum();
    let mut out = String::with_capacity(capacity);
    let mut prev_word_like = false;

    for (i, item) in items.iter().enumerate() {
        if let Tok::Preproc(line) = &item.tok {
            if !out.is_empty() && !out.ends_with('\n') {
                out.push('\n');
            }
            out.push_str(line);
            out.push('\n');
            prev_word_like = false;
            continue;
        }

        let cur_word_like = is_word_like(&item.tok);
        let mut need_space = prev_word_like && cur_word_like;

        if !need_space && i > 0 && !out.is_empty()
            && matches!(&items[i - 1].tok, Tok::Punct(_)) && matches!(&item.tok, Tok::Punct(_)) {
                let prev_char = out.chars().last().unwrap();
                let next_char = item.text.chars().next().unwrap_or(' ');
                if item.space_before && forms_ambiguous_pair(prev_char, next_char) {
                    need_space = true;
                }
            }

        if need_space {
            out.push(' ');
        }
        out.push_str(&item.text);
        prev_word_like = cur_word_like;
    }

    out
}

#[cfg(test)]
mod tests {
    use super::golf;
    use super::golf_with_options;
    use super::golf_with_protected_names;
    use super::golf_with_protected_names_traced;
    use super::AggressiveOptions;
    use super::{
        assign_rename_map, block_scope_tree, builtin_functions, builtin_variables,
        choose_frequency_aware_candidate_reference, collect_char_and_bigram_frequency,
        find_renamable, first_available_candidate, keywords, preproc_referenced_names,
        protected_host_names, register_candidate, render_code, tokenize_spaced, unique_placeholder,
        Scope, Tok,
    };
    use crate::budget::estimate_budget;
    use std::collections::{HashMap, HashSet};

    #[test]
    fn trace_pass_order_and_counts_match_fixture_regression() {
        // Regression guard for the fixpoint pass loop's fixed order: if a
        // future edit reorders, adds, or removes a pass inside the loop in
        // golf_with_protected_names_impl, this fixture's per-pass sequence
        // will drift and this test will fail, even though every individual
        // pass's own unit tests still pass in isolation.
        let source = include_str!("../../fixtures/golf_trace.glsl");
        let (result, trace) = golf_with_protected_names_traced(source, AggressiveOptions::all(), &[]);

        assert_eq!(
            result.code,
            "void mainImage(out vec4 b,in vec2 d){float c=2.,a=c;if(a>0.)--a;else++a;b=vec4(a);}"
        );

        // Exactly two fixpoint iterations: the first with real work spread
        // across several passes (dead-local removal, constant folding,
        // compound-assignment/increment-decrement rewriting, declaration
        // merging, brace stripping), the second a clean, all-zero pass
        // confirming the fixpoint, followed by the single, non-fixpoint
        // `apply_swizzle_alphabet` step (golf.md Phase 29.2) that always
        // runs exactly once after the loop closes. Seventeen passes per
        // iteration plus that one trailing step, in the exact order
        // golf_with_protected_names_impl invokes them.
        let expected: [(&str, usize); 35] = [
            ("eliminate_dead_locals", 1),
            ("eliminate_dead_stores", 0),
            ("eliminate_dead_functions", 0),
            ("inline_single_call_functions", 0),
            ("fold_constants", 1),
            ("reduce_constant_vectors", 0),
            ("factor_repeated_vector_args", 0),
            ("simplify_algebraic_identities", 0),
            ("eliminate_common_subexpressions", 0),
            ("compound_assignments", 2),
            ("increment_decrement", 2),
            ("ternary_from_if_else", 0),
            ("merge_declarations", 1),
            ("strip_redundant_braces", 2),
            ("strip_redundant_parens", 0),
            ("strip_duplicate_precision", 0),
            ("strip_trailing_void_return", 0),
            ("eliminate_dead_locals", 0),
            ("eliminate_dead_stores", 0),
            ("eliminate_dead_functions", 0),
            ("inline_single_call_functions", 0),
            ("fold_constants", 0),
            ("reduce_constant_vectors", 0),
            ("factor_repeated_vector_args", 0),
            ("simplify_algebraic_identities", 0),
            ("eliminate_common_subexpressions", 0),
            ("compound_assignments", 0),
            ("increment_decrement", 0),
            ("ternary_from_if_else", 0),
            ("merge_declarations", 0),
            ("strip_redundant_braces", 0),
            ("strip_redundant_parens", 0),
            ("strip_duplicate_precision", 0),
            ("strip_trailing_void_return", 0),
            ("apply_swizzle_alphabet", 0),
        ];

        assert_eq!(trace.steps.len(), expected.len());
        let actual: Vec<(&str, usize)> = trace.steps.iter().map(|s| (s.pass_name, s.count)).collect();
        assert_eq!(actual, expected);
    }

    #[test]
    fn trace_is_empty_when_every_pass_is_disabled() {
        let (result, trace) = golf_with_protected_names_traced(
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){float unused=1.0;fragColor=vec4(2.0);}",
            AggressiveOptions::none(),
            &[],
        );
        // `apply_swizzle_alphabet` (Phase 29.2) is the one exception: it is
        // not gated by an `AggressiveOptions` bool like every other pass,
        // it is always driven directly by the `swizzle_alphabet` field, so
        // it still records its single, zero-count, no-op step even when
        // every other pass is disabled.
        assert_eq!(trace.steps.len(), 1);
        assert_eq!(trace.steps[0].pass_name, "apply_swizzle_alphabet");
        assert_eq!(trace.steps[0].count, 0);
        assert_eq!(result.stats.aggressive.dead_locals_removed, 0);
    }

    #[test]
    fn trace_matches_the_untraced_entry_point_output_and_stats() {
        let source = "void mainImage(out vec4 fragColor,in vec2 fragCoord){float unused=1.0;fragColor=vec4(2.0);}";
        let mut opts = AggressiveOptions::none();
        opts.eliminate_dead_locals = true;
        let untraced = golf_with_protected_names(source, opts, &[]);
        let (traced, _) = golf_with_protected_names_traced(source, opts, &[]);
        assert_eq!(untraced.code, traced.code);
        assert_eq!(untraced.stats.aggressive.dead_locals_removed, traced.stats.aggressive.dead_locals_removed);
    }

    #[test]
    fn trace_records_one_step_per_fixpoint_iteration_for_the_one_enabled_pass() {
        let mut opts = AggressiveOptions::none();
        opts.eliminate_dead_locals = true;
        let (result, trace) = golf_with_protected_names_traced(
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){float unused=1.0;fragColor=vec4(2.0);}",
            opts,
            &[],
        );
        assert_eq!(result.stats.aggressive.dead_locals_removed, 1);
        // Every fixpoint-loop step is the one enabled pass; the single
        // trailing `apply_swizzle_alphabet` step (Phase 29.2, always runs
        // regardless of which `AggressiveOptions` bools are set) is the
        // sole, expected exception.
        assert!(trace
            .steps
            .iter()
            .all(|s| s.pass_name == "eliminate_dead_locals" || s.pass_name == "apply_swizzle_alphabet"));
        assert_eq!(trace.steps.iter().map(|s| s.count).sum::<usize>(), 1);
        let changed_steps: Vec<_> = trace.steps.iter().filter(|s| s.count > 0).collect();
        assert_eq!(changed_steps.len(), 1);
        assert_ne!(changed_steps[0].before, changed_steps[0].after);
        let unchanged_steps: Vec<_> = trace.steps.iter().filter(|s| s.count == 0).collect();
        for step in unchanged_steps {
            assert_eq!(step.before, step.after);
        }
    }

    #[test]
    fn trace_never_records_a_disabled_pass() {
        let mut opts = AggressiveOptions::none();
        opts.eliminate_dead_locals = true;
        let (_, trace) = golf_with_protected_names_traced(
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){float unused=1.0;fragColor=vec4(2.0);}",
            opts,
            &[],
        );
        assert!(!trace.steps.iter().any(|s| s.pass_name == "fold_constants"));
        assert!(!trace.steps.iter().any(|s| s.pass_name == "eliminate_dead_stores"));
    }

    #[test]
    fn safe_mode_unchanged_by_default() {
        let r = golf("void f(){float a=1.0;a=a-1.0;}", false);
        assert_eq!(r.code, "void b(){float a=1.;a=a-1.;}");
        assert_eq!(r.stats.aggressive.compound_assignments, 0);
        assert_eq!(r.stats.aggressive.declarations_merged, 0);
    }

    #[test]
    fn swizzle_after_dot_is_never_treated_as_a_variable_reference() {
        let r = golf("float h(float x){return x;}vec3 g(vec3 p){return vec3(p.x,p.y,p.z);}", false);
        assert!(r.code.contains(".x"), "swizzle .x must survive renaming: {}", r.code);
        assert!(r.code.contains(".y"), "swizzle .y must survive renaming: {}", r.code);
        assert!(r.code.contains(".z"), "swizzle .z must survive renaming: {}", r.code);
    }

    #[test]
    fn compound_assignment_single_term_rhs() {
        let r = golf("x=x-1.0;", true);
        assert_eq!(r.code, "--x;");
        assert_eq!(r.stats.aggressive.compound_assignments, 1);
        assert_eq!(r.stats.aggressive.increments_decrements, 1);
    }

    #[test]
    fn increment_decrement_rewrites_compound_assign_by_one() {
        let r = golf("x+=1.0;y-=1.0;", true);
        assert_eq!(r.code, "++x;--y;");
        assert_eq!(r.stats.aggressive.increments_decrements, 2);
    }

    #[test]
    fn increment_decrement_refuses_amounts_other_than_one() {
        let r = golf("x+=2.0;", true);
        assert_eq!(r.code, "x+=2.;");
        assert_eq!(r.stats.aggressive.increments_decrements, 0);
    }

    #[test]
    fn increment_decrement_uses_prefix_so_expression_value_stays_correct() {
        let r = golf("y=(x+=1.0);", true);
        assert_eq!(r.code, "y=++x;");
        assert_eq!(r.stats.aggressive.increments_decrements, 1);
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 1);
    }

    #[test]
    fn increment_decrement_never_collides_with_a_preceding_operator() {
        let r = golf("y=x+=1.0;", true);
        assert_eq!(r.code, "y=++x;");
        assert_eq!(r.stats.aggressive.increments_decrements, 1);
    }

    #[test]
    fn ternary_from_braced_if_else() {
        let r = golf("void f(){if(x>0.){a=1.;}else{a=-1.;}}", true);
        assert_eq!(r.code, "void b(){a=(x>0.)?1.:-1.;}");
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 1);
    }

    #[test]
    fn ternary_from_unbraced_if_else() {
        let r = golf("float f(float ready,float xv,float yv){float a=0.;if(ready>0.)a=xv;else a=yv;return a;}", true);
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 1);
        assert!(!r.code.contains("if("), "if/else should have been fully collapsed: {}", r.code);
        assert!(r.code.contains("?") && r.code.contains(":"), "expected a ternary: {}", r.code);
    }

    #[test]
    fn ternary_refuses_mismatched_targets() {
        let r = golf("void f(){if(c){a=1.;}else{b=2.;}}", true);
        assert!(r.code.contains("if("), "must not rewrite when the two branches assign different variables: {}", r.code);
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 0);
    }

    #[test]
    fn ternary_refuses_multi_term_rhs() {
        let r = golf("void f(){if(c){a=p+q;}else{a=r;}}", true);
        assert!(r.code.contains("if("), "must not rewrite a multi-term arm: {}", r.code);
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 0);
    }

    #[test]
    fn ternary_wraps_condition_containing_its_own_ternary() {
        let r = golf("void f(){if(c?d:e){a=1.;}else{a=2.;}}", true);
        assert_eq!(r.code, "void b(){a=(c?d:e)?1.:2.;}");
    }

    #[test]
    fn ternary_does_not_confuse_equality_with_assignment() {
        let r = golf("void f(){if(c){a==1.;}else{a==2.;}}", true);
        assert!(r.code.contains("if("), "must not treat == as an assignment: {}", r.code);
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 0);
    }

    // golf.md Phase 31.3 -- guard-clause ternary extension.

    #[test]
    fn ternary_guard_clause_braced_return_at_tail_of_function() {
        let r = golf_with_protected_names(
            "float f(float x){if(x>0.){return 1.;}return -1.;}",
            AggressiveOptions::all(),
            &["f".to_string(), "x".to_string()],
        );
        assert_eq!(r.code, "float f(float x){return(x>0.)?1.:-1.;}");
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 1);
    }

    #[test]
    fn ternary_guard_clause_unbraced_return_at_tail_of_function() {
        let r = golf_with_protected_names(
            "float f(float x){if(x>0.)return 1.;return -1.;}",
            AggressiveOptions::all(),
            &["f".to_string(), "x".to_string()],
        );
        assert_eq!(r.code, "float f(float x){return(x>0.)?1.:-1.;}");
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 1);
    }

    #[test]
    fn ternary_guard_clause_declines_when_an_else_is_present() {
        // `if(cond){return a;}else{return b;}` has no bare trailing
        // `return` statement at all (the second `return` is inside the
        // `else` branch), so it matches neither the assignment-pair form
        // above (which requires `ident=expr` in both arms, not `return`)
        // nor the new guard-clause form, whose defining precondition is
        // "no `else` present" per golf.md Phase 31.3 -- must be left
        // untouched.
        let source = "float f(float x){if(x>0.){return 1.;}else{return -1.;}}";
        let r = golf_with_protected_names(source, AggressiveOptions::all(), &["f".to_string(), "x".to_string()]);
        assert!(r.code.contains("if("), "an if/else pair of returns is out of scope for this pass: {}", r.code);
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 0);
    }

    #[test]
    fn ternary_guard_clause_declines_when_not_at_the_tail_of_the_function() {
        let source = "float f(float x){if(x>0.){return 1.;}return -1.;return -2.;}";
        let r = golf_with_protected_names(source, AggressiveOptions::all(), &["f".to_string(), "x".to_string()]);
        assert!(
            r.code.contains("if("),
            "must decline when the guard clause is not the tail of the function body: {}",
            r.code
        );
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 0);
    }

    #[test]
    fn ternary_guard_clause_declines_when_the_second_arm_is_not_a_bare_return() {
        let source = "float f(float x){if(x>0.){return 1.;}g=2.;}";
        let r = golf_with_protected_names(
            source,
            AggressiveOptions::all(),
            &["f".to_string(), "x".to_string(), "g".to_string()],
        );
        assert!(r.code.contains("if("), "must decline when the tail statement is not a return: {}", r.code);
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 0);
    }

    #[test]
    fn ternary_guard_clause_declines_when_the_arm_is_multi_term() {
        let source = "float f(float x,float p,float q){if(x>0.){return p+q;}return -1.;}";
        let r = golf_with_protected_names(
            source,
            AggressiveOptions::all(),
            &["f".to_string(), "x".to_string(), "p".to_string(), "q".to_string()],
        );
        assert!(r.code.contains("if("), "must not rewrite a multi-term arm: {}", r.code);
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 0);
    }

    #[test]
    fn ternary_guard_clause_uses_the_existing_toggle_with_no_new_flag() {
        let mut off = AggressiveOptions::all();
        off.ternary_from_if_else = false;
        let source = "float f(float x){if(x>0.){return 1.;}return -1.;}";
        let r = golf_with_protected_names(source, off, &["f".to_string(), "x".to_string()]);
        assert!(r.code.contains("if("), "no dedicated toggle exists, so ternary_from_if_else=false must also suppress the guard-clause form: {}", r.code);
        assert_eq!(r.stats.aggressive.ternaries_from_if_else, 0);
    }

    #[test]
    fn compound_assignment_refuses_unsafe_chain() {
        let r = golf("x=x-y-z;", true);
        assert_eq!(r.code, "x=x-y-z;");
        assert_eq!(r.stats.aggressive.compound_assignments, 0);
    }

    #[test]
    fn compound_assignment_refuses_self_initializing_declarator() {
        let r = golf("float a=a+1.0;", true);
        assert_eq!(r.code, "float a=a+1.;");
        assert_eq!(r.stats.aggressive.compound_assignments, 0);
    }

    #[test]
    fn compound_assignment_allows_parenthesised_single_term() {
        let r = golf("x=x/(y*z);", true);
        assert_eq!(r.code, "x/=(y*z);");
        assert_eq!(r.stats.aggressive.compound_assignments, 1);
    }

    #[test]
    fn merges_adjacent_same_type_declarations() {
        let r = golf("void f(){float a=1.0;float b=2.0;x=a+b;}", true);
        assert_eq!(r.code, "void c(){float a=1.,b=2.;x=a+b;}");
        assert_eq!(r.stats.aggressive.declarations_merged, 1);
        assert_eq!(r.stats.renamed_count, 3);
    }

    #[test]
    fn does_not_bridge_merge_across_unrelated_statement() {
        let r = golf("void f(){float a=1.0;x=2.0;float b=3.0;y=a+b;}", true);
        assert_eq!(r.code.matches("float").count(), 2);
        assert_eq!(r.stats.aggressive.declarations_merged, 0);
    }

    #[test]
    fn strips_braces_of_single_statement_if_body() {
        let r = golf("void f(){if(x){y=1.0;}}", true);
        assert_eq!(r.code, "void a(){if(x)y=1.;}");
        assert_eq!(r.stats.aggressive.braces_removed, 1);
    }

    #[test]
    fn refuses_to_strip_when_it_would_change_dangling_else_binding() {
        let r = golf("void h(){if(p){if(q)x;}else y;}", true);
        assert_eq!(r.code, "void a(){if(p){if(q)x;}else y;}");
        assert_eq!(r.stats.aggressive.braces_removed, 0);
    }

    #[test]
    fn refuses_to_strip_a_declaration_body() {
        let r = golf("void f(){if(x){float y=1.0;}z=y;}", true);
        assert_eq!(r.code, "void b(){if(x){float a=1.;}z=a;}");
        assert_eq!(r.stats.aggressive.braces_removed, 0);
    }

    #[test]
    fn strips_braces_of_single_statement_for_body() {
        let r = golf("void f(){for(int i=0;i<9;i++){x=1.0;}}", true);
        assert_eq!(r.code, "void b(){for(int a=0;a<9;a++)x=1.;}");
        assert_eq!(r.stats.aggressive.braces_removed, 1);
    }

    #[test]
    fn strips_braces_of_single_statement_do_while_body() {
        let r = golf("void f(){do{x=1.0;}while(x<9.0);}", true);
        assert_eq!(r.code, "void a(){do x=1.;while(x<9.);}");
        assert_eq!(r.stats.aggressive.braces_removed, 1);
    }

    #[test]
    fn keeps_multi_statement_block_but_recurses_into_it() {
        let r = golf("void f(){if(x){if(y){z=1.0;}w=2.0;}}", true);
        assert_eq!(r.code, "void a(){if(x){if(y)z=1.;w=2.;}}");
        assert_eq!(r.stats.aggressive.braces_removed, 1);
    }

    #[test]
    fn folds_plain_int_multiplication() {
        let r = golf("x=2*3;", true);
        assert_eq!(r.code, "x=6;");
        assert_eq!(r.stats.aggressive.constants_folded, 1);
    }

    #[test]
    fn folds_a_left_associative_chain_in_one_pass() {
        let r = golf("x=2*3*4;", true);
        assert_eq!(r.code, "x=24;");
        assert_eq!(r.stats.aggressive.constants_folded, 2);
    }

    #[test]
    fn folds_truncating_integer_division_and_modulo() {
        let r = golf("x=7/2;", true);
        assert_eq!(r.code, "x=3;");
        let r = golf("x=7%3;", true);
        assert_eq!(r.code, "x=1;");
    }

    #[test]
    fn folds_multiplicative_then_additive_across_the_fixpoint_loop() {
        let r = golf("x=2+3*4;", true);
        assert_eq!(r.code, "x=14;");
        assert_eq!(r.stats.aggressive.constants_folded, 2);
    }

    #[test]
    fn folds_a_simple_plus_and_minus() {
        let r = golf("x=1+2;", true);
        assert_eq!(r.code, "x=3;");
        let r = golf("x=3-5;", true);
        assert_eq!(r.code, "x=-2;");
    }

    #[test]
    fn folds_an_additive_chain_left_to_right() {
        let r = golf("x=1+2+3;", true);
        assert_eq!(r.code, "x=6;");
        let r = golf("x=3-5+10;", true);
        assert_eq!(r.code, "x=8;");
    }

    #[test]
    fn folds_a_leading_unary_sign_into_the_chain() {
        let r = golf("x=-5+3;", true);
        assert_eq!(r.code, "x=-2;");
    }

    #[test]
    fn refuses_to_fold_additive_chain_preceded_by_a_variable() {
        let r = golf("x=y-1+2;", true);
        assert_eq!(r.code, "x=y-1+2;");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn refuses_to_fold_additive_chain_preceded_by_a_closing_bracket() {
        let r = golf("x=f()-1+2;", true);
        assert_eq!(r.code, "x=f()-1+2;");
        let r = golf("x=a[0]-1+2;", true);
        assert_eq!(r.code, "x=a[0]-1+2;");
    }

    #[test]
    fn refuses_to_fold_across_a_following_tighter_operator_in_additive_chain() {
        let r = golf("x=1+2*3;", true);
        assert_eq!(r.code, "x=7;");
        let r = golf("x=1-2*3;", true);
        assert_eq!(r.code, "x=-5;");
    }

    #[test]
    fn refuses_to_fold_a_doubled_unary_sign() {
        let r = golf("x=- -3+2;", true);
        assert_eq!(r.code, "x=- -3+2;");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn refuses_to_fold_additive_overflow() {
        let r = golf("x=2147483647+1;", true);
        assert_eq!(r.code, "x=2147483647+1;");
        let r = golf("x=-2147483648-1;", true);
        assert_eq!(r.code, "x=-2147483648-1;");
    }

    #[test]
    fn additive_and_multiplicative_folding_compose_across_the_fixpoint_loop() {
        let r = golf("x=4*3+2;", true);
        assert_eq!(r.code, "x=14;");
        assert_eq!(r.stats.aggressive.constants_folded, 2);
    }

    #[test]
    fn refuses_to_fold_division_by_zero() {
        let r = golf("x=5/0;", true);
        assert_eq!(r.code, "x=5/0;");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn refuses_to_fold_on_i32_overflow() {
        let r = golf("x=2000000000*3;", true);
        assert_eq!(r.code, "x=2000000000*3;");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn refuses_to_fold_hex_literals() {
        let r = golf("x=0xFF*2;", true);
        assert_eq!(r.code, "x=0xFF*2;");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn folded_constant_then_feeds_compound_assignment() {
        let r = golf("x=x*2*3;", true);
        assert_eq!(r.code, "x*=6;");
        assert_eq!(r.stats.aggressive.constants_folded, 1);
        assert_eq!(r.stats.aggressive.compound_assignments, 1);
    }

    #[test]
    fn reduces_a_constant_vector_of_identical_literals() {
        let r = golf("void f(){vec3 a=vec3(1.0,1.0,1.0);}", true);
        assert_eq!(r.code, "void a(){vec3 b=vec3(1.);}");
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 1);
    }

    #[test]
    fn refuses_to_reduce_a_vector_of_differing_literals() {
        let r = golf("void f(){vec3 a=vec3(1.0,2.0,1.0);}", true);
        assert_eq!(r.code, "void a(){vec3 b=vec3(1.,2.,1.);}");
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 0);
    }

    #[test]
    fn refuses_to_reduce_a_vector_with_a_non_literal_argument() {
        // `reduce_constant_vectors` (literal-only) still declines this
        // input, but golf.md Phase 29.3's `factor_repeated_vector_args`
        // now legitimately collapses it via a separate, dedicated pass
        // and its own `vector_args_factored` stat, so the two stats are
        // asserted independently instead of asserting the pre-Phase-29.3
        // output was left untouched.
        let r = golf("void f(float w){vec3 a=vec3(w,w,w);}", true);
        assert_eq!(r.code, "void b(float a){vec3 c=vec3(a);}");
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 0);
        assert_eq!(r.stats.aggressive.vector_args_factored, 1);
    }

    #[test]
    fn factors_identical_dotted_swizzle_chain_arguments() {
        // `vec3(p.x,p.x,p.x)` is three token-identical dotted member
        // chains, not bare identifiers, but still a "pure identifier
        // expression" per golf.md Phase 29.3, so it factors the same way.
        let r = golf("void f(vec3 p){vec3 a=vec3(p.x,p.x,p.x);}", true);
        assert_eq!(r.code, "void b(vec3 a){vec3 c=vec3(a.x);}");
        assert_eq!(r.stats.aggressive.vector_args_factored, 1);
    }

    #[test]
    fn refuses_to_factor_a_vector_with_one_differing_argument() {
        // golf.md Phase 29.3's own example: `vec4(p.x,p.x,p.x,1.)` is
        // left alone because the fourth argument is not the same
        // identifier expression as the other three (a mix of an
        // identifier chain and a literal never counts as "all-equal").
        let r = golf("void f(vec3 p){vec4 a=vec4(p.x,p.x,p.x,1.0);}", true);
        assert_eq!(r.code, "void b(vec3 a){vec4 c=vec4(a.x,a.x,a.x,1.);}");
        assert_eq!(r.stats.aggressive.vector_args_factored, 0);
    }

    #[test]
    fn factoring_and_literal_reduction_never_double_count_the_same_call() {
        // The literal-only `reduce_constant_vectors` pass and the
        // identifier-only `factor_repeated_vector_args` pass are
        // mutually exclusive by construction (golf.md Phase 29.3): a
        // shader mixing both call shapes must attribute each reduction
        // to exactly one of the two stats, never both.
        let r = golf("void f(float w){vec3 a=vec3(1.0,1.0,1.0);vec3 b=vec3(w,w,w);}", true);
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 1);
        assert_eq!(r.stats.aggressive.vector_args_factored, 1);
    }

    #[test]
    fn reduces_constant_vec2_and_vec4() {
        let r = golf("void f(){vec2 a=vec2(1.,1.);}", true);
        assert_eq!(r.code, "void a(){vec2 b=vec2(1.);}");
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 1);
    }

    #[test]
    fn refuses_a_vector_with_more_arguments_than_its_arity() {
        let r = golf("void f(){vec4 a=vec4(1.,1.,1.,1.,1.);}", true);
        assert_eq!(r.code, "void a(){vec4 b=vec4(1.,1.,1.,1.,1.);}");
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 0);
    }

    #[test]
    fn folded_constants_feed_constant_vector_reduction() {
        let r = golf("void f(){vec3 a=vec3(2*3,2*3,2*3);}", true);
        assert_eq!(r.code, "void a(){vec3 b=vec3(6);}");
        assert_eq!(r.stats.aggressive.constants_folded, 3);
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 1);
    }

    #[test]
    fn folded_float_additions_feed_constant_vector_reduction() {
        let r = golf("void f(){vec3 a=vec3(2.0+1.0,2.0+1.0,2.0+1.0);}", true);
        assert_eq!(r.code, "void a(){vec3 b=vec3(3.);}");
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 1);
    }

    #[test]
    fn folded_result_and_an_untouched_literal_of_the_same_value_still_match() {
        let r = golf(
            "void f(){vec4 a=vec4(1000000.0+0.0,1000000.0,1000000.0,1000000.0);}",
            true,
        );
        assert_eq!(r.code, "void a(){vec4 b=vec4(1e6);}");
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 1);
    }

    #[test]
    fn a_folded_small_fraction_also_gets_the_scientific_notation_comparison() {
        let r = golf("void f(){vec2 a=vec2(0.00005+0.00005,0.0001);}", true);
        assert_eq!(r.code, "void a(){vec2 b=vec2(1e-4);}");
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 1);
    }

    #[test]
    fn strips_a_trailing_bare_return_in_a_void_function() {
        let r = golf("void f(){foo();return;}", true);
        assert_eq!(r.code, "void a(){foo();}");
        assert_eq!(r.stats.aggressive.trailing_void_returns_removed, 1);
    }

    #[test]
    fn strips_a_solitary_trailing_return() {
        let r = golf("void f(){return;}", true);
        assert_eq!(r.code, "void a(){}");
        assert_eq!(r.stats.aggressive.trailing_void_returns_removed, 1);
    }

    #[test]
    fn refuses_an_unbraced_if_bodied_trailing_return() {
        let r = golf("void f(){if(x)return;}", true);
        assert_eq!(r.code, "void a(){if(x)return;}");
        assert_eq!(r.stats.aggressive.trailing_void_returns_removed, 0);
    }

    #[test]
    fn refuses_the_same_trap_even_after_brace_stripping_exposes_it() {
        let r = golf("void f(){if(x){return;}}", true);
        assert_eq!(r.code, "void a(){if(x)return;}");
        assert_eq!(r.stats.aggressive.braces_removed, 1);
        assert_eq!(r.stats.aggressive.trailing_void_returns_removed, 0);
    }

    #[test]
    fn refuses_a_return_not_immediately_before_the_functions_own_close() {
        let r = golf("void f(){if(x)return;else bar();}", true);
        assert_eq!(r.code, "void a(){if(x)return;else bar();}");
        assert_eq!(r.stats.aggressive.trailing_void_returns_removed, 0);
    }

    #[test]
    fn refuses_a_return_carrying_a_value() {
        let r = golf("float f(){return 1.0;}", true);
        assert_eq!(r.code, "float a(){return 1.;}");
        assert_eq!(r.stats.aggressive.trailing_void_returns_removed, 0);
    }

    #[test]
    fn removes_a_local_never_referenced_again() {
        let r = golf("void f(){float unused=1.0;x=2.0;}", true);
        assert_eq!(r.code, "void a(){x=2.;}");
        assert_eq!(r.stats.aggressive.dead_locals_removed, 1);
    }

    #[test]
    fn removes_an_uninitialized_dead_local() {
        let r = golf("void f(){float unused;x=2.0;}", true);
        assert_eq!(r.code, "void a(){x=2.;}");
        assert_eq!(r.stats.aggressive.dead_locals_removed, 1);
    }

    #[test]
    fn refuses_to_remove_a_local_that_is_read_later() {
        let r = golf("void f(){float used=1.0;x=used;}", true);
        assert_eq!(r.code, "void b(){float a=1.;x=a;}");
        assert_eq!(r.stats.aggressive.dead_locals_removed, 0);
    }

    #[test]
    fn refuses_to_remove_when_initializer_calls_a_function() {
        let r = golf("void f(){float unused=foo(y);x=2.0;}", true);
        assert_eq!(r.code, "void a(){float b=foo(y);x=2.;}");
        assert_eq!(r.stats.aggressive.dead_locals_removed, 0);
    }

    #[test]
    fn refuses_to_remove_an_array_declarator() {
        let r = golf("void f(){float unused[3];x=2.0;}", true);
        assert_eq!(r.code, "void a(){float b[3];x=2.;}");
        assert_eq!(r.stats.aggressive.dead_locals_removed, 0);
    }

    #[test]
    fn dead_local_removal_can_enable_a_later_declaration_merge() {
        let r = golf("void f(){float p=1.0;float unused=2.0;float q=3.0;x=p+q;}", true);
        assert_eq!(r.code, "void c(){float a=1.,b=3.;x=a+b;}");
        assert_eq!(r.stats.aggressive.dead_locals_removed, 1);
        assert_eq!(r.stats.aggressive.declarations_merged, 1);
    }

    #[test]
    fn struct_member_named_like_a_swizzle_is_never_renamed() {
        let r = golf(
            "struct Foo{float x;float y;};void mainImage(out vec4 fragColor,in vec2 fragCoord){Foo f;f.x=1.0;f.y=2.0;vec3 p=vec3(1.0,2.0,3.0);vec3 q=p.xyz+p.x;fragColor=vec4(q,f.x+f.y);}",
            false,
        );
        assert_eq!(
            r.code,
            "struct b{float x;float y;};void mainImage(out vec4 c,in vec2 e){b f;f.x=1.;f.y=2.;vec3 a=vec3(1.,2.,3.);vec3 d=a.xyz+a.x;c=vec4(d,f.x+f.y);}"
        );
    }

    #[test]
    fn unrecognized_struct_instance_name_is_protected_from_collision() {
        let r = golf(
            "struct W{float v;};void h(){W a;float longName=1.0;longName=longName+1.0;}",
            false,
        );
        assert_eq!(r.code, "struct c{float v;};void d(){c a;float b=1.;b=b+1.;}");
    }

    #[test]
    fn name_referenced_only_inside_a_macro_body_is_protected_from_collision() {
        let r = golf(
            "#define a 3.0\n#define TAU (2.0*a)\nvoid mainImage(out vec4 fragColor,in vec2 fragCoord){float velocity=1.0;fragColor=vec4(velocity+TAU);}",
            false,
        );
        assert_eq!(
            r.code,
            "#define a 3.0\n#define TAU (2.0*a)\nvoid mainImage(out vec4 b,in vec2 d){float c=1.;b=vec4(c+TAU);}"
        );
    }

    #[test]
    fn protected_names_are_never_renamed() {
        let r = golf_with_protected_names(
            "uniform float uSpeed;void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(uSpeed);}",
            AggressiveOptions::none(),
            &["uSpeed".to_string()],
        );
        assert!(r.code.contains("uSpeed"), "protected name must survive verbatim: {}", r.code);
    }

    #[test]
    fn protected_names_also_reserve_the_spelling_from_reuse() {
        let r = golf_with_protected_names(
            "uniform float keep;void mainImage(out vec4 fragColor,in vec2 fragCoord){float longLocalName=1.0;fragColor=vec4(keep+longLocalName);}",
            AggressiveOptions::none(),
            &["keep".to_string()],
        );
        assert!(!r.code.contains("float keep="), "the spelling \"keep\" must never be handed to a different variable: {}", r.code);
        assert!(r.code.contains("keep"), "the protected uniform must still appear under its own name: {}", r.code);
    }

    #[test]
    fn protected_single_character_names_are_never_reused_even_in_aggressive_mode() {
        // Regression test for a real user-reported shader: this app's Twigl
        // Export panel protects r/m/t/f (the Geek-family iResolution/iMouse/
        // iTime/iFrame shorthand) specifically so the golfer never picks
        // those *particular* single-character names for some other local --
        // but with several one-line locals and full aggressive renaming, the
        // golfer will always try single-character names first, so this is
        // exactly the case `protected_names_also_reserve_the_spelling_from_reuse`
        // (which uses a 4-character protected name the golfer would never
        // reach anyway) does not actually exercise.
        let source = "void mainImage(out vec4 fragColor,in vec2 fragCoord){\
            float distanceAccumulator=0.0;\
            float stepCount=0.0;\
            vec3 hitPoint=vec3(0.0);\
            vec3 sceneColor=vec3(0.0);\
            for(int i=0;i<10;i++){distanceAccumulator+=stepCount;hitPoint+=sceneColor;}\
            fragColor=vec4(distanceAccumulator+hitPoint.x+sceneColor.x,0.0,0.0,1.0);\
        }";
        let protected: Vec<String> = vec!["r".to_string(), "m".to_string(), "t".to_string(), "f".to_string()];
        let r = golf_with_protected_names(source, AggressiveOptions::all(), &protected);
        fn contains_identifier(text: &str, name: &str) -> bool {
            let mut start = 0usize;
            while let Some(pos) = text[start..].find(name) {
                let abs = start + pos;
                let end = abs + name.len();
                let before_ok = text[..abs].chars().last().map_or(true, |c| !(c.is_alphanumeric() || c == '_'));
                let after_ok = text[end..].chars().next().map_or(true, |c| !(c.is_alphanumeric() || c == '_'));
                if before_ok && after_ok {
                    return true;
                }
                start = abs + 1;
            }
            false
        }
        for reserved in ["r", "m", "t", "f"] {
            assert!(
                !contains_identifier(&r.code, reserved),
                "protected single-character name '{reserved}' must never be assigned to an unrelated local (none of r/m/t/f are used as uniform names here, so none should survive at all): {}",
                r.code
            );
        }
    }

    #[test]
    fn renaming_is_deterministic() {
        let source = include_str!("../../fixtures/frequency_renaming.glsl");
        let mut opts = AggressiveOptions::all();
        opts.frequency_aware_renaming = true;
        let first = golf_with_protected_names(source, opts, &[]).code;
        for _ in 0..8 {
            let again = golf_with_protected_names(source, opts, &[]).code;
            assert_eq!(again, first);
        }
    }

    #[test]
    fn frequency_aware_renaming_never_worsens_deflate_budget_on_tracked_sources() {
        let sources = [
            include_str!("../../fixtures/frequency_renaming.glsl"),
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){float minimumInside=min(sin(fragCoord.x),sin(fragCoord.y));float minimumInline=min(minimumInside,sin(minimumInside));float minimumFinal=min(minimumInline,minimumInside)+sin(minimumInline);fragColor=vec4(minimumInside+minimumInline+minimumFinal);}",
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){float floorFactor=floor(fragCoord.x)+floor(fragCoord.y);float fractFactor=fract(fragCoord.x)+fract(fragCoord.y);float finalFactor=floorFactor+fractFactor+floor(floorFactor);fragColor=vec4(floorFactor+fractFactor+finalFactor);}",
        ];

        for source in sources {
            let mut naive = AggressiveOptions::all();
            naive.frequency_aware_renaming = false;
            let mut freq = AggressiveOptions::all();
            freq.frequency_aware_renaming = true;

            let naive_result = golf_with_protected_names(source, naive, &[]);
            let freq_result = golf_with_protected_names(source, freq, &[]);

            assert!(
                estimate_budget(&freq_result.code).deflate_bytes <= estimate_budget(&naive_result.code).deflate_bytes,
                "frequency-aware rename must not inflate the DEFLATE estimate\nnaive: {}\nfreq : {}",
                naive_result.code,
                freq_result.code
            );
        }
    }

    #[test]
    fn frequency_aware_renaming_preserves_output_when_no_better_mapping_exists() {
        let source = include_str!("../../fixtures/frequency_renaming.glsl");
        let mut naive = AggressiveOptions::all();
        naive.frequency_aware_renaming = false;
        let mut freq = AggressiveOptions::all();
        freq.frequency_aware_renaming = true;

        let naive_result = golf_with_protected_names(source, naive, &[]);
        let freq_result = golf_with_protected_names(source, freq, &[]);

        assert_eq!(
            estimate_budget(&freq_result.code).deflate_bytes,
            estimate_budget(&naive_result.code).deflate_bytes
        );
        assert_eq!(freq_result.code, naive_result.code);
    }

    // Mirrors assign_rename_map's own setup (tokenize, compute taken/scope
    // context, char/bigram frequency) so the optimized-vs-reference
    // comparison test below exercises the exact same inputs
    // choose_frequency_aware_candidate sees in production, just routed
    // through whichever candidate-selection implementation is requested.
    fn assign_rename_map_for_test(
        source: &str,
        aggressive: AggressiveOptions,
        use_reference_frequency_candidate: bool,
    ) -> HashMap<String, String> {
        let spaced = tokenize_spaced(source);
        let tokens: Vec<Tok> = spaced.iter().map(|(t, _)| t.clone()).collect();
        let had_space: Vec<bool> = spaced.iter().map(|(_, s)| *s).collect();

        let kw = keywords();
        let builtins = builtin_functions();
        let builtin_vars = builtin_variables();
        let protected = protected_host_names();

        let renamable: Vec<(String, Scope)> = find_renamable(&tokens);

        let mut taken: HashSet<String> = HashSet::new();
        taken.extend(kw.iter().map(|s| s.to_string()));
        taken.extend(builtins.iter().map(|s| s.to_string()));
        taken.extend(builtin_vars.iter().map(|s| s.to_string()));
        taken.extend(protected.iter().map(|s| s.to_string()));
        let renamable_set: HashSet<&str> = renamable.iter().map(|(name, _)| name.as_str()).collect();
        for tok in &tokens {
            if let Tok::Ident(name) = tok {
                if !renamable_set.contains(name.as_str()) {
                    taken.insert(name.clone());
                }
            }
        }
        taken.extend(preproc_referenced_names(&tokens));

        let base_code = render_code(&tokens, &had_space, &HashMap::new());
        let (char_frequency, bigram_frequency) = collect_char_and_bigram_frequency(&base_code);
        let block_scopes = block_scope_tree(&tokens);

        if !use_reference_frequency_candidate {
            return assign_rename_map(
                &renamable, aggressive, &tokens, &had_space, &taken, &block_scopes,
                &char_frequency, &bigram_frequency,
            );
        }

        let mut local_taken: HashMap<usize, HashSet<String>> = HashMap::new();
        let mut rename_map: HashMap<String, String> = HashMap::new();
        for (original, scope) in &renamable {
            let candidate = if aggressive.frequency_aware_renaming {
                choose_frequency_aware_candidate_reference(
                    original, scope, &tokens, &had_space, &rename_map, &taken, &local_taken,
                    &block_scopes, &char_frequency, &bigram_frequency,
                )
            } else {
                first_available_candidate(scope, &taken, &local_taken, &block_scopes).1
            };
            register_candidate(&candidate, original, scope, &mut taken, &mut local_taken, &mut rename_map);
        }
        rename_map
    }

    // ROADMAP.md's own Phase 37.1 note flagged choose_frequency_aware_candidate's
    // O(identifiers x candidates x file-size) cost as a known, previously
    // unfixed performance cliff -- deliberately left alone because touching
    // "already-shipped, exact-string-tested rename logic" was judged too
    // risky without dedicated verification. This test is that dedicated
    // verification: the optimized placeholder-substitution path must select
    // the exact same candidate, for every renamable identifier, as the
    // original always-correct-but-slow full-render implementation (kept as
    // choose_frequency_aware_candidate_reference specifically to make this
    // comparison possible) -- across a broad sample of the fixture corpus,
    // not just the one or two shapes the pre-existing tests above happen to
    // cover.
    #[test]
    #[ignore = "runs the deliberately-slow reference implementation across \
                17 fixtures for comparison (~4 minutes); not part of the \
                default suite, run explicitly with `cargo test -- --ignored` \
                after touching choose_frequency_aware_candidate"]
    fn frequency_aware_candidate_optimized_path_matches_the_reference_implementation_on_every_fixture() {
        let fixtures: &[&str] = &[
            include_str!("../../fixtures/frequency_renaming.glsl"),
            include_str!("../../fixtures/macro_cse.glsl"),
            include_str!("../../fixtures/aggressive_inlining.glsl"),
            include_str!("../../fixtures/fractal.glsl"),
            include_str!("../../fixtures/scope_aware_renaming.glsl"),
            include_str!("../../fixtures/block_scope_renaming.glsl"),
            include_str!("../../fixtures/swizzle_alphabet.glsl"),
            include_str!("../../fixtures/swizzle_after_dot.glsl"),
            include_str!("../../fixtures/loop_form_golf.glsl"),
            include_str!("../../fixtures/loop_header_golf.glsl"),
            include_str!("../../fixtures/declarations.glsl"),
            include_str!("../../fixtures/declaration_hoisting.glsl"),
            include_str!("../../fixtures/common_subexpressions.glsl"),
            include_str!("../../fixtures/vector_argument_factoring.glsl"),
            include_str!("../../fixtures/statement_fusion.glsl"),
            include_str!("../../fixtures/struct_safety.glsl"),
            include_str!("../../fixtures/twigl_source.glsl"),
        ];
        let mut opts = AggressiveOptions::all();
        opts.frequency_aware_renaming = true;

        for source in fixtures {
            let fast = assign_rename_map_for_test(source, opts, false);
            let reference = assign_rename_map_for_test(source, opts, true);
            assert_eq!(
                fast, reference,
                "optimized frequency-aware candidate selection diverged from the \
                 reference implementation on fixture:\n{source}"
            );
        }
    }

    #[test]
    fn unique_placeholder_never_collides_with_text_already_present() {
        let code = "USHADERFREQPLACEHOLDER and USHADERFREQPLACEHOLDERQ both appear here";
        let placeholder = unique_placeholder(code).expect("a free placeholder must exist");
        assert!(!code.contains(&placeholder));
    }

    #[test]
    fn declaration_heuristic_ignores_non_type_keywords() {
        let r = golf("void f(){return z;}", false);
        assert_eq!(r.code, "void a(){return z;}");

        let r = golf("struct Foo{float x;};void f(){Foo a;}", false);
        assert_eq!(r.code, "struct b{float x;};void c(){b a;}");
    }

    #[test]
    fn protects_a_declared_name_also_referenced_inside_a_macro_body() {
        let r = golf(
            "#define GET_X(p) (p.x + OFFSET)\nvoid mainImage(out vec4 fragColor, in vec2 fragCoord){float OFFSET = 1.0;fragColor=vec4(GET_X(fragCoord),0.0,0.0,1.0);}",
            false,
        );
        assert_eq!(
            r.code,
            "#define GET_X(p) (p.x + OFFSET)\nvoid mainImage(out vec4 a,in vec2 b){float OFFSET=1.;a=vec4(GET_X(b),0.,0.,1.);}"
        );
    }

    #[test]
    fn scope_aware_renaming_reuses_short_names_across_independent_functions() {
        let r = golf(
            "float helperOne(float longParamName){float localVarOne=longParamName*2.0;return localVarOne;}\nfloat helperTwo(float anotherParam){float localVarTwo=anotherParam+1.0;return localVarTwo;}\nvoid mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(helperOne(1.0)+helperTwo(2.0),0.0,0.0,1.0);}",
            false,
        );
        assert_eq!(
            r.code,
            "float a(float b){float c=b*2.;return c;}float d(float b){float c=b+1.;return c;}void mainImage(out vec4 b,in vec2 c){b=vec4(a(1.)+d(2.),0.,0.,1.);}"
        );
    }

    #[test]
    fn block_scope_renaming_reuses_a_name_across_disjoint_if_else_branches() {
        let r = golf(
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){float x=0.0;if(x>0.5){float tempResult=x*2.0;x=tempResult;}else{float otherThing=x+1.0;x=otherThing;}fragColor=vec4(x);}",
            true,
        );
        assert_eq!(
            r.code,
            "void mainImage(out vec4 b,in vec2 d){float a=0.;if(a>.5){float c=a*2.;a=c;}else{float c=a+1.;a=c;}b=vec4(a);}"
        );
    }

    #[test]
    fn block_scope_renaming_reuses_a_loop_counter_across_two_disjoint_for_loops() {
        let r = golf(
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){float s=0.0;for(int i=0;i<3;i++){s+=float(i);}for(int i=0;i<5;i++){s+=float(i)*2.0;}fragColor=vec4(s);}",
            true,
        );
        assert_eq!(
            r.code,
            "void mainImage(out vec4 c,in vec2 d){float b=0.;for(int a=0;a<3;a++)b+=float(a);for(int a=0;a<5;a++)b+=float(a)*2.;c=vec4(b);}"
        );
    }

    #[test]
    fn block_scope_renaming_never_collides_a_descendant_scope_with_its_ancestor() {
        let r = golf(
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){float s=0.0;for(int i=0;i<3;i++){s+=float(i);}for(int i=0;i<5;i++){s+=float(i)*2.0;}fragColor=vec4(s);}",
            true,
        );
        assert_ne!(
            r.code, "void mainImage(out vec4 c,in vec2 d){float a=0.;for(int a=0;a<3;a++)a+=float(a);for(int a=0;a<5;a++)a+=float(a)*2.;c=vec4(a);}",
            "the loop counter and the outer accumulator must never be renamed to the same identifier"
        );
    }

    #[test]
    fn block_scope_renaming_never_reuses_a_name_across_nested_scopes() {
        let r = golf(
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){float outer=1.0;if(outer>0.0){float mid=2.0;if(mid>0.0){float inner=3.0;outer=inner;}}fragColor=vec4(outer);}",
            true,
        );
        assert_eq!(
            r.code,
            "void mainImage(out vec4 b,in vec2 e){float a=1.;if(a>0.){float c=2.;if(c>0.){float d=3.;a=d;}}b=vec4(a);}"
        );
    }

    #[test]
    fn block_scope_renaming_reuses_a_name_across_three_disjoint_sibling_blocks() {
        let r = golf(
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){if(true){float a1=1.0;fragColor=vec4(a1);}else{float b1=2.0;fragColor=vec4(b1);}if(true){float c1=3.0;fragColor+=vec4(c1);}}",
            true,
        );
        assert_eq!(
            r.code,
            "void mainImage(out vec4 a,in vec2 c){if(true){float b=1.;a=vec4(b);}else{float b=2.;a=vec4(b);}if(true){float b=3.;a+=vec4(b);}}"
        );
    }

    #[test]
    fn eliminates_a_chain_of_adjacent_dead_stores() {
        let r = golf("void f(){x=1.0;x=2.0;x=3.0;foo(x);}", true);
        assert_eq!(r.code, "void a(){x=3.;foo(x);}");
        assert_eq!(r.stats.aggressive.dead_stores_removed, 2);
    }

    #[test]
    fn reduces_a_dead_initializer_to_a_bare_declaration() {
        let r = golf("void f(){float x=1.0;x=2.0;foo(x);}", true);
        assert_eq!(r.code, "void b(){float a;a=2.;foo(a);}");
        assert_eq!(r.stats.aggressive.dead_stores_removed, 1);
    }

    #[test]
    fn refuses_to_drop_a_write_the_next_statement_reads() {
        let r = golf("void f(){x=1.0;x=x;foo(x);}", true);
        assert_eq!(r.code, "void a(){x=1.;x=x;foo(x);}");
        assert_eq!(r.stats.aggressive.dead_stores_removed, 0);
    }

    #[test]
    fn refuses_to_treat_a_compound_assignment_as_superseding() {
        let r = golf("void f(){x=1.0;x+=2.0;foo(x);}", true);
        assert_eq!(r.code, "void a(){x=1.;x+=2.;foo(x);}");
        assert_eq!(r.stats.aggressive.dead_stores_removed, 0);
    }

    #[test]
    fn never_matches_a_for_headers_own_clauses() {
        let r = golf("void f(){for(int i=0;i<9;i++){x+=1.0;}}", true);
        assert_eq!(r.code, "void b(){for(int a=0;a<9;a++)++x;}");
        assert_eq!(r.stats.aggressive.dead_stores_removed, 0);
    }

    #[test]
    fn catches_a_dead_store_separated_by_an_unrelated_statement() {
        let r = golf("void f(){x=1.0;y=2.0;x=3.0;foo(x,y);}", true);
        assert_eq!(r.code, "void a(){y=2.;x=3.;foo(x,y);}");
        assert_eq!(r.stats.aggressive.dead_stores_removed, 1);
    }

    #[test]
    fn still_declines_when_an_intervening_write_reads_the_tracked_name() {
        let r = golf("void f(){x=1.0;y=x;x=3.0;foo(x,y);}", true);
        assert_eq!(r.code, "void a(){x=1.;y=x;x=3.;foo(x,y);}");
        assert_eq!(r.stats.aggressive.dead_stores_removed, 0);
    }

    #[test]
    fn strips_parens_around_a_single_literal() {
        let r = golf("void f(){float a=(1.0);foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=1.;foo(a);}");
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 1);
    }

    #[test]
    fn strips_nested_parens_via_the_fixpoint_loop() {
        let r = golf("void f(){float a=((1.0));foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=1.;foo(a);}");
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 2);
    }

    #[test]
    fn refuses_parens_around_more_than_one_primary() {
        let r = golf("void f(){float a=(x+y);foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=(x+y);foo(a);}");
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 0);
    }

    #[test]
    fn refuses_a_real_function_calls_parens() {
        let r = golf("void f(){vec3 a=vec3((1.0));foo(a);}", true);
        assert_eq!(r.code, "void b(){vec3 a=vec3(1.);foo(a);}");
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 1);
    }

    #[test]
    fn refuses_a_control_flow_keywords_mandatory_parens() {
        let r = golf("void f(){if((true)){foo();}}", true);
        assert_eq!(r.code, "void a(){if(true)foo();}");
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 1);
    }

    #[test]
    fn refuses_parens_around_a_binary_expression_used_as_an_operand() {
        let r = golf("void f(){float a=(x+y)*2.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=(x+y)*2.;foo(a);}");
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 0);
    }

    #[test]
    fn preserves_a_disambiguating_space_after_stripping_parens_around_a_unary_minus() {
        let r = golf("void f(){float x=1.0;float a;a=5.0-(-x);foo(a);}", true);
        assert_eq!(r.code, "void c(){float b=1.,a;a=5.- -b;foo(a);}");
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 1);
    }

    #[test]
    fn preserves_a_disambiguating_space_after_stripping_parens_around_a_unary_plus() {
        let r = golf("void f(){float x=1.0;float a;a=5.0+(+x);foo(a);}", true);
        assert_eq!(r.code, "void c(){float b=1.,a;a=5.+ +b;foo(a);}");
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 1);
    }

    #[test]
    fn does_not_force_an_unnecessary_space_when_no_fusion_risk_exists() {
        let r = golf("void f(){float x=1.0;float a;a=5.0*(-x);foo(a);}", true);
        assert_eq!(r.code, "void c(){float b=1.,a;a=5.*-b;foo(a);}");
        assert_eq!(r.stats.aggressive.redundant_parens_removed, 1);
    }

    #[test]
    fn folds_a_float_multiplication() {
        let r = golf("void f(){float a=2.0*3.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=6.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 1);
    }

    #[test]
    fn folds_a_float_multiplication_chain() {
        let r = golf("void f(){float a=2.0*3.0*4.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=24.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 2);
    }

    #[test]
    fn folds_a_float_additive_chain() {
        let r = golf("void f(){float a=1.0+2.0+3.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=6.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 1);
    }

    #[test]
    fn folds_a_negative_float_result() {
        let r = golf("void f(){float a=3.0-5.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=-2.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 1);
    }

    #[test]
    fn folds_a_leading_unary_sign_into_a_float_chain() {
        let r = golf("void f(){float a=-5.0+3.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=-2.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 1);
    }

    #[test]
    fn refuses_to_fold_a_float_additive_chain_preceded_by_a_variable() {
        let r = golf("void f(){float a=x-1.0+2.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=x-1.+2.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn refuses_to_fold_float_division() {
        let r = golf("void f(){float a=1.0/2.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=1./2.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn refuses_float_literals_with_an_exponent_or_suffix() {
        let r = golf("void f(){float a=1.0e5*2.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=1.e5*2.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 0);

        let r = golf("void f(){float a=1.0f*2.0f;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=1.f*2.f;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn refuses_to_fold_a_float_multiplication_that_overflows_to_infinity() {
        let r = golf(
            "void f(){float a=999999999999999999999999999999.0*999999999999999999999999999999.0;foo(a);}",
            true,
        );
        assert_eq!(r.code, "void b(){float a=1e30*1e30;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn refuses_to_fold_a_float_chain_that_would_produce_negative_zero() {
        let r = golf("void f(){float a=-0.0-0.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=-0.-0.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 0);
    }

    #[test]
    fn simplifies_multiplicative_and_additive_identities_on_identifiers() {
        let r = golf(
            "void f(){float a=x*1.0;float b=1.0*x;float c=x/1.0;float d=x+0.0;float e=0.0+x;float g=x-0.0;foo(a,b,c,d,e,g);}",
            true,
        );
        assert!(r.stats.aggressive.algebraic_identities_simplified >= 6);
        assert!(!r.code.contains('*'));
        assert!(!r.code.contains('/'));
    }

    #[test]
    fn simplifies_pow_of_two_on_a_single_identifier() {
        let r = golf("void f(){float a=pow(x,2.0);foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=x*x;foo(a);}");
        assert_eq!(r.stats.aggressive.algebraic_identities_simplified, 1);
    }

    #[test]
    fn does_not_simplify_identities_on_numeric_literal_operands() {
        let r = golf("void f(){float a=2.0*1.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=2.;foo(a);}");
    }

    #[test]
    fn does_not_duplicate_a_call_expression_for_pow_square() {
        let r = golf("void f(){float a=pow(rand(),2.0);foo(a);}", true);
        assert_eq!(r.stats.aggressive.algebraic_identities_simplified, 0);
        assert!(r.code.contains("pow("));
    }

    #[test]
    fn common_subexpression_elimination_reuses_the_first_variable() {
        let r = golf(
            "void f(vec3 p){float a=dot(p,p);float b=dot(p,p);foo(a,b);}",
            true,
        );
        assert_eq!(r.stats.aggressive.common_subexpressions_eliminated, 1);
        assert_eq!(r.code, "void d(vec3 a){float b=dot(a,a),c=b;foo(b,c);}");
    }

    #[test]
    fn common_subexpression_elimination_references_the_actual_renamed_variable() {
        // Regression test: the first implementation read the pre-rename identifier
        // text out of Item::tok (which the renaming pass deliberately leaves
        // untouched for other passes) instead of the post-rename Item::text,
        // producing a reference to a name that was never actually declared (in
        // this repro, the enclosing function's own new name) rather than to the
        // variable that really held the cached value.
        let r = golf(
            "void f(){float d=dot(a,a);float e=dot(a,a);g(d,e);}",
            true,
        );
        assert_eq!(r.stats.aggressive.common_subexpressions_eliminated, 1);
        // The rewritten declaration must reference a name that was actually
        // declared as a float immediately before it, never the function name.
        assert_eq!(r.code, "void d(){float b=dot(a,a),c=b;g(b,c);}");
    }

    #[test]
    fn common_subexpression_cache_does_not_survive_into_a_shadowing_block() {
        // If the cache were not cleared on entering the nested block, "sin(p)"
        // inside the if-branch would be wrongly matched against the outer
        // "sin(p)" even though the inner "p" is a different, shadowed variable
        // with a different value.
        let r = golf(
            "void f(){float p=1.0;float a=sin(p);if(true){float p=2.0;float b=sin(p);g(a,b);}}",
            true,
        );
        assert_eq!(r.stats.aggressive.common_subexpressions_eliminated, 0);
    }

    #[test]
    fn common_subexpression_cache_does_not_survive_a_plain_assignment() {
        let r = golf(
            "void f(vec3 p){float a=dot(p,p);p.x+=1.0;float b=dot(p,p);foo(a,b);}",
            true,
        );
        assert_eq!(r.stats.aggressive.common_subexpressions_eliminated, 0);
    }

    #[test]
    fn float_multiplication_then_addition_compose_across_the_fixpoint_loop() {
        let r = golf("void f(){float a=2.0+3.0*4.0;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=14.;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 2);
    }

    #[test]
    fn folds_a_float_result_that_needs_host_precision_agreement() {
        let r = golf("void f(){float a=0.1+0.2;foo(a);}", true);
        assert_eq!(r.code, "void b(){float a=0.3;foo(a);}");
        assert_eq!(r.stats.aggressive.constants_folded, 1);
    }

    #[test]
    fn shortens_a_large_whole_number_to_scientific_notation() {
        let r = golf("void f(){float a=1000000.0;foo(a);}", false);
        assert_eq!(r.code, "void b(){float a=1e6;foo(a);}");
        assert_eq!(r.stats.numbers_shortened, 1);
    }

    #[test]
    fn shortens_a_small_fraction_to_scientific_notation() {
        let r = golf("void f(){float a=0.0001;foo(a);}", false);
        assert_eq!(r.code, "void b(){float a=1e-4;foo(a);}");
    }

    #[test]
    fn keeps_decimal_form_when_it_is_already_shorter() {
        let r = golf("void f(){float a=123456.0;foo(a);}", false);
        assert_eq!(r.code, "void b(){float a=123456.;foo(a);}");
    }

    #[test]
    fn keeps_decimal_form_on_an_exact_tie() {
        let r = golf("void f(){float a=0.000123;foo(a);}", false);
        assert_eq!(r.code, "void b(){float a=.000123;foo(a);}");
    }

    #[test]
    fn never_converts_a_bare_integer_to_scientific_notation() {
        let r = golf("void f(){int a[1000000];foo(a[0]);}", false);
        assert_eq!(r.code, "void b(){int a[1000000];foo(a[0]);}");
        assert_eq!(r.stats.numbers_shortened, 0);
    }

    #[test]
    fn leaves_a_literal_that_already_has_an_exponent_untouched_by_this_comparison() {
        let r = golf("void f(){float a=1.5e10;foo(a);}", false);
        assert_eq!(r.code, "void b(){float a=1.5e10;foo(a);}");
    }

    #[test]
    fn scientific_notation_correctly_carries_a_type_suffix() {
        let r = golf("void f(){float a=1000000.0f;foo(a);}", false);
        assert_eq!(r.code, "void b(){float a=1e6f;foo(a);}");
    }

    #[test]
    fn strips_an_exact_duplicate_precision_statement() {
        let r = golf(
            "precision highp float;precision highp float;void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}",
            true,
        );
        assert_eq!(
            r.code,
            "precision highp float;void mainImage(out vec4 a,in vec2 b){a=vec4(1.);}"
        );
        assert_eq!(r.stats.aggressive.duplicate_precision_removed, 1);
    }

    #[test]
    fn collapses_a_triple_duplicate_precision_statement_to_one() {
        let r = golf(
            "precision highp float;precision highp float;precision highp float;void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}",
            true,
        );
        assert_eq!(
            r.code,
            "precision highp float;void mainImage(out vec4 a,in vec2 b){a=vec4(1.);}"
        );
        assert_eq!(r.stats.aggressive.duplicate_precision_removed, 2);
    }

    #[test]
    fn keeps_a_single_precision_statement_untouched() {
        let r = golf(
            "precision highp float;void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}",
            true,
        );
        assert_eq!(
            r.code,
            "precision highp float;void mainImage(out vec4 a,in vec2 b){a=vec4(1.);}"
        );
        assert_eq!(r.stats.aggressive.duplicate_precision_removed, 0);
    }

    #[test]
    fn keeps_precision_statements_that_differ_in_qualifier() {
        let r = golf(
            "precision highp float;precision mediump float;void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}",
            true,
        );
        assert_eq!(
            r.code,
            "precision highp float;precision mediump float;void mainImage(out vec4 a,in vec2 b){a=vec4(1.);}"
        );
        assert_eq!(r.stats.aggressive.duplicate_precision_removed, 0);
    }

    #[test]
    fn keeps_precision_statements_that_differ_in_type() {
        let r = golf(
            "precision highp float;precision highp int;void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}",
            true,
        );
        assert_eq!(
            r.code,
            "precision highp float;precision highp int;void mainImage(out vec4 a,in vec2 b){a=vec4(1.);}"
        );
        assert_eq!(r.stats.aggressive.duplicate_precision_removed, 0);
    }

    #[test]
    fn removes_a_function_never_called_from_mainimage() {
        let r = golf(
            "float unused(float x){return x*2.0;}void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}",
            true,
        );
        assert_eq!(r.code, "void mainImage(out vec4 a,in vec2 c){a=vec4(1.);}");
        assert_eq!(r.stats.aggressive.dead_functions_removed, 1);
    }

    #[test]
    fn keeps_a_function_called_from_mainimage_that_isnt_a_single_call_site_inlining_candidate() {
        let r = golf(
            "float helper(float x){return x*2.0;}void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(helper(1.0)+helper(2.0));}",
            true,
        );
        assert_eq!(
            r.code,
            "float a(float b){return b*2.;}void mainImage(out vec4 b,in vec2 c){b=vec4(a(1.)+a(2.));}"
        );
        assert_eq!(r.stats.aggressive.dead_functions_removed, 0);
        assert_eq!(r.stats.aggressive.functions_inlined, 0);
    }

    #[test]
    fn inlines_and_then_folds_a_single_call_site_helper_reachable_only_transitively() {
        let r = golf(
            "float a(float x){return b(x);}float b(float x){return x*2.0;}void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(a(1.0));}",
            true,
        );
        assert_eq!(r.code, "void mainImage(out vec4 a,in vec2 d){a=vec4(2.);}");
        assert_eq!(r.stats.aggressive.dead_functions_removed, 0);
        assert_eq!(r.stats.aggressive.functions_inlined, 2);
    }

    #[test]
    fn removes_a_mutually_recursive_pair_thats_unreachable_from_any_entry_point() {
        let r = golf(
            "float dead1(){return 1.0;}float dead2(){return dead1();}void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}",
            true,
        );
        assert_eq!(r.code, "void mainImage(out vec4 b,in vec2 d){b=vec4(1.);}");
        assert_eq!(r.stats.aggressive.dead_functions_removed, 2);
    }

    #[test]
    fn keeps_all_overloads_of_a_reachable_name() {
        let r = golf(
            "float f(float x){return x;}float f(vec2 x){return x.x;}void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(f(1.0));}",
            true,
        );
        assert_eq!(
            r.code,
            "float b(float a){return a;}float b(vec2 a){return a.x;}void mainImage(out vec4 a,in vec2 c){a=vec4(b(1.));}"
        );
        assert_eq!(r.stats.aggressive.dead_functions_removed, 0);
    }

    #[test]
    fn declines_entirely_when_there_is_no_recognized_entry_point() {
        let r = golf("float helper(float x){return x*2.0;}", true);
        assert_eq!(r.code, "float b(float a){return a*2.;}");
        assert_eq!(r.stats.aggressive.dead_functions_removed, 0);
    }

    #[test]
    fn fuses_a_run_of_adjacent_fusable_statements() {
        // golf.md Phase 30.3: an assignment, another assignment, and a
        // postfix increment in a row -- all three are bare comma-operand
        // shapes -- collapse into one `a,b,c;` statement.
        let mut opts = AggressiveOptions::none();
        opts.fuse_statement_sequences = true;
        let r = golf_with_protected_names(
            "void f(){float a;float b;a=1.0;b=2.0;a++;}",
            opts,
            &["f".to_string(), "a".to_string(), "b".to_string()],
        );
        assert_eq!(r.code, "void f(){float a;float b;a=1.,b=2.,a++;}");
        assert_eq!(r.stats.aggressive.statement_sequences_fused, 1);
    }

    #[test]
    fn never_fuses_a_declaration_into_the_sequence() {
        // The two declarations remain their own statements -- `float` is a
        // reserved word, so `classify_fusable_statement` declines at the
        // very first token and the run boundary holds -- only the two
        // assignments after them fuse together.
        let mut opts = AggressiveOptions::none();
        opts.fuse_statement_sequences = true;
        let r = golf_with_protected_names(
            "void f(){float a;float b;a=1.0;b=2.0;}",
            opts,
            &["f".to_string(), "a".to_string(), "b".to_string()],
        );
        assert_eq!(r.code, "void f(){float a;float b;a=1.,b=2.;}");
        assert_eq!(r.stats.aggressive.statement_sequences_fused, 1);
    }

    #[test]
    fn never_fuses_across_an_if_statement_or_its_closing_brace() {
        // Regression test for the same brace-boundary discipline already
        // established for the Phase 11 CSE pass (ROADMAP.md Phase 11 bug
        // #2): a `}` closing an `if` body is a statement boundary exactly
        // like a `;`, so the run started by `b=3.0;a++;` after the `if`
        // must never be allowed to reach back across it and swallow
        // anything from inside -- or before -- the `if`.
        let mut opts = AggressiveOptions::none();
        opts.fuse_statement_sequences = true;
        let r = golf_with_protected_names(
            "void f(){float a;float b;a=1.0;if(a>0.0){b=2.0;}b=3.0;a++;}",
            opts,
            &["f".to_string(), "a".to_string(), "b".to_string()],
        );
        assert_eq!(r.code, "void f(){float a;float b;a=1.;if(a>0.){b=2.;}b=3.,a++;}");
        assert_eq!(r.stats.aggressive.statement_sequences_fused, 1);
    }

    #[test]
    fn never_fuses_a_return_statement_into_the_sequence() {
        // golf.md Phase 30.3's own "never fuses ... a `return`" rule:
        // `return` is a reserved word, so the run started by the two
        // assignments must stop right before it.
        let mut opts = AggressiveOptions::none();
        opts.fuse_statement_sequences = true;
        let r = golf_with_protected_names(
            "float f(float a,float b){a=a+1.0;b=b+1.0;return a+b;}",
            opts,
            &["f".to_string(), "a".to_string(), "b".to_string()],
        );
        assert_eq!(r.code, "float f(float a,float b){a=a+1.,b=b+1.;return a+b;}");
        assert_eq!(r.stats.aggressive.statement_sequences_fused, 1);
    }

    #[test]
    fn statement_fusion_stays_off_by_default_even_when_fusable_statements_exist() {
        let r = golf_with_protected_names(
            "void f(){float a;float b;a=1.0;b=2.0;}",
            AggressiveOptions::none(),
            &["f".to_string(), "a".to_string(), "b".to_string()],
        );
        assert_eq!(r.code, "void f(){float a;float b;a=1.;b=2.;}");
        assert_eq!(r.stats.aggressive.statement_sequences_fused, 0);
    }

    #[test]
    fn statement_fusion_never_worsens_deflate_budget_on_the_tracked_fixture() {
        let source = include_str!("../../fixtures/statement_fusion.glsl");
        let mut fused = AggressiveOptions::all();
        fused.fuse_statement_sequences = true;
        let mut unfused = AggressiveOptions::all();
        unfused.fuse_statement_sequences = false;

        let fused_result = golf_with_options(source, fused);
        let unfused_result = golf_with_options(source, unfused);

        assert!(
            estimate_budget(&fused_result.code).deflate_bytes
                <= estimate_budget(&unfused_result.code).deflate_bytes,
            "statement fusion must not inflate the DEFLATE estimate\nfused  : {}\nunfused: {}",
            fused_result.code,
            unfused_result.code
        );
        assert!(fused_result.stats.aggressive.statement_sequences_fused >= 1);
    }

    #[test]
    fn hoist_declarations_hoists_across_a_safe_gap() {
        // golf.md Phase 30.4: the gap statement touches neither `a` nor
        // `b`, so `b`'s declaration is free to relocate backward and merge
        // with `a`'s.
        let mut opts = AggressiveOptions::none();
        opts.hoist_declarations = true;
        let r = golf_with_protected_names(
            "void f(){float a=1.0;g=g+1.0;float b=2.0;}",
            opts,
            &["f".to_string(), "a".to_string(), "b".to_string(), "g".to_string()],
        );
        assert_eq!(r.code, "void f(){float a=1.,b=2.;g=g+1.;}");
        assert_eq!(r.stats.aggressive.declarations_merged, 1);
    }

    #[test]
    fn hoist_declarations_declines_when_the_gap_touches_the_anchor_declaration() {
        // golf.md Phase 30.4: the gap statement reads and writes `a`,
        // which is already referenced by the anchor declaration itself --
        // the conservative straight-line check must decline rather than
        // risk reordering that read/write around the hoisted declaration.
        let mut opts = AggressiveOptions::none();
        opts.hoist_declarations = true;
        let r = golf_with_protected_names(
            "void f(){float a=1.0;a=a+1.0;float b=2.0;}",
            opts,
            &["f".to_string(), "a".to_string(), "b".to_string()],
        );
        assert_eq!(r.code, "void f(){float a=1.;a=a+1.;float b=2.;}");
        assert_eq!(r.stats.aggressive.declarations_merged, 0);
    }

    #[test]
    fn hoist_declarations_declines_across_a_block_boundary() {
        // golf.md Phase 30.4: a `{`/`}` of any kind clears every pending
        // chain outright, even when the nested block declares nothing --
        // hoisting must never reach into or out of a different scope
        // depth.
        let mut opts = AggressiveOptions::none();
        opts.hoist_declarations = true;
        let r = golf_with_protected_names(
            "void f(){float a=1.0;{g=g+1.0;}float b=2.0;}",
            opts,
            &["f".to_string(), "a".to_string(), "b".to_string(), "g".to_string()],
        );
        assert_eq!(r.code, "void f(){float a=1.;{g=g+1.;}float b=2.;}");
        assert_eq!(r.stats.aggressive.declarations_merged, 0);
    }

    #[test]
    fn hoist_declarations_declines_when_a_later_declaration_depends_on_an_intervening_different_type_declaration() {
        // Regression test for a real user-reported shader (a raymarcher):
        // a later same-type declaration's initializer can depend on a name
        // introduced by an *intervening different-type* declaration, not
        // just an assignment statement -- hoisting it backward past that
        // declaration would reference the name before it exists ("used
        // before declared" GLSL that fails to compile). See roadmap.md,
        // "Correction critique #3, découverte le 2026-08-05".
        let mut opts = AggressiveOptions::none();
        opts.hoist_declarations = true;
        let r = golf_with_protected_names(
            "void f(){float a=1.0;vec3 c=vec3(2.0);float b=c.x;}",
            opts,
            &["f".to_string(), "a".to_string(), "b".to_string(), "c".to_string()],
        );
        assert_eq!(r.code, "void f(){float a=1.;vec3 c=vec3(2.);float b=c.x;}");
        assert_eq!(r.stats.aggressive.declarations_merged, 0);
    }

    #[test]
    fn hoist_declarations_still_merges_across_an_unrelated_different_type_declaration() {
        // The fix for the case directly above must not become so
        // conservative that it declines every same-type merge whenever a
        // different-type declaration merely appears in the gap -- only
        // when the later declaration actually depends on a name that
        // different-type declaration introduced.
        let mut opts = AggressiveOptions::none();
        opts.hoist_declarations = true;
        let r = golf_with_protected_names(
            "void f(){float a=1.0;vec3 c=vec3(2.0);float b=3.0;}",
            opts,
            &["f".to_string(), "a".to_string(), "b".to_string(), "c".to_string()],
        );
        assert_eq!(r.code, "void f(){float a=1.,b=3.;vec3 c=vec3(2.);}");
        assert_eq!(r.stats.aggressive.declarations_merged, 1);
    }

    #[test]
    fn hoist_declarations_merges_a_later_declaration_that_reads_the_chains_own_earlier_declarator() {
        // Regression test for a real user-reported shader: `p`'s initializer
        // reads `A`, which is the chain's own anchor declaration -- not some
        // unrelated intervening name. Merging is safe here because GLSL
        // evaluates a comma-declarator list left to right and hoisting
        // preserves `A`'s position ahead of `p`, so this must merge (unlike
        // the "intervening different-type declaration" case above, where the
        // referenced name is never moved along with it).
        let mut opts = AggressiveOptions::none();
        opts.hoist_declarations = true;
        let r = golf_with_protected_names(
            "void f(){vec2 A=r.rg;float b=t*.4;vec2 p=(FC.xy*2.-A)/A.y;}",
            opts,
            &["f".to_string(), "A".to_string(), "b".to_string(), "p".to_string(), "r".to_string(), "t".to_string(), "FC".to_string()],
        );
        assert_eq!(r.code, "void f(){vec2 A=r.xy,p=(FC.xy*2.-A)/A.y;float b=t*.4;}");
        assert_eq!(r.stats.aggressive.declarations_merged, 1);
    }

    #[test]
    fn hoist_declarations_never_worsens_deflate_budget_on_the_tracked_fixture() {
        let source = include_str!("../../fixtures/declaration_hoisting.glsl");
        let mut hoisted = AggressiveOptions::all();
        hoisted.hoist_declarations = true;
        let mut unhoisted = AggressiveOptions::all();
        unhoisted.hoist_declarations = false;

        let hoisted_result = golf_with_options(source, hoisted);
        let unhoisted_result = golf_with_options(source, unhoisted);

        assert!(
            estimate_budget(&hoisted_result.code).deflate_bytes
                <= estimate_budget(&unhoisted_result.code).deflate_bytes,
            "declaration hoisting must not inflate the DEFLATE estimate\nhoisted  : {}\nunhoisted: {}",
            hoisted_result.code,
            unhoisted_result.code
        );
        assert!(hoisted_result.stats.aggressive.declarations_merged >= 1);
    }
}

