// golf.md Phase 30.2 -- whole-shader cross-statement common subexpression
// elimination via `#define` macro extraction.
//
// `aggressive::eliminate_common_subexpressions` only matches whole
// declaration-statement initializers in an uninterrupted straight-line run
// (Phase 11 design, deliberately conservative). This module adds a second,
// separately toggled pass that finds a pure sub-expression (same purity
// whitelist as the existing pass -- identifiers, numbers, operators,
// member/swizzle access, and a fixed whitelist of pure builtins, **never**
// a user function, via the existing `aggressive::expr_is_pure`) repeated
// **verbatim** three or more times anywhere in the file -- not just inside
// one straight-line run, not just as a whole declaration initializer -- and
// extracts it to a `#define` macro when doing so is net-smaller under the
// active budget metric: both a raw-character comparison and an incremental,
// whole-buffer DEFLATE-estimate comparison (mirroring Phase 30.1's
// `inline_aggressive` commit gate, but measured against the actual
// accumulated buffer so far rather than an isolated fragment, since an
// isolated snippet's compression estimate is a poor proxy for its real
// contribution once embedded in the rest of the shader) must agree the
// extraction is smaller before a group is committed, since a `#define`
// line's one-time, unique-text overhead does not always amortize against
// the LZ77 back-references DEFLATE would otherwise have found in the
// un-extracted repeats, particularly on short shaders where the
// compression window has little prior context.
//
// Because a `#define` is pure textual substitution -- re-expanding to the
// exact original token sequence at every call site -- this pass never
// needs the substituted *value* to be identical the way a variable-based
// CSE pass would; a macro invocation simply reproduces, in place, tokens
// that were already valid at that exact position. The one scope-related
// danger that textual substitution genuinely cannot paper over is a
// *nested* re-declaration: extracting an expression from an outer scope
// when an occurrence's identifier is separately (and validly) redeclared
// in a scope nested *inside* another kept occurrence would still expand
// correctly at each individual site, but is exactly the shape this
// document's "never invent a new safety class" rule says to decline
// rather than reason further about. This pass therefore reuses the
// existing `golfer::block_scope_tree`/`innermost_scope`/`mutually_disjoint`
// machinery to require every kept occurrence of a group to resolve to
// either the exact same block as every other kept occurrence, or to a
// block pairwise mutually disjoint from it -- never partially nested --
// before the group is accepted. This is deliberately what lets the
// flagship case this phase targets -- the same pure expression (e.g.
// `dot(p,p)`) appearing verbatim in several *unrelated* functions, each
// with its own same-named local parameter -- extract cleanly: sibling
// function bodies are mutually disjoint scopes, so each occurrence's `p`
// keeps resolving, after extraction exactly as before, to that function's
// own parameter.

use crate::aggressive::{expr_is_pure, AggressiveStats, Item};
use crate::expr::{parse_expr, Expr, ExprKind};
use crate::golfer::{block_scope_tree, innermost_scope, mutually_disjoint, BlockScope};
use crate::lexer::Tok;
use crate::vocab::{builtin_functions, builtin_variables, keywords, protected_host_names};
use std::collections::HashSet;

struct Occurrence {
    start: usize,
    end: usize,
    expr: Expr,
}

fn unwrap_paren(e: &Expr) -> &Expr {
    match &e.kind {
        ExprKind::Paren(inner) => unwrap_paren(inner),
        _ => e,
    }
}

fn scope_ids_for_group(scopes: &[BlockScope], occs: &[&Occurrence]) -> Vec<usize> {
    let mut seen: Vec<usize> = Vec::new();
    for occ in occs {
        if let Some(idx) = innermost_scope(occ.start, scopes) {
            if !seen.contains(&idx) {
                seen.push(idx);
            }
        }
    }
    seen
}

fn overlaps(a: (usize, usize), b: (usize, usize)) -> bool {
    a.0 < b.1 && b.0 < a.1
}

