use crate::aggressive::{
    classify_fusable_statement, find_ident, is_statement_boundary, parse_declaration_statement,
    scan_statement, skip_balanced, AggressiveStats, FusableStmt, Item,
};
use crate::budget::estimate_budget;
use crate::expr::{parse_expr, ExprKind};
use crate::lexer::Tok;

fn mk_punct(c: char, space_before: bool) -> Item {
    Item {
        tok: Tok::Punct(c),
        text: c.to_string(),
        space_before,
    }
}

fn mk_ident(s: &str, space_before: bool) -> Item {
    Item {
        tok: Tok::Ident(s.to_string()),
        text: s.to_string(),
        space_before,
    }
}

fn expr_references(kind: &ExprKind, name: &str) -> bool {
    match kind {
        ExprKind::Number(_) => false,
        ExprKind::Ident(n) => n == name,
        ExprKind::Unary(_, e) => expr_references(&e.kind, name),
        ExprKind::Binary(_, l, r) => expr_references(&l.kind, name) || expr_references(&r.kind, name),
        ExprKind::Ternary(c, t, e) => {
            expr_references(&c.kind, name) || expr_references(&t.kind, name) || expr_references(&e.kind, name)
        }
        ExprKind::Call(_, args) => args.iter().any(|a| expr_references(&a.kind, name)),
        ExprKind::Index(b, i) => expr_references(&b.kind, name) || expr_references(&i.kind, name),
        ExprKind::Member(b, _) => expr_references(&b.kind, name),
        ExprKind::Paren(e) => expr_references(&e.kind, name),
    }
}

fn span_references(items: &[Item], start: usize, end: usize, name: &str) -> bool {
    items[start..end]
        .iter()
        .any(|it| matches!(&it.tok, Tok::Ident(s) if s == name))
}

