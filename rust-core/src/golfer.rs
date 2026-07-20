use crate::aggressive::{
    compound_assignments, eliminate_common_subexpressions, eliminate_dead_functions,
    eliminate_dead_locals, eliminate_dead_stores, fold_additive_constants,
    fold_additive_float_constants, fold_constants, fold_float_constants, increment_decrement,
    merge_declarations, reduce_constant_vectors, shortest_scientific_form,
    simplify_algebraic_identities, strip_duplicate_precision, strip_redundant_braces,
    strip_redundant_parens, strip_trailing_void_return, ternary_from_if_else, AggressiveStats, Item,
};
use crate::lexer::{tokenize_spaced, Tok};
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

struct BlockScope {
    open: usize,
    close: usize,
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

fn block_scope_tree(tokens: &[Tok]) -> Vec<BlockScope> {
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

fn innermost_scope(pos: usize, scopes: &[BlockScope]) -> Option<usize> {
    scopes
        .iter()
        .enumerate()
        .filter(|(_, s)| pos > s.open && pos < s.close)
        .max_by_key(|(_, s)| s.open)
        .map(|(idx, _)| idx)
}

fn mutually_disjoint(indices: &[usize], scopes: &[BlockScope]) -> bool {
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
    let renamable_set: HashSet<&str> = renamable.iter().map(|(name, _)| name.as_str()).collect();
    for tok in &tokens {
        if let Tok::Ident(name) = tok {
            if !renamable_set.contains(name.as_str()) {
                taken.insert(name.clone());
            }
        }
    }
    taken.extend(preproc_referenced_names(&tokens));

    let block_scopes = block_scope_tree(&tokens);
    let mut local_taken: HashMap<usize, HashSet<String>> = HashMap::new();
    let mut rename_map: HashMap<String, String> = HashMap::new();
    for (original, scope) in &renamable {
        let mut gen = NameGen::new();
        loop {
            let candidate = gen.next().unwrap();
            let collides = taken.contains(&candidate)
                || match scope {
                    Scope::Local(indices) => local_taken.iter().any(|(other_idx, names)| {
                        names.contains(&candidate)
                            && indices
                                .iter()
                                .any(|idx| !mutually_disjoint(&[*idx, *other_idx], &block_scopes))
                    }),
                    Scope::Global => local_taken.values().any(|s| s.contains(&candidate)),
                };
            if collides {
                continue;
            }
            match scope {
                Scope::Global => {
                    taken.insert(candidate.clone());
                }
                Scope::Local(indices) => {
                    for idx in indices {
                        local_taken.entry(*idx).or_default().insert(candidate.clone());
                    }
                }
            }
            rename_map.insert(original.clone(), candidate);
            break;
        }
    }

    let mut numbers_shortened = 0usize;
    let mut items: Vec<Item> = Vec::with_capacity(tokens.len());

    for (idx, tok) in tokens.iter().enumerate() {
        let preceded_by_dot = idx > 0 && matches!(tokens[idx - 1], Tok::Punct('.'));
        let text = match tok {
            Tok::Ident(name) if preceded_by_dot => name.clone(),
            Tok::Ident(name) => rename_map.get(name).cloned().unwrap_or_else(|| name.clone()),
            Tok::Number(raw) => {
                let shortened = shorten_number(raw);
                if shortened != *raw {
                    numbers_shortened += 1;
                }
                shortened
            }
            Tok::Punct(c) => c.to_string(),
            Tok::Preproc(_) => String::new(),
        };
        items.push(Item {
            tok: tok.clone(),
            text,
            space_before: had_space[idx],
        });
    }

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

fn layout(items: &[Item]) -> String {
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
    use super::golf_with_protected_names;
    use super::golf_with_protected_names_traced;
    use super::AggressiveOptions;

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
        // confirming the fixpoint. Sixteen passes per iteration, in the
        // exact order golf_with_protected_names_impl invokes them.
        let expected: [(&str, usize); 32] = [
            ("eliminate_dead_locals", 1),
            ("eliminate_dead_stores", 0),
            ("eliminate_dead_functions", 0),
            ("inline_single_call_functions", 0),
            ("fold_constants", 1),
            ("reduce_constant_vectors", 0),
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
        assert!(trace.steps.is_empty());
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
        assert!(trace.steps.iter().all(|s| s.pass_name == "eliminate_dead_locals"));
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
        let r = golf("void f(float w){vec3 a=vec3(w,w,w);}", true);
        assert_eq!(r.code, "void b(float a){vec3 c=vec3(a,a,a);}");
        assert_eq!(r.stats.aggressive.constant_vectors_reduced, 0);
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
}