fn pick_macro_name(used: &HashSet<String>) -> Option<String> {
    let kw = keywords();
    let builtins = builtin_functions();
    let builtin_vars = builtin_variables();
    let protected = protected_host_names();
    let is_free = |cand: &str| {
        !used.contains(cand)
            && !kw.contains(cand)
            && !builtins.contains(cand)
            && !builtin_vars.contains(cand)
            && !protected.contains(cand)
    };
    for c in ('A'..='Z').chain('a'..='z') {
        let cand = c.to_string();
        if is_free(&cand) {
            return Some(cand);
        }
    }
    for c1 in 'A'..='Z' {
        for c2 in ('A'..='Z').chain('0'..='9') {
            let cand = format!("{c1}{c2}");
            if is_free(&cand) {
                return Some(cand);
            }
        }
    }
    None
}

fn char_len(items: &[Item], start: usize, end: usize) -> usize {
    items[start..end].iter().map(|it| it.text.chars().count()).sum()
}

fn collect_pure_occurrences(e: &Expr, out: &mut Vec<Occurrence>) {
    if e.end > e.start + 1 && !matches!(unwrap_paren(e).kind, ExprKind::Ident(_) | ExprKind::Number(_)) && expr_is_pure(e) {
        out.push(Occurrence {
            start: e.start,
            end: e.end,
            expr: e.clone(),
        });
    }
    match &e.kind {
        ExprKind::Unary(_, inner) | ExprKind::Paren(inner) => collect_pure_occurrences(inner, out),
        ExprKind::Binary(_, l, r) => {
            collect_pure_occurrences(l, out);
            collect_pure_occurrences(r, out);
        }
        ExprKind::Ternary(c, t, f) => {
            collect_pure_occurrences(c, out);
            collect_pure_occurrences(t, out);
            collect_pure_occurrences(f, out);
        }
        ExprKind::Call(_, args) => {
            for a in args {
                collect_pure_occurrences(a, out);
            }
        }
        ExprKind::Index(b, idx) => {
            collect_pure_occurrences(b, out);
            collect_pure_occurrences(idx, out);
        }
        ExprKind::Member(b, _) => collect_pure_occurrences(b, out),
        ExprKind::Number(_) | ExprKind::Ident(_) => {}
    }
}

