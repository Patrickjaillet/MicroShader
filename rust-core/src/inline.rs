use crate::aggressive::{
    classify_fusable_statement, is_statement_boundary, skip_balanced, AggressiveStats, FusableStmt, Item,
};
use crate::budget::estimate_budget;
use crate::callgraph::{find_function_definitions, CallGraph, FunctionDef};
use crate::expr::{parse_arg_list, parse_expr, Expr, ExprKind};
use crate::lexer::Tok;
use crate::vocab::{keywords, type_keywords};
use std::collections::HashMap;
use std::collections::HashSet;

fn char_len(items: &[Item]) -> usize {
    items.iter().map(|it| it.text.chars().count()).sum()
}

fn is_safe_arg(items: &[Item], e: &Expr) -> bool {
    match &e.kind {
        ExprKind::Number(_) | ExprKind::Ident(_) => true,
        ExprKind::Unary(_, inner) => is_safe_arg(items, inner),
        // A pure component/swizzle access chain rooted in a bare operand
        // (e.g. `p.x`, `p.xy`, `p.xy.x`) has no side effects and evaluates
        // to the same value every time, so it is exactly as safe to
        // duplicate across substitution sites as the bare identifier it is
        // built from -- this is required for golf.md Phase 30.1's own
        // fixtures (`tally(p.x)`), which pass a swizzled component rather
        // than a pre-extracted local.
        ExprKind::Member(inner, _) => is_safe_arg(items, inner),
        _ => {
            let _ = items;
            false
        }
    }
}

fn is_primary_level(e: &Expr) -> bool {
    matches!(
        e.kind,
        ExprKind::Number(_) | ExprKind::Ident(_) | ExprKind::Call(_, _) | ExprKind::Index(_, _) | ExprKind::Member(_, _) | ExprKind::Paren(_)
    )
}

struct Param {
    name: String,
    disallowed: bool,
}

fn parse_params(items: &[Item], open_paren: usize) -> Option<(Vec<Param>, usize)> {
    let close_paren = skip_balanced(items, open_paren, '(', ')')? - 1;
    let kw = keywords();
    let type_kw = type_keywords();
    let mut params = Vec::new();
    let mut i = open_paren + 1;
    if i == close_paren {
        return Some((params, close_paren));
    }
    loop {
        let mut disallowed = false;
        while let Some(Tok::Ident(w)) = items.get(i).map(|it| &it.tok) {
            if !kw.contains(w.as_str()) || type_kw.contains(w.as_str()) {
                break;
            }
            if w == "out" || w == "inout" {
                disallowed = true;
            }
            i += 1;
        }
        match items.get(i).map(|it| &it.tok) {
            Some(Tok::Ident(w)) if type_kw.contains(w.as_str()) => i += 1,
            _ => return None,
        }
        let name = match items.get(i).map(|it| &it.tok) {
            Some(Tok::Ident(n)) => n.clone(),
            _ => return None,
        };
        i += 1;
        if matches!(items.get(i).map(|it| &it.tok), Some(Tok::Punct('['))) {
            disallowed = true;
            i = skip_balanced(items, i, '[', ']')?;
        }
        params.push(Param { name, disallowed });
        match items.get(i).map(|it| &it.tok) {
            Some(Tok::Punct(',')) => i += 1,
            Some(Tok::Punct(')')) if i == close_paren => break,
            _ => return None,
        }
    }
    Some((params, close_paren))
}

fn parse_single_return_body(items: &[Item], open_brace: usize, body_close: usize) -> Option<Expr> {
    if items.get(open_brace + 1).map(|it| it.text.as_str()) != Some("return") {
        return None;
    }
    let expr = parse_expr(items, open_brace + 2)?;
    if !matches!(items.get(expr.end).map(|it| &it.tok), Some(Tok::Punct(';'))) {
        return None;
    }
    if expr.end + 1 != body_close {
        return None;
    }
    Some(expr)
}