/// Recognizes one of the three `golf.md` Phase 31.1 counter-increment
/// shapes -- `i++`, `i+=1.`, `i=i+1.` -- for variable `var`, starting
/// exactly at `start`. Unlike a terminated statement, this variant is used
/// for the `for(...)` header's own increment clause, so it does not expect
/// a trailing `;`; instead the recognized shape must end exactly at
/// `end_limit` (the index of the header's closing `)`).
fn match_increment_clause(items: &[Item], start: usize, var: &str, end_limit: usize) -> bool {
    if find_ident(items, start) != Some(var) {
        return false;
    }
    // `i++`
    if let (Some(Tok::Punct(c1)), Some(second)) = (
        items.get(start + 1).map(|it| &it.tok),
        items.get(start + 2),
    ) {
        if let Tok::Punct(c2) = second.tok {
            if *c1 == '+' && c2 == '+' && !second.space_before && start + 3 == end_limit {
                return true;
            }
        }
    }
    // `i+=1.`
    if matches!(items.get(start + 1).map(|it| &it.tok), Some(Tok::Punct('+')))
        && matches!(items.get(start + 2).map(|it| &it.tok), Some(Tok::Punct('=')))
    {
        if let Some(item) = items.get(start + 3) {
            if matches!(&item.tok, Tok::Number(_)) {
                let n = item.text.as_str();
                if (n == "1" || n == "1.") && start + 4 == end_limit {
                    return true;
                }
            }
        }
    }
    // `i=i+1.`, proven via `expr.rs`'s `Expr` parser rather than raw tokens.
    if matches!(items.get(start + 1).map(|it| &it.tok), Some(Tok::Punct('='))) {
        let is_eqeq = matches!(items.get(start + 2).map(|it| &it.tok), Some(Tok::Punct('=')))
            && !items.get(start + 2).map(|it| it.space_before).unwrap_or(true);
        if !is_eqeq {
            if let Some(expr) = parse_expr(items, start + 2) {
                if let ExprKind::Binary(op, l, r) = &expr.kind {
                    if op == "+" {
                        if let (ExprKind::Ident(li), ExprKind::Number(_)) = (&l.kind, &r.kind) {
                            let num_text = items[r.start].text.as_str();
                            if li == var && (num_text == "1" || num_text == "1.") && expr.end == end_limit {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    false
}

/// Same three shapes as `match_increment_clause`, but for a full body
/// statement terminated by `;`. Returns the index just past the `;` on a
/// match.
fn match_increment_statement(items: &[Item], start: usize, var: &str) -> Option<usize> {
    if matches!(items.get(start).map(|it| &it.tok), Some(Tok::Punct(';'))) {
        return None;
    }
    // Find where the statement ends the same way `match_increment_clause`
    // would validate it, but here the terminator is a `;` we must also
    // consume.
    let mut end = start;
    while end < items.len() {
        if matches!(items.get(end).map(|it| &it.tok), Some(Tok::Punct(';'))) {
            break;
        }
        end += 1;
    }
    if end >= items.len() {
        return None;
    }
    if match_increment_clause(items, start, var, end) {
        Some(end + 1)
    } else {
        None
    }
}

struct BodyEdit {
    counter_type: Item,
    counter_name: Item,
    init_tokens: Vec<Item>,
    cond_op: &'static str,
    bound_tokens: Vec<Item>,
    increment_at_top: bool,
    body_open: Item,
    body_close: Item,
    body_inner: Vec<Item>,
}

fn collect_statement_spans(items: &[Item], start: usize, end: usize) -> Option<Vec<(usize, usize)>> {
    let mut spans = Vec::new();
    let mut i = start;
    while i < end {
        let stmt_end = scan_statement(items, i)?;
        if stmt_end > end {
            return None;
        }
        spans.push((i, stmt_end));
        i = stmt_end;
    }
    Some(spans)
}

fn try_golf_loop(items: &[Item], decl_start: usize) -> Option<(BodyEdit, usize)> {
    let (type_name, subs, decl_end) = parse_declaration_statement(items, decl_start)?;
    if subs.len() != 1 {
        return None;
    }
    let counter_sub = &subs[0];
    let counter_name = find_ident(items, counter_sub.name_idx)?.to_string();
    let (_, init_start, init_end) = counter_sub.initializer.as_ref()?;
    let init_start = *init_start;
    let init_end = *init_end;

    if find_ident(items, decl_end) != Some("for") {
        return None;
    }
    let for_start = decl_end;
    if !matches!(items.get(for_start + 1).map(|it| &it.tok), Some(Tok::Punct('('))) {
        return None;
    }
    let paren_end = skip_balanced(items, for_start + 1, '(', ')')?;

    let header_start = for_start + 2;
    let (loop_type, loop_subs, init_clause_end) = parse_declaration_statement(items, header_start)?;
    let _ = loop_type;
    if loop_subs.len() != 1 {
        return None;
    }
    let loop_var = find_ident(items, loop_subs[0].name_idx)?.to_string();
    if loop_var == counter_name {
        return None;
    }

    let cond_expr = parse_expr(items, init_clause_end)?;
    let (op, l, r) = match &cond_expr.kind {
        ExprKind::Binary(op, l, r) if op == "<" || op == "<=" => (op.as_str(), l, r),
        _ => return None,
    };
    if !matches!(&l.kind, ExprKind::Ident(n) if *n == loop_var) {
        return None;
    }
    if expr_references(&r.kind, &loop_var) || expr_references(&r.kind, &counter_name) {
        return None;
    }
    if !matches!(items.get(cond_expr.end).map(|it| &it.tok), Some(Tok::Punct(';'))) {
        return None;
    }
    let incr_start = cond_expr.end + 1;
    let header_close = paren_end - 1;
    if !match_increment_clause(items, incr_start, &loop_var, header_close) {
        return None;
    }

    // The counter's own initializer must not (nonsensically) reference the
    // loop variable, which is not yet declared at that point.
    if span_references(items, init_start, init_end, &loop_var) {
        return None;
    }

    // The bound is copied verbatim from the original `for` header, except
    // for one narrow, explicitly-needed coercion: a bare integer literal
    // bound (`j<8`) compared against an `int` loop variable must become a
    // float literal (`i++<8.`) once the comparison moves onto the `float`
    // counter, matching this document's own worked example. Anything more
    // complex than a single bare literal is left untouched rather than
    // guessed at.
    let mut bound_tokens = items[r.start..r.end].to_vec();
    if type_name == "float" && bound_tokens.len() == 1 {
        if matches!(&bound_tokens[0].tok, Tok::Number(_)) {
            let n = bound_tokens[0].text.clone();
            if !n.contains('.') && !n.to_ascii_lowercase().contains('e') && n.chars().all(|c| c.is_ascii_digit()) {
                let new_text = format!("{}.", n);
                bound_tokens[0] = Item {
                    tok: Tok::Number(new_text.clone()),
                    text: new_text,
                    space_before: bound_tokens[0].space_before,
                };
            }
        }
    }

    let body_start = paren_end;
    if !matches!(items.get(body_start).map(|it| &it.tok), Some(Tok::Punct('{'))) {
        return None;
    }
    let body_end = skip_balanced(items, body_start, '{', '}')?;
    let inner_start = body_start + 1;
    let inner_end = body_end - 1;

    // golf.md Phase 31.1: never applied when the loop contains a
    // `continue` -- conservative "decline rather than risk" rule reused
    // from Phase 30.4.
    if span_references(items, inner_start, inner_end, "continue") {
        return None;
    }
    // The old loop variable must not be read anywhere in the body, since
    // its declaration is being discarded entirely.
    if span_references(items, inner_start, inner_end, &loop_var) {
        return None;
    }

    let spans = collect_statement_spans(items, inner_start, inner_end)?;
    if spans.is_empty() {
        return None;
    }

    let (increment_at_top, removed_span) = {
        let (first_start, first_end) = spans[0];
        if match_increment_statement(items, first_start, &counter_name) == Some(first_end) {
            (true, (first_start, first_end))
        } else {
            let (last_start, last_end) = *spans.last().unwrap();
            if match_increment_statement(items, last_start, &counter_name) == Some(last_end) {
                (false, (last_start, last_end))
            } else {
                return None;
            }
        }
    };

    // Critical correctness check: moving the counter's declaration into
    // the `for(...)` header changes its scope to the loop statement
    // itself (GLSL, like C, scopes a for-init declaration to the loop).
    // If anything after the loop still reads the counter's final value
    // (a very common idiom -- e.g. reporting how many raymarch steps
    // were taken), the rewrite would make that read refer to an
    // out-of-scope variable, so it must be declined outright.
    if span_references(items, body_end, items.len(), &counter_name) {
        return None;
    }

    let mut body_inner: Vec<Item> = Vec::with_capacity(inner_end - inner_start);
    body_inner.extend_from_slice(&items[inner_start..removed_span.0]);
    body_inner.extend_from_slice(&items[removed_span.1..inner_end]);

    let edit = BodyEdit {
        counter_type: Item {
            tok: Tok::Ident(type_name.clone()),
            text: type_name,
            space_before: true,
        },
        counter_name: items[counter_sub.name_idx].clone(),
        init_tokens: items[init_start..init_end].to_vec(),
        cond_op: match op {
            "<" => "<",
            _ => "<=",
        },
        bound_tokens,
        increment_at_top,
        body_open: items[body_start].clone(),
        body_close: items[body_end - 1].clone(),
        body_inner,
    };

    Some((edit, body_end))
}

fn render_edit(edit: BodyEdit) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::new();
    out.push(Item {
        tok: Tok::Ident("for".to_string()),
        text: "for".to_string(),
        space_before: true,
    });
    out.push(mk_punct('(', false));
    out.push(edit.counter_type);
    out.push(edit.counter_name.clone());
    out.push(mk_punct('=', false));
    out.extend(edit.init_tokens);
    out.push(mk_punct(';', false));
    out.push(edit.counter_name.clone());
    if edit.increment_at_top {
        out.push(mk_punct('+', false));
        out.push(mk_punct('+', false));
    }
    for c in edit.cond_op.chars() {
        out.push(mk_punct(c, false));
    }
    out.extend(edit.bound_tokens);
    out.push(mk_punct(';', false));
    if !edit.increment_at_top {
        out.push(edit.counter_name);
        out.push(mk_punct('+', false));
        out.push(mk_punct('+', false));
    }
    out.push(mk_punct(')', false));
    out.push(edit.body_open);
    out.extend(edit.body_inner);
    out.push(edit.body_close);
    out
}

/// `golf.md` Phase 31.1 -- folds the extremely common Shadertoy/demoscene
/// idiom of a standalone float counter incremented at the top or bottom of
/// a `for` loop body into the `for(...)` header itself, e.g. rewriting
/// `float i=0.;for(int j=0;j<8;j++){i+=1.;...}` into
/// `for(float i=0.;i++<8.;){...}`.
pub fn golf_loop_headers(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        if is_statement_boundary(&items, i) {
            if let Some((edit, consumed_end)) = try_golf_loop(&items, i) {
                out.extend(render_edit(edit));
                stats.loop_headers_golfed += 1;
                i = consumed_end;
                continue;
            }
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn char_len(items: &[Item]) -> usize {
    items.iter().map(|it| it.text.chars().count()).sum()
}

fn item_text(items: &[Item]) -> String {
    items.iter().map(|it| it.text.as_str()).collect()
}

/// True exactly when the span `[start, end)` is the single token literal
/// `true` -- the only condition value a `for(...)`'s test clause can ever
/// safely drop entirely (an empty test clause means "always true", GLSL
/// has no implicit int-to-bool conversion so `while(1)`/`do{}while(1);`
/// are not legal GLSL to begin with, and any other non-trivial condition
/// must stay in place to keep its own side effects, if any, in order).
fn is_literal_true(items: &[Item], start: usize, end: usize) -> bool {
    end == start + 1 && matches!(items.get(start).map(|it| &it.tok), Some(Tok::Ident(s)) if s == "true")
}

/// Requires every statement across `[start, end)` to classify as a bare
/// fusable expression-statement (`classify_fusable_statement`, the exact
/// same assignment/postfix-incr-decr/call-statement whitelist Phase 30.3's
/// `fuse_statement_sequences` already established and shipped) and to
/// cover the span exactly, with nothing left over. A single declaration,
/// control-flow statement, `return`, `break`, or `continue` anywhere in
/// the span makes the whole span unrepresentable and this returns `None`
/// -- matching this module's "decline rather than risk" rule, not a new
/// safety class.
fn collect_full_fusable_span(items: &[Item], start: usize, end: usize) -> Option<Vec<FusableStmt>> {
    let mut out = Vec::new();
    let mut i = start;
    while i < end {
        let stmt = classify_fusable_statement(items, i)?;
        if stmt.stmt_end > end {
            return None;
        }
        i = stmt.stmt_end;
        out.push(stmt);
    }
    if i != end {
        return None;
    }
    Some(out)
}

/// `golf.md` Phase 31.2 -- `while(cond){body}` is always exactly
/// equivalent to `for(;cond;){body}` (a `for` loop's test clause is
/// evaluated at the same point in every iteration a `while`'s is, and its
/// empty init/increment clauses mean `continue` still lands on the
/// condition test, matching `while`'s own `continue` target exactly), so
/// -- unlike `golf_loop_headers` above -- this rewrite never needs to
/// inspect the body at all, not even for `continue`. Only fires when the
/// resulting header is strictly shorter, which in practice means only
/// `while(true){...}` (where the `for` form can drop the condition
/// entirely, per `is_literal_true`) actually wins -- any other condition
/// ties exactly in raw character count (`while(` .. `)` and `for(;` ..
/// `;)` are both seven boilerplate characters), so this never fires for a
/// non-trivial condition, by design rather than by omission.
fn try_golf_while(items: &[Item], start: usize) -> Option<(Vec<Item>, usize)> {
    if find_ident(items, start) != Some("while") {
        return None;
    }
    let paren_open = start + 1;
    if !matches!(items.get(paren_open).map(|it| &it.tok), Some(Tok::Punct('('))) {
        return None;
    }
    let paren_end = skip_balanced(items, paren_open, '(', ')')?;
    let cond_start = paren_open + 1;
    let cond_end = paren_end - 1;
    if cond_end <= cond_start {
        return None;
    }
    let body_start = paren_end;
    // Only the common brace-delimited body form is handled -- a
    // single-statement, brace-free `while(cond) stmt;` body is left
    // untouched rather than risked, matching this document's "decline
    // rather than risk" rule.
    if !matches!(items.get(body_start).map(|it| &it.tok), Some(Tok::Punct('{'))) {
        return None;
    }
    let body_end = skip_balanced(items, body_start, '{', '}')?;

    let before_header = items[start..body_start].to_vec();

    let mut after_header: Vec<Item> = Vec::new();
    after_header.push(mk_ident("for", items[start].space_before));
    after_header.push(mk_punct('(', false));
    after_header.push(mk_punct(';', false));
    if !is_literal_true(items, cond_start, cond_end) {
        after_header.extend(items[cond_start..cond_end].iter().cloned());
    }
    after_header.push(mk_punct(';', false));
    after_header.push(mk_punct(')', false));

    if char_len(&after_header) >= char_len(&before_header) {
        return None;
    }
    if estimate_budget(&item_text(&after_header)).deflate_bytes
        >= estimate_budget(&item_text(&before_header)).deflate_bytes
    {
        return None;
    }

    let mut out = after_header;
    out.extend(items[body_start..body_end].iter().cloned());
    Some((out, body_end))
}

/// `golf.md` Phase 31.2 -- `do{S}while(cond);` is equivalent to running
/// `S` once unconditionally and then looping `for(;cond;){S}`; rather than
/// duplicating `S` (which is never smaller), this folds `S` itself into
/// the `for` header's own test clause via the comma operator: `S`'s
/// statements are only representable there when every one of them is
/// already representable as a bare `Expr` (`collect_full_fusable_span`
/// above), so `for(;S_as_comma_expr,cond;);` evaluates `S`'s statements in
/// order and then `cond` on every iteration, exactly once per iteration,
/// exactly like the original `do`/`while` did -- and `continue` cannot
/// appear inside `S` at all once this constraint holds (it is a
/// control-flow statement, so `classify_fusable_statement` already
/// declines it), so no separate `continue` guard is needed the way
/// `golf_loop_headers` above needed one.
fn try_golf_do_while(items: &[Item], start: usize) -> Option<(Vec<Item>, usize)> {
    if find_ident(items, start) != Some("do") {
        return None;
    }
    let body_start = start + 1;
    if !matches!(items.get(body_start).map(|it| &it.tok), Some(Tok::Punct('{'))) {
        return None;
    }
    let body_end = skip_balanced(items, body_start, '{', '}')?;
    let inner_start = body_start + 1;
    let inner_end = body_end - 1;

    if find_ident(items, body_end) != Some("while") {
        return None;
    }
    let paren_open = body_end + 1;
    if !matches!(items.get(paren_open).map(|it| &it.tok), Some(Tok::Punct('('))) {
        return None;
    }
    let paren_end = skip_balanced(items, paren_open, '(', ')')?;
    let cond_start = paren_open + 1;
    let cond_end = paren_end - 1;
    if cond_end <= cond_start {
        return None;
    }
    if !matches!(items.get(paren_end).map(|it| &it.tok), Some(Tok::Punct(';'))) {
        return None;
    }
    let stmt_end = paren_end + 1;

    let body_stmts = collect_full_fusable_span(items, inner_start, inner_end)?;
    let drop_cond = body_stmts.is_empty() && is_literal_true(items, cond_start, cond_end);

    let before = items[start..stmt_end].to_vec();

    let mut after: Vec<Item> = Vec::new();
    after.push(mk_ident("for", items[start].space_before));
    after.push(mk_punct('(', false));
    after.push(mk_punct(';', false));
    for stmt in &body_stmts {
        after.extend(items[stmt.start..stmt.expr_end].iter().cloned());
        after.push(mk_punct(',', false));
    }
    if !drop_cond {
        after.extend(items[cond_start..cond_end].iter().cloned());
    }
    after.push(mk_punct(';', false));
    after.push(mk_punct(')', false));
    after.push(mk_punct(';', false));

    if char_len(&after) >= char_len(&before) {
        return None;
    }
    if estimate_budget(&item_text(&after)).deflate_bytes >= estimate_budget(&item_text(&before)).deflate_bytes {
        return None;
    }

    Some((after, stmt_end))
}

/// `golf.md` Phase 31.2 -- normalizes `while`/`do`-`while` loops into the
/// `for` form whenever doing so is strictly shorter (see `try_golf_while`
/// and `try_golf_do_while` above for the two shapes and why each is
/// behavior-preserving unconditionally, not just under a size guard).
/// Loop-form choice is purely cosmetic and reversible -- unlike every
/// other pass in this file, it carries no correctness precondition beyond
/// its own size guard, so callers may enable it freely.
pub fn golf_loop_forms(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        if is_statement_boundary(&items, i) {
            if let Some((replacement, consumed_end)) = try_golf_do_while(&items, i) {
                out.extend(replacement);
                stats.loop_forms_normalized += 1;
                i = consumed_end;
                continue;
            }
            if let Some((replacement, consumed_end)) = try_golf_while(&items, i) {
                out.extend(replacement);
                stats.loop_forms_normalized += 1;
                i = consumed_end;
                continue;
            }
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

#[cfg(test)]
mod tests {
    use crate::golfer::{golf_with_protected_names, AggressiveOptions};

    fn opts() -> AggressiveOptions {
        let mut o = AggressiveOptions::none();
        o.loop_header_golf = true;
        o
    }

    #[test]
    fn folds_top_of_body_increment_into_postincrement_condition() {
        let r = golf_with_protected_names(
            "void f(){float i=0.;for(int j=0;j<8;j++){i+=1.;g=g+i;}}",
            opts(),
            &["f".to_string(), "i".to_string(), "j".to_string(), "g".to_string()],
        );
        assert_eq!(r.code, "void f(){for(float i=0.;i++<8.;){g=g+i;}}");
        assert_eq!(r.stats.aggressive.loop_headers_golfed, 1);
    }

    #[test]
    fn folds_bottom_of_body_increment_keeping_the_increment_clause() {
        let r = golf_with_protected_names(
            "void f(){float i=0.;for(int j=0;j<8;j++){g=g+i;i+=1.;}}",
            opts(),
            &["f".to_string(), "i".to_string(), "j".to_string(), "g".to_string()],
        );
        assert_eq!(r.code, "void f(){for(float i=0.;i<8.;i++){g=g+i;}}");
        assert_eq!(r.stats.aggressive.loop_headers_golfed, 1);
    }

    #[test]
    fn recognizes_the_plain_postfix_increment_shape() {
        let r = golf_with_protected_names(
            "void f(){float i=0.;for(int j=0;j<8;j++){i++;g=g+i;}}",
            opts(),
            &["f".to_string(), "i".to_string(), "j".to_string(), "g".to_string()],
        );
        assert_eq!(r.code, "void f(){for(float i=0.;i++<8.;){g=g+i;}}");
    }

    #[test]
    fn recognizes_the_i_equals_i_plus_1_shape_via_the_expr_parser() {
        let r = golf_with_protected_names(
            "void f(){float i=0.;for(int j=0;j<8;j++){i=i+1.;g=g+i;}}",
            opts(),
            &["f".to_string(), "i".to_string(), "j".to_string(), "g".to_string()],
        );
        assert_eq!(r.code, "void f(){for(float i=0.;i++<8.;){g=g+i;}}");
    }

    #[test]
    fn declines_when_a_continue_is_present_in_the_body() {
        let source = "void f(){float i=0.;for(int j=0;j<8;j++){if(g>0.)continue;i+=1.;g=g+i;}}";
        let r = golf_with_protected_names(
            source,
            opts(),
            &["f".to_string(), "i".to_string(), "j".to_string(), "g".to_string()],
        );
        assert_eq!(r.stats.aggressive.loop_headers_golfed, 0);
        assert!(r.code.contains("for(int j=0;j<8;j++)"));
    }

    #[test]
    fn declines_when_the_increment_is_in_the_middle_of_the_body() {
        let source = "void f(){float i=0.;for(int j=0;j<8;j++){g=g+i;i+=1.;g=g+i;}}";
        let r = golf_with_protected_names(
            source,
            opts(),
            &["f".to_string(), "i".to_string(), "j".to_string(), "g".to_string()],
        );
        assert_eq!(r.stats.aggressive.loop_headers_golfed, 0);
    }

    #[test]
    fn declines_when_the_loop_variable_is_also_read_in_the_body() {
        let source = "void f(){float i=0.;for(int j=0;j<8;j++){i+=1.;g=g+float(j);}}";
        let r = golf_with_protected_names(
            source,
            opts(),
            &["f".to_string(), "i".to_string(), "j".to_string(), "g".to_string()],
        );
        assert_eq!(r.stats.aggressive.loop_headers_golfed, 0);
    }

    #[test]
    fn is_off_by_default_even_in_all() {
        assert!(!AggressiveOptions::all().loop_header_golf);
    }

    #[test]
    fn never_worsens_deflate_budget_on_the_tracked_fixture() {
        use crate::budget::estimate_budget;
        use crate::golfer::golf_with_options;

        let source = include_str!("../../fixtures/loop_header_golf.glsl");
        let mut golfed = AggressiveOptions::all();
        golfed.loop_header_golf = true;
        let mut ungolfed = AggressiveOptions::all();
        ungolfed.loop_header_golf = false;

        let golfed_result = golf_with_options(source, golfed);
        let ungolfed_result = golf_with_options(source, ungolfed);

        assert!(
            estimate_budget(&golfed_result.code).deflate_bytes
                <= estimate_budget(&ungolfed_result.code).deflate_bytes,
            "loop header golfing must not inflate the DEFLATE estimate\ngolfed  : {}\nungolfed: {}",
            golfed_result.code,
            ungolfed_result.code
        );
        assert!(golfed_result.stats.aggressive.loop_headers_golfed >= 2);
    }
}

#[cfg(test)]
mod loop_form_tests {
    use crate::golfer::{golf_with_protected_names, AggressiveOptions};

    fn opts() -> AggressiveOptions {
        let mut o = AggressiveOptions::none();
        o.loop_form_golf = true;
        o
    }

    #[test]
    fn while_true_becomes_an_empty_for_header() {
        let r = golf_with_protected_names(
            "void f(){while(true){x++;}}",
            opts(),
            &["f".to_string(), "x".to_string()],
        );
        assert_eq!(r.code, "void f(){for(;;){x++;}}");
        assert_eq!(r.stats.aggressive.loop_forms_normalized, 1);
    }

    #[test]
    fn while_with_a_non_trivial_condition_never_changes_size_so_it_declines() {
        let source = "void f(){while(x<8){x++;}}";
        let r = golf_with_protected_names(source, opts(), &["f".to_string(), "x".to_string()]);
        assert_eq!(r.code, source);
        assert_eq!(r.stats.aggressive.loop_forms_normalized, 0);
    }

    #[test]
    fn while_with_a_brace_free_body_is_left_untouched() {
        let source = "void f(){while(true)x++;}";
        let r = golf_with_protected_names(source, opts(), &["f".to_string(), "x".to_string()]);
        assert_eq!(r.code, source);
        assert_eq!(r.stats.aggressive.loop_forms_normalized, 0);
    }

    #[test]
    fn do_while_folds_its_expression_only_body_into_the_for_test_clause() {
        let r = golf_with_protected_names(
            "void f(){do{x++;}while(x<8.);}",
            opts(),
            &["f".to_string(), "x".to_string()],
        );
        assert_eq!(r.code, "void f(){for(;x++,x<8.;);}");
        assert_eq!(r.stats.aggressive.loop_forms_normalized, 1);
    }

    #[test]
    fn do_while_with_an_empty_body_and_a_literal_true_condition_becomes_for_ever() {
        let r = golf_with_protected_names(
            "void f(){do{}while(true);}",
            opts(),
            &["f".to_string()],
        );
        assert_eq!(r.code, "void f(){for(;;);}");
        assert_eq!(r.stats.aggressive.loop_forms_normalized, 1);
    }

    #[test]
    fn do_while_declines_when_the_body_contains_a_break() {
        let source = "void f(){do{if(x>0.)break;x++;}while(x<8.);}";
        let r = golf_with_protected_names(source, opts(), &["f".to_string(), "x".to_string()]);
        assert_eq!(r.code, source);
        assert_eq!(r.stats.aggressive.loop_forms_normalized, 0);
    }

    #[test]
    fn do_while_declines_when_the_body_contains_a_declaration() {
        let source = "void f(){do{float y=x;x=y+1.;}while(x<8.);}";
        let r = golf_with_protected_names(source, opts(), &["f".to_string(), "x".to_string(), "y".to_string()]);
        assert_eq!(r.code, source);
        assert_eq!(r.stats.aggressive.loop_forms_normalized, 0);
    }

    #[test]
    fn is_off_by_default_even_in_all() {
        assert!(!AggressiveOptions::all().loop_form_golf);
    }

    #[test]
    fn never_worsens_deflate_budget_on_the_tracked_fixture() {
        use crate::budget::estimate_budget;
        use crate::golfer::golf_with_options;

        let source = include_str!("../../fixtures/loop_form_golf.glsl");
        let mut golfed = AggressiveOptions::all();
        golfed.loop_form_golf = true;
        let mut ungolfed = AggressiveOptions::all();
        ungolfed.loop_form_golf = false;

        let golfed_result = golf_with_options(source, golfed);
        let ungolfed_result = golf_with_options(source, ungolfed);

        assert!(
            estimate_budget(&golfed_result.code).deflate_bytes
                <= estimate_budget(&ungolfed_result.code).deflate_bytes,
            "loop form normalization must not inflate the DEFLATE estimate\ngolfed  : {}\nungolfed: {}",
            golfed_result.code,
            ungolfed_result.code
        );
        assert!(golfed_result.stats.aggressive.loop_forms_normalized >= 2);
    }
}




#[cfg(test)]
mod print_form_fixture {
    #[test]
    fn print_golfed_fixture() {
        use crate::golfer::{golf_with_protected_names, AggressiveOptions};
        let mut o = AggressiveOptions::none();
        o.loop_form_golf = true;
        let source = include_str!("../../fixtures/loop_form_golf.glsl");
        let r = golf_with_protected_names(source, o, &["mainImage".to_string()]);
        println!("GOLFED>>>{}<<<GOLFED", r.code);
    }
}