/// golf.md Phase 30.2 -- see module documentation above.
///
/// `compression_budget` mirrors Phase 30.1's own documented rule ("raw
/// and, when the budget preset is compression-based, DEFLATE-estimated")
/// -- the raw-character gate below always applies, but the incremental,
/// whole-buffer DEFLATE gate is only meaningful when the active budget
/// preset actually tracks compressed size (a `deflate_limit`, e.g. the
/// "4KB intro"/"8KB intro" presets in `budget::presets`), as opposed to a
/// raw-character-limit preset (e.g. "Shadertoy"/"X/Twitter shader"), where
/// only the character count matters and gating on DEFLATE would wrongly
/// decline an extraction that is a pure win under the metric that
/// actually applies. Callers pass `true` here when the active preset has
/// a `deflate_limit`, `false` otherwise (or when no preset is active).
pub fn eliminate_macro_common_subexpressions(items: Vec<Item>, stats: &mut AggressiveStats, compression_budget: bool) -> Vec<Item> {
    if items.is_empty() {
        return items;
    }

    let tokens: Vec<Tok> = items.iter().map(|it| it.tok.clone()).collect();
    let scopes = block_scope_tree(&tokens);

    // Every token position is tried as a parse start (matching this pass's
    // original, deliberately cheap scanning strategy), but the *maximal*
    // parse at each position is then walked recursively so a repeated pure
    // sub-expression is found even when it is combined with an operator at
    // one or more of its occurrences -- e.g. `dot(p,p)` inside
    // `dot(p,p)+1.` -- rather than only ever matching a bare, top-level
    // occurrence. `collect_pure_occurrences` applies the exact same
    // bare-identifier/number and purity filters this pass has always used,
    // just per-node instead of only at the outermost node.
    let mut occs: Vec<Occurrence> = Vec::new();
    for i in 0..items.len() {
        if matches!(items[i].tok, Tok::Preproc(_)) {
            continue;
        }
        let Some(expr) = parse_expr(&items, i) else {
            continue;
        };
        collect_pure_occurrences(&expr, &mut occs);
    }
    let mut groups: Vec<Vec<usize>> = Vec::new();
    'outer: for idx in 0..occs.len() {
        for g in groups.iter_mut() {
            if occs[g[0]].expr.structurally_eq(&occs[idx].expr) {
                g.push(idx);
                continue 'outer;
            }
        }
        groups.push(vec![idx]);
    }

    let mut used_names: HashSet<String> = items
        .iter()
        .filter_map(|it| if let Tok::Ident(n) = &it.tok { Some(n.clone()) } else { None })
        .collect();

    let mut edits: Vec<(usize, usize, Vec<Item>)> = Vec::new();
    let mut macro_defs: Vec<String> = Vec::new();
    let mut extracted = 0usize;
    let mut committed: Vec<(usize, usize)> = Vec::new();

    // Assembles a candidate output from the original `items` plus a given
    // set of already-decided edits/macro-def lines, so each group below can
    // be measured against the *actual* accumulated file so far rather than
    // an isolated fragment -- an isolated `before`/`after` snippet compresses
    // very differently from the same text in its real surrounding context
    // (a short repeated expression alone looks like pure win-by-back-
    // reference, but once it sits among a shader's other tokens the
    // DEFLATE estimate can move either way), so only a whole-buffer
    // comparison is a reliable proxy for this pass's own "never regress
    // the DEFLATE estimate" invariant (`golf.md` Phase 30's corollary,
    // shared with Phase 30.1's `inline_aggressive`).
    fn assemble(items: &[Item], edits: &[(usize, usize, Vec<Item>)], macro_defs: &[String]) -> Vec<Item> {
        let mut sorted = edits.to_vec();
        sorted.sort_by_key(|e| e.0);
        let mut out = Vec::with_capacity(items.len());
        let mut i = 0usize;
        let mut edit_iter = sorted.into_iter().peekable();
        while i < items.len() {
            if let Some((s, _, _)) = edit_iter.peek() {
                if *s == i {
                    let (_, e, repl) = edit_iter.next().unwrap();
                    out.extend(repl);
                    i = e;
                    continue;
                }
            }
            out.push(items[i].clone());
            i += 1;
        }
        let mut result = Vec::with_capacity(out.len() + macro_defs.len());
        for line in macro_defs {
            result.push(Item {
                tok: Tok::Preproc(line.clone()),
                text: line.clone(),
                space_before: false,
            });
        }
        result.extend(out);
        result
    }

    // The whole-buffer DEFLATE baseline is only needed when the incremental
    // gate below is actually going to run.
    let mut baseline_deflate = if compression_budget {
        crate::budget::estimate_budget(&crate::golfer::layout(&items)).deflate_bytes
    } else {
        0
    };

    for g in &groups {
        if g.len() < 3 {
            continue;
        }
        let mut kept: Vec<&Occurrence> = Vec::new();
        for &oi in g {
            let occ = &occs[oi];
            if committed.iter().any(|c| overlaps(*c, (occ.start, occ.end))) {
                continue;
            }
            if kept.iter().any(|k| overlaps((k.start, k.end), (occ.start, occ.end))) {
                continue;
            }
            kept.push(occ);
        }
        if kept.len() < 3 {
            continue;
        }

        let scope_ids = scope_ids_for_group(&scopes, &kept);
        if scope_ids.len() > 1 && !mutually_disjoint(&scope_ids, &scopes) {
            continue;
        }

        let Some(name) = pick_macro_name(&used_names) else {
            continue;
        };

        let rep = kept[0];
        let body_text: String = items[rep.start..rep.end].iter().map(|it| it.text.as_str()).collect();
        let define_line = format!("#define {name} {body_text}");

        let before: usize = kept.iter().map(|o| char_len(&items, o.start, o.end)).sum();
        let after: usize = kept.len() * name.chars().count() + define_line.chars().count();
        if after >= before {
            continue;
        }

        // Budget-preset-aware check, mirroring `inline_aggressive`'s (Phase
        // 30.1) own commit gate: a raw-character win can still regress the
        // DEFLATE-estimated size, since a `#define` line's one-time,
        // unique-text overhead is not always amortized by the LZ77
        // back-references DEFLATE would otherwise have found in the
        // un-extracted repeats -- particularly on short shaders where the
        // window of prior context is small. This only matters, though,
        // when the active budget preset actually tracks compressed size
        // (`compression_budget`); for a raw-character-limit preset (or no
        // preset at all), the raw gate above is already the correct and
        // only metric, so the DEFLATE estimate -- which reflects how well
        // an *already-repeating* string compresses via LZ77 on its own,
        // and can therefore make a `#define`'s one-time overhead look like
        // a regression even when it strictly shrinks the raw byte count --
        // is skipped entirely rather than allowed to veto a genuine
        // raw-metric win. Measured against the whole buffer accumulated so
        // far (see `assemble` above), not an isolated fragment, and applied
        // incrementally so an earlier group's accepted win becomes the new
        // baseline for the next.
        if compression_budget {
            let mut candidate_edits = edits.clone();
            for o in &kept {
                candidate_edits.push((
                    o.start,
                    o.end,
                    vec![Item {
                        tok: Tok::Ident(name.clone()),
                        text: name.clone(),
                        space_before: items[o.start].space_before,
                    }],
                ));
            }
            let mut candidate_defs = macro_defs.clone();
            candidate_defs.push(define_line.clone());
            let candidate_deflate = crate::budget::estimate_budget(&crate::golfer::layout(&assemble(&items, &candidate_edits, &candidate_defs))).deflate_bytes;
            if candidate_deflate >= baseline_deflate {
                continue;
            }
            baseline_deflate = candidate_deflate;
        }

        used_names.insert(name.clone());
        for o in &kept {
            committed.push((o.start, o.end));
            edits.push((
                o.start,
                o.end,
                vec![Item {
                    tok: Tok::Ident(name.clone()),
                    text: name.clone(),
                    space_before: items[o.start].space_before,
                }],
            ));
        }
        macro_defs.push(define_line);
        extracted += kept.len();
    }

    if edits.is_empty() {
        return items;
    }

    let result = assemble(&items, &edits, &macro_defs);

    // When `compression_budget` is set, each accepted group above was
    // already gated, incrementally, on the whole-buffer DEFLATE estimate
    // (see `assemble`/`baseline_deflate` above), so `result`'s DEFLATE
    // estimate is guaranteed to be no larger than `items`'s own -- no
    // further whole-file check is needed here. When it is not set, only
    // the raw-character gate applies, matching the active (non-
    // compression-based) budget metric.
    stats.common_subexpressions_eliminated += extracted;
    result
}