struct CallSite {
    name_start: usize,
    end: usize,
    args: Vec<Expr>,
}

fn find_call_sites(items: &[Item], def: &FunctionDef) -> Option<Vec<CallSite>> {
    let mut sites = Vec::new();
    let mut i = 0;
    while i < items.len() {
        let is_own_signature = i == def.def_start + 1;
        let is_within_own_body = i >= def.def_start && i <= def.body_close;
        let matches_name = matches!(items.get(i).map(|it| &it.tok), Some(Tok::Ident(n)) if n == &def.name);
        if matches_name && !is_own_signature && !is_within_own_body && matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Punct('('))) {
            let close = skip_balanced(items, i + 1, '(', ')')?;
            let args = parse_arg_list(items, i + 2, close - 1)?;
            sites.push(CallSite { name_start: i, end: close, args });
        }
        i += 1;
    }
    Some(sites)
}

fn substitute_params(items: &[Item], expr_start: usize, expr_end: usize, params: &[Param], args: &[Expr]) -> Vec<Item> {
    let mut out = Vec::new();
    let mut i = expr_start;
    while i < expr_end {
        if let Tok::Ident(name) = &items[i].tok {
            if let Some(pos) = params.iter().position(|p| &p.name == name) {
                let mut arg_tokens: Vec<Item> = items[args[pos].start..args[pos].end].to_vec();
                if let Some(first) = arg_tokens.first_mut() {
                    first.space_before = true;
                }
                out.extend(arg_tokens);
                i += 1;
                continue;
            }
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn substitute_expr(items: &[Item], expr: &Expr, params: &[Param], args: &[Expr]) -> Vec<Item> {
    substitute_params(items, expr.start, expr.end, params, args)
}

struct Edit {
    start: usize,
    end: usize,
    replacement: Vec<Item>,
}

fn overlaps(a: (usize, usize), b: (usize, usize)) -> bool {
    a.0 < b.1 && b.0 < a.1
}

pub fn inline_single_call_functions(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let defs = find_function_definitions(&items);
    if defs.is_empty() {
        return items;
    }

    let mut name_counts: HashMap<&str, usize> = HashMap::new();
    for d in &defs {
        *name_counts.entry(d.name.as_str()).or_insert(0) += 1;
    }
    let names = defs.iter().map(|d| d.name.clone()).collect();
    let graph = CallGraph::build(&items, &defs, &names);

    let mut edits: Vec<Edit> = Vec::new();

    for def in &defs {
        if def.name == "main" || def.name == "mainImage" {
            continue;
        }
        if name_counts.get(def.name.as_str()).copied().unwrap_or(0) != 1 {
            continue;
        }
        if graph.total_calls_to(&def.name) != 1 {
            continue;
        }
        let Some(sites) = find_call_sites(&items, def) else {
            continue;
        };
        let [site] = sites.as_slice() else {
            continue;
        };

        let open_paren = def.def_start + 2;
        let Some((params, close_paren)) = parse_params(&items, open_paren) else {
            continue;
        };
        if params.iter().any(|p| p.disallowed) {
            continue;
        }
        if params.len() != site.args.len() {
            continue;
        }
        if !site.args.iter().all(|a| is_safe_arg(&items, a)) {
            continue;
        }
        let open_brace = close_paren + 1;
        if !matches!(items.get(open_brace).map(|it| &it.tok), Some(Tok::Punct('{'))) {
            continue;
        }
        let Some(return_expr) = parse_single_return_body(&items, open_brace, def.body_close) else {
            continue;
        };

        let declaration_cost = char_len(&items[def.def_start..=def.body_close]);
        let call_site_cost = char_len(&items[site.name_start..site.end]);
        let before_cost = declaration_cost + call_site_cost;

        let mut substituted = substitute_expr(&items, &return_expr, &params, &site.args);
        let needs_wrap = !is_primary_level(&return_expr);
        let after_cost = char_len(&substituted) + if needs_wrap { 2 } else { 0 };

        if after_cost >= before_cost {
            continue;
        }

        let decl_range = (def.def_start, def.body_close + 1);
        let call_range = (site.name_start, site.end);
        if edits.iter().any(|e| overlaps((e.start, e.end), decl_range) || overlaps((e.start, e.end), call_range)) {
            continue;
        }

        if let Some(first) = substituted.first_mut() {
            first.space_before = true;
        }
        let mut replacement = Vec::with_capacity(substituted.len() + 2);
        if needs_wrap {
            replacement.push(Item { tok: Tok::Punct('('), text: "(".to_string(), space_before: true });
            replacement.append(&mut substituted);
            replacement.push(Item { tok: Tok::Punct(')'), text: ")".to_string(), space_before: false });
        } else {
            replacement = substituted;
        }

        stats.functions_inlined += 1;
        edits.push(Edit { start: decl_range.0, end: decl_range.1, replacement: Vec::new() });
        edits.push(Edit { start: site.name_start, end: site.end, replacement: replacement.clone() });
    }

    if edits.is_empty() {
        return items;
    }
    edits.sort_by_key(|e| e.start);

    let mut out = Vec::with_capacity(items.len());
    let mut i = 0;
    let mut edit_iter = edits.into_iter().peekable();
    while i < items.len() {
        if let Some(edit) = edit_iter.peek() {
            if edit.start == i {
                let edit = edit_iter.next().unwrap();
                out.extend(edit.replacement);
                i = edit.end;
                continue;
            }
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

struct StmtCallSite {
    name_start: usize,
    stmt_end: usize,
    args: Vec<Expr>,
}

/// Finds call sites of `def` that are themselves a full statement, i.e.
/// `name(args);` with nothing else on either side -- the only shape
/// `inline_aggressive` (golf.md Phase 30.1) knows how to substitute a
/// multi-statement body into, since a sequence of statements cannot be
/// dropped into an arbitrary expression position the way a single
/// `return`-expression body can. Declines the same "own signature / own
/// body" positions `find_call_sites` above already excludes, for the same
/// self-recursion-safety reason.
fn find_statement_call_sites(items: &[Item], def: &FunctionDef) -> Option<Vec<StmtCallSite>> {
    let mut sites = Vec::new();
    let mut i = 0;
    while i < items.len() {
        let is_own_signature = i == def.def_start + 1;
        let is_within_own_body = i >= def.def_start && i <= def.body_close;
        let matches_name = matches!(items.get(i).map(|it| &it.tok), Some(Tok::Ident(n)) if n == &def.name);
        if matches_name
            && !is_own_signature
            && !is_within_own_body
            && matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Punct('(')))
            && is_statement_boundary(items, i)
        {
            let close = skip_balanced(items, i + 1, '(', ')')?;
            if matches!(items.get(close).map(|it| &it.tok), Some(Tok::Punct(';'))) {
                let args = parse_arg_list(items, i + 2, close - 1)?;
                sites.push(StmtCallSite {
                    name_start: i,
                    stmt_end: close + 1,
                    args,
                });
            }
        }
        i += 1;
    }
    Some(sites)
}

/// Recognizes a function body (the span strictly between its `{` and `}`)
/// as a straight-line sequence of "pure-or-safe assignment expression"
/// statements -- reusing `classify_fusable_statement`, the exact same
/// assignment / increment-decrement / call-statement classifier Phase
/// 30.3's `fuse_statement_sequences` already relies on -- requiring the
/// classification to consume the body *exactly*, with nothing left over.
/// A body containing a declaration, `if`/`for`/`while`/`return`, or any
/// statement shape that classifier doesn't recognize fails this check and
/// is left to the existing single-return path (or left uninlined
/// entirely), matching golf.md Phase 30.1's explicit restriction.
fn classify_full_body(items: &[Item], body_start: usize, body_close: usize) -> Option<Vec<FusableStmt>> {
    if body_start >= body_close {
        return None;
    }
    let mut stmts = Vec::new();
    let mut i = body_start;
    while i < body_close {
        let stmt = classify_fusable_statement(items, i)?;
        i = stmt.stmt_end;
        stmts.push(stmt);
    }
    if i != body_close {
        return None;
    }
    Some(stmts)
}

/// `golf.md` Phase 30.1 -- aggressive multi-call-site, multi-statement
/// function inlining. Unlike `inline_single_call_functions` above (which is
/// restricted to exactly one call site and a single-`return`-expression
/// body), this pass inlines a function called from any number of
/// call-as-statement sites, as long as its entire body is a straight-line
/// run of assignment/increment-decrement/call statements (see
/// `classify_full_body`) -- e.g. a helper that mutates globals through a
/// short sequence of statements, called for its side effects rather than
/// its return value.
///
/// Every call site is substituted independently (parameters re-substituted
/// per site via the existing `substitute_params`), and the whole rewrite is
/// only committed when the total raw-character size after inlining is
/// strictly smaller than keeping the function declaration and every call
/// site as they were -- inlining a function with several call sites can
/// make output larger once its body passes a small size, unlike the
/// always-beneficial single-call-site case, so this pass measures before
/// committing rather than assuming monotonic improvement. `stats` is only
/// updated, and the rewrite only applied, once that measurement passes.
///
/// Never inlines `main`/`mainImage`, a function with an `out`/`inout` or
/// array parameter (`Param::disallowed`, the same restriction
/// `inline_single_call_functions` already enforces), or a function that is
/// recursive or mutually recursive with another function (via
/// `CallGraph::is_recursive`, reusing the existing call-graph construction
/// from `callgraph.rs` rather than re-deriving cycle detection here) --
/// substituting a recursive function's body at its call sites would either
/// loop forever or require unbounded expansion.
pub fn inline_aggressive(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let defs = find_function_definitions(&items);
    if defs.is_empty() {
        return items;
    }
    let names: HashSet<String> = defs.iter().map(|d| d.name.clone()).collect();
    let graph = CallGraph::build(&items, &defs, &names);

    // Every candidate below is evaluated against the *original*, unmutated
    // `items`, and all accepted rewrites are recorded as `Edit`s and applied
    // together in a single pass at the end (exactly like
    // `inline_single_call_functions` above) -- mutating `items` between
    // candidates would invalidate every other candidate's already-computed
    // token indices.
    let mut edits: Vec<Edit> = Vec::new();
    let mut inlined_count = 0usize;

    for def in &defs {
        if def.name == "main" || def.name == "mainImage" {
            continue;
        }
        if graph.is_recursive(&def.name) {
            continue;
        }

        let open_paren = def.def_start + 2;
        let Some((params, close_paren)) = parse_params(&items, open_paren) else {
            continue;
        };
        if params.iter().any(|p| p.disallowed) {
            continue;
        }
        let open_brace = close_paren + 1;
        if !matches!(items.get(open_brace).map(|it| &it.tok), Some(Tok::Punct('{'))) {
            continue;
        }
        let Some(body_stmts) = classify_full_body(&items, open_brace + 1, def.body_close) else {
            continue;
        };
        if body_stmts.is_empty() {
            continue;
        }

        let Some(sites) = find_statement_call_sites(&items, def) else {
            continue;
        };
        if sites.is_empty() {
            continue;
        }
        if sites.iter().any(|s| s.args.len() != params.len()) {
            continue;
        }
        if !sites.iter().all(|s| s.args.iter().all(|a| is_safe_arg(&items, a))) {
            continue;
        }

        let declaration_cost = char_len(&items[def.def_start..=def.body_close]);
        let call_sites_cost: usize = sites.iter().map(|s| char_len(&items[s.name_start..s.stmt_end])).sum();
        let before_cost = declaration_cost + call_sites_cost;

        let mut substitutions: Vec<Vec<Item>> = Vec::with_capacity(sites.len());
        let mut after_cost = 0usize;
        for site in &sites {
            let mut subst: Vec<Item> = Vec::new();
            for bstmt in &body_stmts {
                let mut sub_items = substitute_params(&items, bstmt.start, bstmt.expr_end, &params, &site.args);
                if let Some(first) = sub_items.first_mut() {
                    first.space_before = true;
                }
                subst.append(&mut sub_items);
                subst.push(Item {
                    tok: Tok::Punct(';'),
                    text: ";".to_string(),
                    space_before: false,
                });
            }
            after_cost += char_len(&subst);
            substitutions.push(subst);
        }

        if after_cost >= before_cost {
            continue;
        }

        // Budget-preset-aware check: also require the change not to
        // regress the DEFLATE-estimated size of the two touched regions,
        // since a raw-character win can occasionally still lose under
        // compression once a call site is duplicated several times.
        let before_text: String = items[def.def_start..=def.body_close]
            .iter()
            .chain(sites.iter().flat_map(|s| items[s.name_start..s.stmt_end].iter()))
            .map(|it| it.text.as_str())
            .collect();
        let after_text: String = substitutions.iter().flat_map(|s| s.iter()).map(|it| it.text.as_str()).collect();
        if estimate_budget(&after_text).deflate_bytes >= estimate_budget(&before_text).deflate_bytes {
            continue;
        }

        let decl_range = (def.def_start, def.body_close + 1);
        let mut candidate_edits: Vec<Edit> = vec![Edit {
            start: decl_range.0,
            end: decl_range.1,
            replacement: Vec::new(),
        }];
        for (site, subst) in sites.iter().zip(substitutions.into_iter()) {
            candidate_edits.push(Edit {
                start: site.name_start,
                end: site.stmt_end,
                replacement: subst,
            });
        }

        let conflicts = candidate_edits
            .iter()
            .any(|c| edits.iter().any(|e| overlaps((e.start, e.end), (c.start, c.end))));
        if conflicts {
            continue;
        }

        inlined_count += sites.len();
        edits.extend(candidate_edits);
    }

    if edits.is_empty() {
        return items;
    }
    edits.sort_by_key(|e| e.start);

    let mut out = Vec::with_capacity(items.len());
    let mut i = 0;
    let mut edit_iter = edits.into_iter().peekable();
    while i < items.len() {
        if let Some(edit) = edit_iter.peek() {
            if edit.start == i {
                let edit = edit_iter.next().unwrap();
                out.extend(edit.replacement);
                i = edit.end;
                continue;
            }
        }
        out.push(items[i].clone());
        i += 1;
    }
    stats.functions_inlined += inlined_count;
    out
}

#[cfg(test)]
mod tests {
    use crate::budget::estimate_budget;
    use crate::golfer::golf;

    #[test]
    fn inlines_a_single_call_site_pure_function() {
        let r = golf("float sq(float a){return a*a;}void mainImage(out vec4 c,in vec2 p){float x=p.x;c=vec4(sq(x));}", true);
        assert_eq!(r.code, "void mainImage(out vec4 c,in vec2 d){float a=d.x;c=vec4((a*a));}");
        assert_eq!(r.stats.aggressive.functions_inlined, 1);
    }

    #[test]
    fn wraps_a_substituted_binary_expression_in_parens_when_needed() {
        let r = golf(
            "float sq2(float a){return a+1.;}void mainImage(out vec4 c,in vec2 p){float x=p.x;c=vec4(2.*sq2(x));}",
            true,
        );
        assert_eq!(r.code, "void mainImage(out vec4 c,in vec2 d){float a=d.x;c=vec4(2.*(a+1.));}");
    }

    #[test]
    fn declines_when_called_more_than_once() {
        let r = golf(
            "float sq(float a){return a*a;}void mainImage(out vec4 c,in vec2 p){c=vec4(sq(p.x)+sq(p.y));}",
            true,
        );
        assert!(r.code.contains("return"), "expected sq to survive being called twice, got: {}", r.code);
        assert_eq!(r.stats.aggressive.functions_inlined, 0);
    }

    #[test]
    fn declines_when_an_argument_is_not_a_bare_operand() {
        let r = golf("float sq(float a){return a*a;}void mainImage(out vec4 c,in vec2 p){c=vec4(sq(p.x+1.));}", true);
        assert!(r.code.contains("return"), "a non-bare-operand argument must decline inlining, got: {}", r.code);
        assert_eq!(r.stats.aggressive.functions_inlined, 0);
    }

    #[test]
    fn declines_an_inout_parameter() {
        let r = golf(
            "float sq(inout float a){return a*a;}void mainImage(out vec4 c,in vec2 p){float x=p.x;c=vec4(sq(x));}",
            true,
        );
        assert!(r.code.contains("return"), "inout parameter must decline inlining, got: {}", r.code);
        assert_eq!(r.stats.aggressive.functions_inlined, 0);
    }

    #[test]
    fn declines_a_multi_statement_body() {
        let r = golf(
            "float sq(float a){float b=a*a;return b;}void mainImage(out vec4 c,in vec2 p){float x=p.x;c=vec4(sq(x));}",
            true,
        );
        assert!(r.code.contains("return"), "multi-statement body must decline inlining, got: {}", r.code);
        assert_eq!(r.stats.aggressive.functions_inlined, 0);
    }

    #[test]
    fn declines_self_recursive_functions_even_in_isolation() {
        let mut opts = crate::golfer::AggressiveOptions::none();
        opts.inline_single_call_functions = true;
        let out = crate::golfer::golf_with_options("float f(float a){return f(a);}void mainImage(out vec4 c,in vec2 p){c=vec4(1.);}", opts).code;
        assert!(out.contains("return"), "self-recursive candidate must decline inlining, got: {out}");
    }

    #[test]
    fn zero_parameter_function_inlines_cleanly() {
        let r = golf("float one(){return 1.;}void mainImage(out vec4 c,in vec2 p){c=vec4(one());}", true);
        assert_eq!(r.code, "void mainImage(out vec4 b,in vec2 c){b=vec4(1.);}");
        assert_eq!(r.stats.aggressive.functions_inlined, 1);
    }

    #[test]
    fn parenthesized_identifier_return_value_inlines_cleanly() {
        let r = golf("float id(float a){return (a);}void mainImage(out vec4 c,in vec2 p){float x=p.x;c=vec4(id(x));}", true);
        assert_eq!(r.code, "void mainImage(out vec4 c,in vec2 d){float a=d.x;c=vec4(a);}");
        assert_eq!(r.stats.aggressive.functions_inlined, 1);
    }

    #[test]
    fn unused_parameter_argument_is_never_silently_dropped() {
        let r = golf(
            "float first(float a,float b){return a;}void mainImage(out vec4 c,in vec2 p){float x=p.x;c=vec4(first(x,x));}",
            true,
        );
        assert_eq!(r.code, "void mainImage(out vec4 c,in vec2 d){float a=d.x;c=vec4(a);}");
        assert_eq!(r.stats.aggressive.functions_inlined, 1);
    }

    fn aggressive_opts() -> crate::golfer::AggressiveOptions {
        let mut o = crate::golfer::AggressiveOptions::none();
        o.aggressive_inlining = true;
        o
    }

    #[test]
    fn inline_aggressive_inlines_a_multi_call_site_multi_statement_side_effect_function_when_net_smaller() {
        // golf.md Phase 30.1: `tally` is called-as-statement from two
        // sites and its body is a straight-line run of pure-assignment
        // statements against a global -- both sites are net-smaller once
        // substituted, so the declaration and both call sites are removed.
        let r = crate::golfer::golf_with_protected_names(
            "float acc;void tally(float v){acc=acc+v;acc=acc*0.5;acc=acc+1.0;}void mainImage(out vec4 c,in vec2 p){acc=0.0;tally(p.x);tally(p.y);c=vec4(acc,acc,acc,1.0);}",
            aggressive_opts(),
            &["mainImage".to_string()],
        );
        assert!(!r.code.contains("tally"), "expected tally to be fully inlined away, got: {}", r.code);
        assert_eq!(r.stats.aggressive.functions_inlined, 2);
    }

    #[test]
    fn inline_aggressive_declines_when_substituting_every_call_site_is_net_larger() {
        // golf.md Phase 30.1: an 8-statement body duplicated at two call
        // sites is larger than keeping the declaration plus two short
        // calls, so this pass must measure before committing rather than
        // assume monotonic improvement.
        let r = crate::golfer::golf_with_protected_names(
            "float acc;void tally(float v){acc=acc+v;acc=acc*v;acc=acc-v;acc=acc/2.0;acc=acc+3.0;acc=acc*4.0;acc=acc-5.0;acc=acc+6.0;}void mainImage(out vec4 c,in vec2 p){acc=0.0;tally(p.x);tally(p.y);c=vec4(acc,acc,acc,1.0);}",
            aggressive_opts(),
            &["acc".to_string(), "mainImage".to_string(), "tally".to_string()],
        );
        assert!(r.code.contains("tally"), "expected tally to survive a net-larger inlining attempt, got: {}", r.code);
        assert_eq!(r.stats.aggressive.functions_inlined, 0);
    }

    #[test]
    fn inline_aggressive_never_inlines_a_body_containing_control_flow() {
        // golf.md Phase 30.1: a body containing `if`/`for`/`while`/a
        // mid-body `return` is never inlined by this pass -- it falls
        // through to the existing single-return path or is left uninlined.
        let r = crate::golfer::golf_with_protected_names(
            "float acc;void tally(float v){if(v>0.0){acc=acc+v;}else{acc=acc-v;}}void mainImage(out vec4 c,in vec2 p){acc=0.0;tally(p.x);tally(p.y);c=vec4(acc,acc,acc,1.0);}",
            aggressive_opts(),
            &["acc".to_string(), "mainImage".to_string(), "tally".to_string()],
        );
        assert!(r.code.contains("tally"), "a control-flow body must decline aggressive inlining, got: {}", r.code);
        assert_eq!(r.stats.aggressive.functions_inlined, 0);
    }

    #[test]
    fn inline_aggressive_never_inlines_a_self_recursive_function() {
        // golf.md Phase 30.1: reuses `CallGraph::is_recursive` -- a
        // self-recursive candidate is declined outright, matching
        // `inline_single_call_functions`'s existing recursion guard.
        let r = crate::golfer::golf_with_protected_names(
            "void tally(float v){tally(v);}void mainImage(out vec4 c,in vec2 p){tally(p.x);c=vec4(1.0);}",
            aggressive_opts(),
            &["mainImage".to_string(), "tally".to_string()],
        );
        assert!(r.code.contains("tally"), "a self-recursive candidate must decline aggressive inlining, got: {}", r.code);
        assert_eq!(r.stats.aggressive.functions_inlined, 0);
    }

    #[test]
    fn inline_aggressive_never_worsens_deflate_budget_on_the_tracked_fixture() {
        let source = include_str!("../../fixtures/aggressive_inlining.glsl");
        let mut inlined = crate::golfer::AggressiveOptions::all();
        inlined.aggressive_inlining = true;
        let uninlined = crate::golfer::AggressiveOptions::all();

        let inlined_result = crate::golfer::golf_with_options(source, inlined);
        let uninlined_result = crate::golfer::golf_with_options(source, uninlined);

        assert!(
            estimate_budget(&inlined_result.code).deflate_bytes
                <= estimate_budget(&uninlined_result.code).deflate_bytes,
            "aggressive inlining must not inflate the DEFLATE estimate\ninlined  : {}\nuninlined: {}",
            inlined_result.code,
            uninlined_result.code
        );
        assert!(inlined_result.stats.aggressive.functions_inlined >= 1);
    }
}