#[cfg(test)]
mod tests {
    use crate::golfer::{golf_with_protected_names, AggressiveOptions};

    /// Raw-character-only mode (`macro_cse_compression_budget: false`,
    /// `AggressiveOptions::none()`'s own default) -- i.e. what a caller
    /// targeting a raw-character-limit preset (or no preset at all) gets.
    /// The DEFLATE-safety invariant itself is covered separately by
    /// `macro_cse_never_worsens_deflate_budget_on_the_tracked_fixture` and
    /// `declines_a_short_repeat_under_a_compression_based_budget` below,
    /// both of which opt into `macro_cse_compression_budget: true`
    /// explicitly.
    fn opts() -> AggressiveOptions {
        let mut o = AggressiveOptions::none();
        o.macro_cse = true;
        o
    }

    #[test]
    fn extracts_a_pure_expression_repeated_three_times_across_sibling_functions() {
        let src = "float f(vec2 p){return dot(p-vec2(.5,.5),p-vec2(.5,.5))+1.;}float g(vec2 p){return dot(p-vec2(.5,.5),p-vec2(.5,.5))*2.;}void mainImage(out vec4 c,in vec2 p){c=vec4(dot(p-vec2(.5,.5),p-vec2(.5,.5)));}";
        let r = golf_with_protected_names(src, opts(), &["p".to_string()]);
        assert!(r.code.starts_with("#define "), "expected a macro definition, got: {}", r.code);
        assert_eq!(
            r.code.matches("dot(p-vec2(.5,.5),p-vec2(.5,.5))").count(),
            1,
            "expected the repeated expression to survive only inside the macro body -- sibling function bodies are mutually disjoint scopes, so this must extract cleanly, got: {}",
            r.code
        );
    }

    #[test]
    fn declines_when_repeated_only_twice() {
        let src = "float f(vec2 p){return dot(p,p)+1.;}void mainImage(out vec4 c,in vec2 p){c=vec4(dot(p,p));}";
        let r = golf_with_protected_names(src, opts(), &["p".to_string()]);
        assert!(!r.code.contains("#define"), "must not extract a two-occurrence expression, got: {}", r.code);
    }

    #[test]
    fn declines_an_expression_nested_inside_the_scope_of_another_kept_occurrence() {
        // The first occurrence lives directly in `mainImage`'s own scope;
        // the other two live inside a nested `if` block. That nested block's
        // scope is *contained in*, not disjoint from, `mainImage`'s scope,
        // so the group must be declined even though all three occurrences
        // are textually identical and individually pure.
        let src = "void mainImage(out vec4 c,in vec2 p){float a=dot(p,p);if(p.x>0.){float b=dot(p,p);float d=dot(p,p);c=vec4(a+b+d);}}";
        let r = golf_with_protected_names(src, opts(), &["p".to_string()]);
        assert!(!r.code.contains("#define"), "a nested-scope occurrence group must decline extraction, got: {}", r.code);
    }

    #[test]
    fn never_extracts_a_bare_identifier_or_number() {
        let src = "void mainImage(out vec4 c,in vec2 p){float a=1.,b=1.,d=1.;c=vec4(a,b,d,1.);}";
        let r = golf_with_protected_names(src, opts(), &["p".to_string()]);
        assert!(!r.code.contains("#define"), "a bare number/identifier must never be extracted to a macro, got: {}", r.code);
    }

    #[test]
    fn declines_a_short_repeat_under_a_compression_based_budget() {
        // Same 3x flagship case as
        // `extracts_a_pure_expression_repeated_three_times_across_sibling_functions`
        // above, but with `macro_cse_compression_budget: true` -- i.e. as if
        // targeting a compression-based preset (`budget::presets`' "4KB
        // intro"-style entries). DEFLATE already captures this short,
        // 3-times-repeated expression cheaply via LZ77 back-references, so
        // the `#define` line's one-time, unique-text overhead is *not*
        // amortized here and the whole-buffer DEFLATE gate must correctly
        // veto the extraction that the raw-character-only gate alone would
        // have accepted, confirming the two modes genuinely differ.
        let src = "float f(vec2 p){return dot(p-vec2(.5,.5),p-vec2(.5,.5))+1.;}float g(vec2 p){return dot(p-vec2(.5,.5),p-vec2(.5,.5))*2.;}void mainImage(out vec4 c,in vec2 p){c=vec4(dot(p-vec2(.5,.5),p-vec2(.5,.5)));}";
        let mut o = opts();
        o.macro_cse_compression_budget = true;
        let r = golf_with_protected_names(src, o, &["p".to_string()]);
        assert!(
            !r.code.contains("#define"),
            "a compression-based budget must decline an extraction that regresses the DEFLATE estimate, got: {}",
            r.code
        );
    }

    #[test]
    fn macro_cse_never_worsens_deflate_budget_on_the_tracked_fixture() {
        // golf.md Phase 30.2's own tracked fixture: a raymarcher-style
        // shader reusing `dot(p,p)`-class expressions across `mapA`,
        // `mapB`, and `mainImage` -- three mutually disjoint sibling
        // scopes, each with its own same-named `p`. Explicitly opts into
        // `macro_cse_compression_budget: true` since this test's whole
        // point is to exercise the DEFLATE-safety invariant.
        use crate::budget::estimate_budget;
        use crate::golfer::golf_with_options;

        let source = include_str!("../../fixtures/macro_cse.glsl");
        let mut extracted = crate::golfer::AggressiveOptions::all();
        extracted.macro_cse = true;
        extracted.macro_cse_compression_budget = true;
        let unextracted = crate::golfer::AggressiveOptions::all();

        let extracted_result = golf_with_options(source, extracted);
        let unextracted_result = golf_with_options(source, unextracted);

        assert!(
            estimate_budget(&extracted_result.code).deflate_bytes
                <= estimate_budget(&unextracted_result.code).deflate_bytes,
            "macro CSE must not inflate the DEFLATE estimate\nextracted  : {}\nunextracted: {}",
            extracted_result.code,
            unextracted_result.code
        );
    }
}

