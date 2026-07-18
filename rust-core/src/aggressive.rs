use crate::expr::{parse_expr, Expr, ExprKind};
use crate::lexer::Tok;
use crate::vocab::type_keywords;
use std::collections::{HashMap, HashSet};

#[derive(Clone, PartialEq)]
pub struct Item {
    pub tok: Tok,
    pub text: String,
    pub space_before: bool,
}

#[derive(Default, Debug, Clone, Copy)]
pub struct AggressiveStats {
    pub compound_assignments: usize,
    pub declarations_merged: usize,
    pub braces_removed: usize,
    pub constants_folded: usize,
    pub dead_locals_removed: usize,
    pub dead_stores_removed: usize,
    pub constant_vectors_reduced: usize,
    pub trailing_void_returns_removed: usize,
    pub increments_decrements: usize,
    pub ternaries_from_if_else: usize,
    pub redundant_parens_removed: usize,
    pub duplicate_precision_removed: usize,
    pub dead_functions_removed: usize,
    pub functions_inlined: usize,
    pub algebraic_identities_simplified: usize,
    pub common_subexpressions_eliminated: usize,
}

pub(crate) fn is_unary_prefix(c: char) -> bool {
    matches!(c, '-' | '+' | '!' | '~')
}

fn find_ident(items: &[Item], i: usize) -> Option<&str> {
    match items.get(i).map(|it| &it.tok) {
        Some(Tok::Ident(s)) => Some(s.as_str()),
        _ => None,
    }
}

fn is_bare_operand(items: &[Item], idx: usize) -> bool {
    matches!(items.get(idx).map(|it| &it.tok), Some(Tok::Ident(_)) | Some(Tok::Number(_)))
}

fn try_remove_dead_decl(
    items: &[Item],
    start: usize,
    type_kw: &std::collections::HashSet<&'static str>,
    freq: &HashMap<String, usize>,
) -> Option<usize> {
    let t = find_ident(items, start)?;
    if !type_kw.contains(t) {
        return None;
    }
    let name = find_ident(items, start + 1)?;
    if freq.get(name).copied().unwrap_or(0) != 1 {
        return None;
    }

    match items.get(start + 2).map(|it| &it.tok) {
        Some(Tok::Punct(';')) => Some(start + 3),
        Some(Tok::Punct('=')) => {
            let mut i = start + 3;
            if let Some(Tok::Punct(c)) = items.get(i).map(|it| &it.tok) {
                if is_unary_prefix(*c) {
                    i += 1;
                }
            }
            if !is_bare_operand(items, i) {
                return None;
            }
            i += 1;
            match items.get(i).map(|it| &it.tok) {
                Some(Tok::Punct(';')) => Some(i + 1),
                _ => None,
            }
        }
        _ => None,
    }
}

pub fn eliminate_dead_locals(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let type_kw = type_keywords();
    let mut freq: HashMap<String, usize> = HashMap::new();
    for it in &items {
        if let Tok::Ident(name) = &it.tok {
            *freq.entry(name.clone()).or_insert(0) += 1;
        }
    }

    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        let at_boundary = out
            .last()
            .is_none_or(|it: &Item| matches!(it.tok, Tok::Punct(';') | Tok::Punct('{') | Tok::Punct('}')));

        if at_boundary {
            if let Some(end) = try_remove_dead_decl(&items, i, type_kw, &freq) {
                stats.dead_locals_removed += 1;
                i = end;
                continue;
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

struct SimpleWrite {
    is_decl: bool,
    name: String,
    rhs_ident: Option<String>,
    start: usize,
    end: usize,
}

fn parse_simple_write(items: &[Item], start: usize) -> Option<SimpleWrite> {
    let type_kw = type_keywords();
    let mut i = start;
    let is_decl = matches!(find_ident(items, i), Some(t) if type_kw.contains(t));
    if is_decl {
        i += 1;
    }
    let name = find_ident(items, i)?.to_string();
    i += 1;
    if !matches!(items.get(i).map(|it| &it.tok), Some(Tok::Punct('='))) {
        return None;
    }
    i += 1;
    if let Some(Tok::Punct(c)) = items.get(i).map(|it| &it.tok) {
        if is_unary_prefix(*c) {
            i += 1;
        }
    }
    let rhs_ident = match items.get(i).map(|it| &it.tok) {
        Some(Tok::Ident(n)) => Some(n.clone()),
        Some(Tok::Number(_)) => None,
        _ => return None,
    };
    i += 1;
    if !matches!(items.get(i).map(|it| &it.tok), Some(Tok::Punct(';'))) {
        return None;
    }
    Some(SimpleWrite {
        is_decl,
        name,
        rhs_ident,
        start,
        end: i + 1,
    })
}

fn parse_write_chain(items: &[Item], start: usize) -> Vec<SimpleWrite> {
    let mut chain = Vec::new();
    let mut i = start;
    while let Some(w) = parse_simple_write(items, i) {
        i = w.end;
        chain.push(w);
    }
    chain
}

fn find_dead_writes_in_chain(chain: &[SimpleWrite]) -> HashSet<usize> {
    let mut dead = HashSet::new();
    for i in 0..chain.len() {
        for j in (i + 1)..chain.len() {
            if chain[j].name == chain[i].name {
                let read_before_overwrite = chain[(i + 1)..=j]
                    .iter()
                    .any(|w| w.rhs_ident.as_deref() == Some(chain[i].name.as_str()));
                if !read_before_overwrite {
                    dead.insert(i);
                }
                break;
            }
        }
    }
    dead
}

pub fn eliminate_dead_stores(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    let mut depth = 0i32;
    while i < items.len() {
        if depth == 0 {
            let chain = parse_write_chain(&items, i);
            let dead = find_dead_writes_in_chain(&chain);
            if !dead.is_empty() {
                for (idx, w) in chain.iter().enumerate() {
                    if dead.contains(&idx) {
                        stats.dead_stores_removed += 1;
                        if w.is_decl {
                            out.push(items[w.start].clone());
                            out.push(items[w.start + 1].clone());
                            out.push(Item {
                                tok: Tok::Punct(';'),
                                text: ";".to_string(),
                                space_before: false,
                            });
                        }
                    } else {
                        out.extend(items[w.start..w.end].iter().cloned());
                    }
                }
                i = chain.last().map(|w| w.end).unwrap_or(i);
                continue;
            }
        }
        match items[i].tok {
            Tok::Punct('(') | Tok::Punct('[') => depth += 1,
            Tok::Punct(')') | Tok::Punct(']') => depth -= 1,
            _ => {}
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

pub(crate) fn skip_balanced(items: &[Item], open: usize, open_c: char, close_c: char) -> Option<usize> {
    match items.get(open).map(|it| &it.tok) {
        Some(Tok::Punct(c)) if *c == open_c => {}
        _ => return None,
    }
    let mut depth = 0i32;
    let mut i = open;
    loop {
        match items.get(i).map(|it| &it.tok) {
            Some(Tok::Punct(c)) if *c == open_c => depth += 1,
            Some(Tok::Punct(c)) if *c == close_c => {
                depth -= 1;
                if depth == 0 {
                    return Some(i + 1);
                }
            }
            None => return None,
            _ => {}
        }
        i += 1;
    }
}

fn scan_primary(items: &[Item], start: usize) -> Option<usize> {
    let mut i = start;
    while let Some(Tok::Punct(c)) = items.get(i).map(|it| &it.tok) {
        if is_unary_prefix(*c) {
            i += 1;
        } else {
            break;
        }
    }
    match items.get(i).map(|it| &it.tok) {
        Some(Tok::Ident(_)) | Some(Tok::Number(_)) => i += 1,
        Some(Tok::Punct('(')) => i = skip_balanced(items, i, '(', ')')?,
        _ => return None,
    }
    loop {
        match items.get(i).map(|it| &it.tok) {
            Some(Tok::Punct('.')) => {
                if matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Ident(_))) {
                    i += 2;
                } else {
                    return None;
                }
            }
            Some(Tok::Punct('[')) => i = skip_balanced(items, i, '[', ']')?,
            Some(Tok::Punct('(')) => i = skip_balanced(items, i, '(', ')')?,
            _ => break,
        }
    }
    Some(i)
}

const FOLDABLE_OPS: &[char] = &['*', '/', '%'];

fn parse_plain_int(raw: &str) -> Option<i64> {
    if raw.is_empty() || raw.starts_with("0x") || raw.starts_with("0X") {
        return None;
    }
    if !raw.chars().all(|c| c.is_ascii_digit()) {
        return None;
    }
    raw.parse::<i64>().ok()
}

fn fold_int_op(a: i64, op: char, b: i64) -> Option<i64> {
    let result = match op {
        '*' => a.checked_mul(b)?,
        '/' => {
            if b == 0 || (a == i32::MIN as i64 && b == -1) {
                return None;
            }
            a / b
        }
        '%' => {
            if b == 0 {
                return None;
            }
            a % b
        }
        _ => return None,
    };
    if result < i32::MIN as i64 || result > i32::MAX as i64 {
        return None;
    }
    Some(result)
}

pub fn fold_constants(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        let left = match out.last() {
            Some(Item {
                tok: Tok::Number(raw),
                ..
            }) => parse_plain_int(raw),
            _ => None,
        };
        let op = match (left, items.get(i).map(|it| &it.tok)) {
            (Some(_), Some(Tok::Punct(c))) if FOLDABLE_OPS.contains(c) => Some(*c),
            _ => None,
        };
        let right = match (op, items.get(i + 1).map(|it| &it.tok)) {
            (Some(_), Some(Tok::Number(raw))) => parse_plain_int(raw),
            _ => None,
        };

        if let (Some(a), Some(op), Some(b)) = (left, op, right) {
            if let Some(value) = fold_int_op(a, op, b) {
                out.pop();
                let text = value.to_string();
                out.push(Item {
                    tok: Tok::Number(text.clone()),
                    text,
                    space_before: false,
                });
                stats.constants_folded += 1;
                i += 2;
                continue;
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn fold_additive_op(a: i64, op: char, b: i64) -> Option<i64> {
    let result = match op {
        '+' => a.checked_add(b)?,
        '-' => a.checked_sub(b)?,
        _ => return None,
    };
    if result < i32::MIN as i64 || result > i32::MAX as i64 {
        return None;
    }
    Some(result)
}

fn additive_chain_boundary_ok(items: &[Item], idx: Option<usize>) -> bool {
    match idx.and_then(|i| items.get(i)).map(|it| &it.tok) {
        None => true,
        Some(Tok::Punct(c)) => !matches!(c, '*' | '/' | '%' | '+' | '-' | ')' | ']'),
        _ => false,
    }
}

fn try_extend_additive_chain(items: &[Item], first_num_idx: usize, leading_sign: i64) -> Option<(usize, i64)> {
    let first_raw = match &items[first_num_idx].tok {
        Tok::Number(raw) => raw,
        _ => return None,
    };
    let mut value = leading_sign * parse_plain_int(first_raw)?;
    let mut i = first_num_idx + 1;
    let mut terms = 1;
    while let Some(Tok::Punct(op @ ('+' | '-'))) = items.get(i).map(|it| &it.tok) {
        let op = *op;
        let term = match items.get(i + 1).map(|it| &it.tok) {
            Some(Tok::Number(raw)) => parse_plain_int(raw),
            _ => None,
        };
        let Some(term) = term else { break };
        let claimed_by_tighter_op =
            matches!(items.get(i + 2).map(|it| &it.tok), Some(Tok::Punct(c)) if matches!(c, '*' | '/' | '%'));
        if claimed_by_tighter_op {
            break;
        }
        value = fold_additive_op(value, op, term)?;
        terms += 1;
        i += 2;
    }
    if terms < 2 {
        return None;
    }
    if value < i32::MIN as i64 || value > i32::MAX as i64 {
        return None;
    }
    Some((i, value))
}

pub fn fold_additive_constants(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        let before = if i == 0 { None } else { Some(i - 1) };
        if additive_chain_boundary_ok(&items, before) {
            if matches!(items[i].tok, Tok::Number(_)) {
                if let Some((end, value)) = try_extend_additive_chain(&items, i, 1) {
                    push_folded_int(&mut out, value);
                    stats.constants_folded += 1;
                    i = end;
                    continue;
                }
            }
            if let Some(Tok::Punct(sign_c @ ('+' | '-'))) = items.get(i).map(|it| &it.tok) {
                let sign_c = *sign_c;
                if matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Number(_))) {
                    let leading_sign = if sign_c == '-' { -1 } else { 1 };
                    if let Some((end, value)) = try_extend_additive_chain(&items, i + 1, leading_sign) {
                        push_folded_int(&mut out, value);
                        stats.constants_folded += 1;
                        i = end;
                        continue;
                    }
                }
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn push_folded_int(out: &mut Vec<Item>, value: i64) {
    if value < 0 {
        out.push(Item {
            tok: Tok::Punct('-'),
            text: "-".to_string(),
            space_before: false,
        });
        let text = (-value).to_string();
        out.push(Item {
            tok: Tok::Number(text.clone()),
            text,
            space_before: false,
        });
    } else {
        let text = value.to_string();
        out.push(Item {
            tok: Tok::Number(text.clone()),
            text,
            space_before: false,
        });
    }
}

fn parse_plain_float(raw: &str) -> Option<f32> {
    if raw.is_empty() || raw.starts_with("0x") || raw.starts_with("0X") {
        return None;
    }
    if raw.contains(['e', 'E', 'f', 'F', 'u', 'U']) {
        return None;
    }
    if !raw.contains('.') {
        return None;
    }
    if !raw.chars().all(|c| c.is_ascii_digit() || c == '.') {
        return None;
    }
    raw.parse::<f32>().ok()
}

fn fold_float_op(a: f32, op: char, b: f32) -> Option<f32> {
    let result = match op {
        '+' => a + b,
        '-' => a - b,
        '*' => a * b,
        _ => return None,
    };
    if !result.is_finite() {
        return None;
    }
    Some(result)
}

fn format_folded_float(value: f32) -> Option<String> {
    if !value.is_finite() || (value == 0.0 && value.is_sign_negative()) {
        return None;
    }
    let magnitude = value.abs();
    let mut text = format!("{magnitude}");
    if !text.contains('.') && !text.contains('e') {
        text.push('.');
    }
    if text.parse::<f32>() != Ok(magnitude) {
        return None;
    }
    if let Some(sci) = shortest_scientific_form(magnitude) {
        if sci.len() < text.len() {
            text = sci;
        }
    }
    Some(text)
}

pub fn shortest_scientific_form(value: f32) -> Option<String> {
    if value == 0.0 {
        return None;
    }
    let text = format!("{value:e}");
    if text.parse::<f32>() != Ok(value) {
        return None;
    }
    Some(text)
}

fn push_folded_float(out: &mut Vec<Item>, value: f32, text: String) {
    if value.is_sign_negative() {
        out.push(Item {
            tok: Tok::Punct('-'),
            text: "-".to_string(),
            space_before: false,
        });
    }
    out.push(Item {
        tok: Tok::Number(text.clone()),
        text,
        space_before: false,
    });
}

pub fn fold_float_constants(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        let left = match out.last() {
            Some(Item {
                tok: Tok::Number(raw),
                ..
            }) => parse_plain_float(raw),
            _ => None,
        };
        let right = if left.is_some() && matches!(items.get(i).map(|it| &it.tok), Some(Tok::Punct('*'))) {
            match items.get(i + 1).map(|it| &it.tok) {
                Some(Tok::Number(raw)) => parse_plain_float(raw),
                _ => None,
            }
        } else {
            None
        };

        if let (Some(a), Some(b)) = (left, right) {
            if let Some(value) = fold_float_op(a, '*', b) {
                if let Some(text) = format_folded_float(value) {
                    out.pop();
                    push_folded_float(&mut out, value, text);
                    stats.constants_folded += 1;
                    i += 2;
                    continue;
                }
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn try_extend_additive_float_chain(items: &[Item], first_num_idx: usize, leading_sign: f32) -> Option<(usize, f32)> {
    let first_raw = match &items[first_num_idx].tok {
        Tok::Number(raw) => raw,
        _ => return None,
    };
    let mut value = leading_sign * parse_plain_float(first_raw)?;
    let mut i = first_num_idx + 1;
    let mut terms = 1;
    while let Some(Tok::Punct(op @ ('+' | '-'))) = items.get(i).map(|it| &it.tok) {
        let op = *op;
        let term = match items.get(i + 1).map(|it| &it.tok) {
            Some(Tok::Number(raw)) => parse_plain_float(raw),
            _ => None,
        };
        let Some(term) = term else { break };
        let claimed_by_tighter_op =
            matches!(items.get(i + 2).map(|it| &it.tok), Some(Tok::Punct(c)) if matches!(c, '*' | '/'));
        if claimed_by_tighter_op {
            break;
        }
        value = fold_float_op(value, op, term)?;
        terms += 1;
        i += 2;
    }
    if terms < 2 {
        return None;
    }
    Some((i, value))
}

pub fn fold_additive_float_constants(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        let before = if i == 0 { None } else { Some(i - 1) };
        if additive_chain_boundary_ok(&items, before) {
            if matches!(items[i].tok, Tok::Number(_)) {
                if let Some((end, value)) = try_extend_additive_float_chain(&items, i, 1.0) {
                    if let Some(text) = format_folded_float(value) {
                        push_folded_float(&mut out, value, text);
                        stats.constants_folded += 1;
                        i = end;
                        continue;
                    }
                }
            }
            if let Some(Tok::Punct(sign_c @ ('+' | '-'))) = items.get(i).map(|it| &it.tok) {
                let sign_c = *sign_c;
                if matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Number(_))) {
                    let leading_sign = if sign_c == '-' { -1.0 } else { 1.0 };
                    if let Some((end, value)) = try_extend_additive_float_chain(&items, i + 1, leading_sign) {
                        if let Some(text) = format_folded_float(value) {
                            push_folded_float(&mut out, value, text);
                            stats.constants_folded += 1;
                            i = end;
                            continue;
                        }
                    }
                }
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn is_ident_or_number(items: &[Item], i: usize) -> bool {
    matches!(
        items.get(i).map(|it| &it.tok),
        Some(Tok::Ident(_)) | Some(Tok::Number(_))
    )
}

fn is_ident(items: &[Item], i: usize) -> bool {
    matches!(items.get(i).map(|it| &it.tok), Some(Tok::Ident(_)))
}

fn is_numeric_value(raw: &str, target: f64) -> bool {
    if let Some(v) = parse_plain_int(raw) {
        return v as f64 == target;
    }
    if let Some(v) = parse_plain_float(raw) {
        return v as f64 == target;
    }
    false
}

/// Removes multiplicative/additive identities (`x*1`, `1*x`, `x/1`, `x+0`,
/// `0+x`, `x-0`) where `x` is a single identifier, and squares expressed via
/// `pow(x,2.)` for a single identifier/number `x`. Numeric-literal operands
/// are deliberately left to `fold_constants`/`fold_*_float_constants`, which
/// already handle the negative-zero edge cases of literal-literal folding;
/// this pass only ever duplicates or drops a bare variable read, which can
/// never have a side effect.
pub fn simplify_algebraic_identities(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        if is_ident(&items, i) {
            if let Some(Tok::Punct(op @ ('*' | '/' | '+' | '-'))) = items.get(i + 1).map(|it| &it.tok) {
                if let Some(Tok::Number(raw)) = items.get(i + 2).map(|it| &it.tok) {
                    let is_identity = match op {
                        '*' | '/' => is_numeric_value(raw, 1.0),
                        '+' | '-' => is_numeric_value(raw, 0.0),
                        _ => false,
                    };
                    if is_identity {
                        out.push(items[i].clone());
                        stats.algebraic_identities_simplified += 1;
                        i += 3;
                        continue;
                    }
                }
            }
        }

        if let Some(Tok::Number(raw)) = items.get(i).map(|it| &it.tok) {
            if let Some(Tok::Punct(op @ ('*' | '+'))) = items.get(i + 1).map(|it| &it.tok) {
                if is_ident(&items, i + 2) {
                    let is_identity = match op {
                        '*' => is_numeric_value(raw, 1.0),
                        '+' => is_numeric_value(raw, 0.0),
                        _ => false,
                    };
                    if is_identity {
                        let mut operand = items[i + 2].clone();
                        operand.space_before = items[i].space_before;
                        out.push(operand);
                        stats.algebraic_identities_simplified += 1;
                        i += 3;
                        continue;
                    }
                }
            }
        }

        let is_pow = matches!(items.get(i).map(|it| &it.tok), Some(Tok::Ident(name)) if name == "pow");
        if is_pow
            && matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Punct('(')))
            && is_ident_or_number(&items, i + 2)
            && matches!(items.get(i + 3).map(|it| &it.tok), Some(Tok::Punct(',')))
        {
            if let Some(Tok::Number(raw)) = items.get(i + 4).map(|it| &it.tok) {
                if is_numeric_value(raw, 2.0)
                    && matches!(items.get(i + 5).map(|it| &it.tok), Some(Tok::Punct(')')))
                {
                    let mut operand = items[i + 2].clone();
                    operand.space_before = items[i].space_before;
                    out.push(operand.clone());
                    out.push(Item {
                        tok: Tok::Punct('*'),
                        text: "*".to_string(),
                        space_before: false,
                    });
                    let mut second = operand;
                    second.space_before = false;
                    out.push(second);
                    stats.algebraic_identities_simplified += 1;
                    i += 6;
                    continue;
                }
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn is_pure_call_name(name: &str) -> bool {
    if is_vector_or_matrix_constructor(name) {
        return true;
    }
    matches!(
        name,
        "radians" | "degrees" | "sin" | "cos" | "tan" | "asin" | "acos" | "atan" | "sinh" | "cosh"
            | "tanh" | "asinh" | "acosh" | "atanh" | "pow" | "exp" | "log" | "exp2" | "log2"
            | "sqrt" | "inversesqrt" | "abs" | "sign" | "floor" | "trunc" | "round" | "roundEven"
            | "ceil" | "fract" | "mod" | "modf" | "min" | "max" | "clamp" | "mix" | "step"
            | "smoothstep" | "isnan" | "isinf" | "floatBitsToInt" | "floatBitsToUint"
            | "intBitsToFloat" | "uintBitsToFloat" | "fma" | "length" | "distance" | "dot"
            | "cross" | "normalize" | "faceforward" | "reflect" | "refract" | "matrixCompMult"
            | "outerProduct" | "transpose" | "determinant" | "inverse" | "lessThan"
            | "lessThanEqual" | "greaterThan" | "greaterThanEqual" | "equal" | "notEqual" | "any"
            | "all" | "not"
    )
}

fn is_vector_or_matrix_constructor(name: &str) -> bool {
    matches!(
        name,
        "float" | "int" | "uint" | "bool" | "vec2" | "vec3" | "vec4" | "ivec2" | "ivec3" | "ivec4"
            | "uvec2" | "uvec3" | "uvec4" | "bvec2" | "bvec3" | "bvec4" | "mat2" | "mat3" | "mat4"
            | "mat2x2" | "mat2x3" | "mat2x4" | "mat3x2" | "mat3x3" | "mat3x4" | "mat4x2" | "mat4x3"
            | "mat4x4"
    )
}

fn expr_is_pure(e: &Expr) -> bool {
    match &e.kind {
        ExprKind::Number(_) | ExprKind::Ident(_) => true,
        ExprKind::Unary(_, inner) | ExprKind::Paren(inner) => expr_is_pure(inner),
        ExprKind::Binary(_, l, r) => expr_is_pure(l) && expr_is_pure(r),
        ExprKind::Ternary(c, t, f) => expr_is_pure(c) && expr_is_pure(t) && expr_is_pure(f),
        ExprKind::Call(name, args) => is_pure_call_name(name) && args.iter().all(expr_is_pure),
        ExprKind::Index(b, idx) => expr_is_pure(b) && expr_is_pure(idx),
        ExprKind::Member(b, _) => expr_is_pure(b),
    }
}

struct CachedExpr {
    expr: Expr,
    type_name: String,
    var_name: String,
}

struct SubDecl {
    name_idx: usize,
    initializer: Option<(Expr, usize, usize)>,
}

fn parse_declaration_statement(items: &[Item], start: usize) -> Option<(String, Vec<SubDecl>, usize)> {
    let type_kw = type_keywords();
    let type_name = match items.get(start).map(|it| &it.tok) {
        Some(Tok::Ident(t)) if type_kw.contains(t.as_str()) => t.clone(),
        _ => return None,
    };
    if !matches!(items.get(start + 1).map(|it| &it.tok), Some(Tok::Ident(_))) {
        return None;
    }

    let mut subs: Vec<SubDecl> = Vec::new();
    let mut i = start + 1;
    loop {
        let name_idx = i;
        if !matches!(items.get(i).map(|it| &it.tok), Some(Tok::Ident(_))) {
            return None;
        }
        i += 1;
        if matches!(items.get(i).map(|it| &it.tok), Some(Tok::Punct('['))) {
            return None;
        }
        if matches!(items.get(i).map(|it| &it.tok), Some(Tok::Punct('='))) {
            let expr_start = i + 1;
            let expr = parse_expr(items, expr_start)?;
            let expr_end = expr.end;
            i = expr_end;
            subs.push(SubDecl {
                name_idx,
                initializer: Some((expr, expr_start, expr_end)),
            });
        } else {
            subs.push(SubDecl {
                name_idx,
                initializer: None,
            });
        }
        match items.get(i).map(|it| &it.tok) {
            Some(Tok::Punct(',')) => {
                i += 1;
                continue;
            }
            Some(Tok::Punct(';')) => {
                i += 1;
                break;
            }
            _ => return None,
        }
    }
    Some((type_name, subs, i))
}

/// Deduplicates declarations that initialize with a token-identical pure
/// expression by rewriting the later ones to just reference the first
/// variable, e.g. `float a=f(x),b=f(x);` -> `float a=f(x),b=a;`. Only
/// matches whole declaration-statement initializers (never a sub-expression
/// buried in a larger expression) built purely from identifiers, numbers,
/// operators, member/swizzle access, and calls to a fixed whitelist of pure
/// GLSL builtins/constructors, so nothing with a potential side effect is
/// ever assumed identical. The candidate cache is cleared on every block
/// boundary and on every statement that is not itself one of these clean
/// declarations, so a match can only ever span a straight-line run of
/// declarations with nothing else — including no plain assignment,
/// unrecognized function call, or control-flow statement -- between them.
pub fn eliminate_common_subexpressions(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut cache: Vec<CachedExpr> = Vec::new();
    let mut i = 0;

    while i < items.len() {
        if matches!(items.get(i).map(|it| &it.tok), Some(Tok::Punct('{')) | Some(Tok::Punct('}'))) {
            // Unconditional, regardless of what the previous token was: a brace can
            // follow ')' (if/for/while/function headers) or "else" just as often as
            // it follows ';', and every one of those is a scope boundary that could
            // shadow a name a cached expression depends on.
            cache.clear();
            out.push(items[i].clone());
            i += 1;
            continue;
        }

        let at_boundary = out
            .last()
            .is_none_or(|it: &Item| matches!(it.tok, Tok::Punct(';') | Tok::Punct('{') | Tok::Punct('}')));

        if at_boundary {
            if let Some((type_name, subs, end)) = parse_declaration_statement(&items, i) {
                out.push(items[i].clone());
                for (idx, sub) in subs.iter().enumerate() {
                    out.push(items[sub.name_idx].clone());
                    if let Some((expr, rhs_start, rhs_end)) = &sub.initializer {
                        out.push(items[rhs_start - 1].clone());

                        let is_pure = expr_is_pure(expr);
                        let cached_match = if is_pure {
                            cache
                                .iter()
                                .find(|c| c.type_name == type_name && c.expr.structurally_eq(expr))
                        } else {
                            None
                        };

                        if let Some(cached) = cached_match {
                            out.push(Item {
                                tok: Tok::Ident(cached.var_name.clone()),
                                text: cached.var_name.clone(),
                                space_before: false,
                            });
                            stats.common_subexpressions_eliminated += 1;
                        } else {
                            for t in *rhs_start..*rhs_end {
                                out.push(items[t].clone());
                            }
                            if is_pure {
                                // `.text` (not `.tok`'s inner identifier string, which the
                                // renaming pass deliberately leaves holding the pre-rename
                                // name for other passes' benefit) is the actual rendered
                                // name this declaration's variable will appear as in the
                                // output, and therefore the only correct thing to reference
                                // from a later duplicate.
                                let var_name = items[sub.name_idx].text.clone();
                                cache.push(CachedExpr {
                                    expr: expr.clone(),
                                    type_name: type_name.clone(),
                                    var_name,
                                });
                            }
                        }
                    }
                    if idx + 1 < subs.len() {
                        out.push(Item {
                            tok: Tok::Punct(','),
                            text: ",".to_string(),
                            space_before: false,
                        });
                    }
                }
                out.push(Item {
                    tok: Tok::Punct(';'),
                    text: ";".to_string(),
                    space_before: false,
                });
                i = end;
                continue;
            } else {
                cache.clear();
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn vec_arity(name: &str) -> Option<usize> {
    match name {
        "vec2" => Some(2),
        "vec3" => Some(3),
        "vec4" => Some(4),
        _ => None,
    }
}

fn try_match_constant_vector(items: &[Item], i: usize) -> Option<(usize, usize)> {
    let arity = vec_arity(find_ident(items, i)?)?;
    if !matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Punct('('))) {
        return None;
    }
    let mut idx = i + 2;
    let mut first_text: Option<&str> = None;
    let mut first_idx = None;
    for k in 0..arity {
        match items.get(idx).map(|it| &it.tok) {
            Some(Tok::Number(_)) => {}
            _ => return None,
        }
        let text = items[idx].text.as_str();
        match first_text {
            Some(ft) if ft != text => return None,
            Some(_) => {}
            None => {
                first_text = Some(text);
                first_idx = Some(idx);
            }
        }
        idx += 1;
        if k + 1 < arity {
            if !matches!(items.get(idx).map(|it| &it.tok), Some(Tok::Punct(','))) {
                return None;
            }
            idx += 1;
        }
    }
    if !matches!(items.get(idx).map(|it| &it.tok), Some(Tok::Punct(')'))) {
        return None;
    }
    Some((idx, first_idx.unwrap()))
}

pub fn reduce_constant_vectors(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        if let Some((close_idx, value_idx)) = try_match_constant_vector(&items, i) {
            out.push(items[i].clone());
            out.push(items[i + 1].clone());
            out.push(items[value_idx].clone());
            out.push(items[close_idx].clone());
            stats.constant_vectors_reduced += 1;
            i = close_idx + 1;
            continue;
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn void_function_body_closers(items: &[Item]) -> std::collections::HashSet<usize> {
    let mut closers = std::collections::HashSet::new();
    let mut i = 0;
    while i < items.len() {
        let is_void = matches!(&items[i].tok, Tok::Ident(s) if s == "void");
        if is_void && matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Ident(_))) {
            if let Some(Tok::Punct('(')) = items.get(i + 2).map(|it| &it.tok) {
                let mut depth = 0i32;
                let mut k = i + 2;
                loop {
                    match items.get(k).map(|it| &it.tok) {
                        Some(Tok::Punct('(')) => depth += 1,
                        Some(Tok::Punct(')')) => {
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
                if matches!(items.get(k + 1).map(|it| &it.tok), Some(Tok::Punct('{'))) {
                    let mut bd = 0i32;
                    let mut m = k + 1;
                    loop {
                        match items.get(m).map(|it| &it.tok) {
                            Some(Tok::Punct('{')) => bd += 1,
                            Some(Tok::Punct('}')) => {
                                bd -= 1;
                                if bd == 0 {
                                    closers.insert(m);
                                    break;
                                }
                            }
                            None => break,
                            _ => {}
                        }
                        m += 1;
                    }
                    i = m;
                    continue;
                }
            }
        }
        i += 1;
    }
    closers
}

fn is_statement_boundary(items: &[Item], idx: usize) -> bool {
    if idx == 0 {
        return true;
    }
    matches!(items.get(idx - 1).map(|it| &it.tok), Some(Tok::Punct(';')) | Some(Tok::Punct('{')) | Some(Tok::Punct('}')))
}

pub fn strip_trailing_void_return(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let closers = void_function_body_closers(&items);
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        let is_return = matches!(&items[i].tok, Tok::Ident(s) if s == "return");
        if is_return
            && is_statement_boundary(&items, i)
            && matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Punct(';')))
            && closers.contains(&(i + 2))
        {
            stats.trailing_void_returns_removed += 1;
            i += 2;
            continue;
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

const STATEMENT_TERMINATORS: &[char] = &[';', ',', ')', ']', '}'];

fn is_terminator(items: &[Item], idx: usize) -> bool {
    match items.get(idx).map(|it| &it.tok) {
        None => true,
        Some(Tok::Punct(c)) => STATEMENT_TERMINATORS.contains(c),
        _ => false,
    }
}

pub fn compound_assignments(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let type_kw = type_keywords();
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        let matches_pattern = if let (
            Some(Tok::Ident(a)),
            Some(Tok::Punct('=')),
            Some(Tok::Ident(a2)),
            Some(Tok::Punct(op)),
        ) = (
            items.get(i).map(|it| &it.tok),
            items.get(i + 1).map(|it| &it.tok),
            items.get(i + 2).map(|it| &it.tok),
            items.get(i + 3).map(|it| &it.tok),
        ) {
            let op = *op;
            a == a2 && matches!(op, '+' | '-' | '*' | '/' | '%')
        } else {
            false
        };

        if matches_pattern {
            let is_declarator = i > 0
                && matches!(&items[i - 1].tok, Tok::Ident(prev) if type_kw.contains(prev.as_str()));

            if !is_declarator {
                let op = match items[i + 3].tok {
                    Tok::Punct(c) => c,
                    _ => unreachable!(),
                };
                if let Some(end) = scan_primary(&items, i + 4) {
                    if is_terminator(&items, end) {
                        out.push(items[i].clone());
                        out.push(Item {
                            tok: Tok::Punct(op),
                            text: op.to_string(),
                            space_before: false,
                        });
                        out.push(Item {
                            tok: Tok::Punct('='),
                            text: "=".to_string(),
                            space_before: false,
                        });
                        out.extend_from_slice(&items[i + 4..end]);
                        stats.compound_assignments += 1;
                        i = end;
                        continue;
                    }
                }
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

pub fn increment_decrement(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        let op = match (
            items.get(i).map(|it| &it.tok),
            items.get(i + 1).map(|it| &it.tok),
            items.get(i + 2).map(|it| &it.tok),
            items.get(i + 3).map(|it| &it.tok),
        ) {
            (Some(Tok::Ident(_)), Some(Tok::Punct(op @ ('+' | '-'))), Some(Tok::Punct('=')), Some(Tok::Number(_))) => Some(*op),
            _ => None,
        };

        if let Some(op) = op {
            let value_text = items[i + 3].text.as_str();
            if (value_text == "1" || value_text == "1.") && is_terminator(&items, i + 4) {
                out.push(Item {
                    tok: Tok::Punct(op),
                    text: op.to_string(),
                    space_before: items[i].space_before,
                });
                out.push(Item {
                    tok: Tok::Punct(op),
                    text: op.to_string(),
                    space_before: false,
                });
                out.push(Item {
                    tok: items[i].tok.clone(),
                    text: items[i].text.clone(),
                    space_before: false,
                });
                stats.increments_decrements += 1;
                i += 4;
                continue;
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

struct TernaryMatch {
    end: usize,
    ident_idx: usize,
    cond: (usize, usize),
    x: (usize, usize),
    y: (usize, usize),
}

fn try_match_ternary(items: &[Item], i: usize) -> Option<TernaryMatch> {
    if !matches!(&items.get(i)?.tok, Tok::Ident(s) if s == "if") {
        return None;
    }
    if !is_statement_boundary(items, i) {
        return None;
    }
    if !matches!(items.get(i + 1).map(|it| &it.tok), Some(Tok::Punct('('))) {
        return None;
    }
    let cond_start = i + 2;
    let after_paren = skip_balanced(items, i + 1, '(', ')')?;
    let cond_end = after_paren - 1;

    let mut j = after_paren;
    let braced1 = matches!(items.get(j).map(|it| &it.tok), Some(Tok::Punct('{')));
    if braced1 {
        j += 1;
    }
    let ident_idx = j;
    let name1 = match items.get(ident_idx).map(|it| &it.tok) {
        Some(Tok::Ident(s)) => s.clone(),
        _ => return None,
    };
    if !matches!(items.get(ident_idx + 1).map(|it| &it.tok), Some(Tok::Punct('='))) {
        return None;
    }
    if matches!(items.get(ident_idx + 2).map(|it| &it.tok), Some(Tok::Punct('='))) {
        return None;
    }
    let x_start = ident_idx + 2;
    let x_end = scan_primary(items, x_start)?;
    if !matches!(items.get(x_end).map(|it| &it.tok), Some(Tok::Punct(';'))) {
        return None;
    }
    let mut k = x_end + 1;
    if braced1 {
        if !matches!(items.get(k).map(|it| &it.tok), Some(Tok::Punct('}'))) {
            return None;
        }
        k += 1;
    }

    if !matches!(&items.get(k)?.tok, Tok::Ident(s) if s == "else") {
        return None;
    }
    k += 1;
    let braced2 = matches!(items.get(k).map(|it| &it.tok), Some(Tok::Punct('{')));
    if braced2 {
        k += 1;
    }
    let ident2_idx = k;
    match items.get(ident2_idx).map(|it| &it.tok) {
        Some(Tok::Ident(s)) if *s == name1 => {}
        _ => return None,
    }
    if !matches!(items.get(ident2_idx + 1).map(|it| &it.tok), Some(Tok::Punct('='))) {
        return None;
    }
    if matches!(items.get(ident2_idx + 2).map(|it| &it.tok), Some(Tok::Punct('='))) {
        return None;
    }
    let y_start = ident2_idx + 2;
    let y_end = scan_primary(items, y_start)?;
    if !matches!(items.get(y_end).map(|it| &it.tok), Some(Tok::Punct(';'))) {
        return None;
    }
    let mut end = y_end + 1;
    if braced2 {
        if !matches!(items.get(end).map(|it| &it.tok), Some(Tok::Punct('}'))) {
            return None;
        }
        end += 1;
    }

    Some(TernaryMatch { end, ident_idx, cond: (cond_start, cond_end), x: (x_start, x_end), y: (y_start, y_end) })
}

pub fn ternary_from_if_else(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        if let Some(m) = try_match_ternary(&items, i) {
            out.push(items[m.ident_idx].clone());
            out.push(Item { tok: Tok::Punct('='), text: "=".to_string(), space_before: false });
            out.push(Item { tok: Tok::Punct('('), text: "(".to_string(), space_before: false });
            out.extend_from_slice(&items[m.cond.0..m.cond.1]);
            out.push(Item { tok: Tok::Punct(')'), text: ")".to_string(), space_before: false });
            out.push(Item { tok: Tok::Punct('?'), text: "?".to_string(), space_before: false });
            out.extend_from_slice(&items[m.x.0..m.x.1]);
            out.push(Item { tok: Tok::Punct(':'), text: ":".to_string(), space_before: false });
            out.extend_from_slice(&items[m.y.0..m.y.1]);
            out.push(Item { tok: Tok::Punct(';'), text: ";".to_string(), space_before: false });
            stats.ternaries_from_if_else += 1;
            i = m.end;
            continue;
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

pub fn merge_declarations(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let type_kw = type_keywords();
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut pending_type: Option<String> = None;
    let mut i = 0;

    while i < items.len() {
        let at_boundary = out
            .last()
            .is_none_or(|it: &Item| matches!(it.tok, Tok::Punct(';') | Tok::Punct('{') | Tok::Punct('}')));

        if at_boundary {
            let decl_start = if let (Some(Tok::Ident(t)), Some(Tok::Ident(_))) = (
                items.get(i).map(|it| &it.tok),
                items.get(i + 1).map(|it| &it.tok),
            ) {
                if type_kw.contains(t.as_str()) {
                    Some(t.clone())
                } else {
                    None
                }
            } else {
                None
            };

            if let Some(t) = decl_start {
                let can_merge = pending_type.as_deref() == Some(t.as_str())
                    && matches!(out.last().map(|it| &it.tok), Some(Tok::Punct(';')));
                if can_merge {
                    out.pop();
                    out.push(Item {
                        tok: Tok::Punct(','),
                        text: ",".to_string(),
                        space_before: false,
                    });
                    stats.declarations_merged += 1;
                    i += 1;
                    continue;
                } else {
                    pending_type = Some(t);
                    out.push(items[i].clone());
                    i += 1;
                    continue;
                }
            } else {
                pending_type = None;
            }
        }

        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn scan_statement(items: &[Item], start: usize) -> Option<usize> {
    match find_ident(items, start) {
        Some("if") => {
            let paren_end = skip_balanced(items, start + 1, '(', ')')?;
            let then_end = scan_statement(items, paren_end)?;
            if find_ident(items, then_end) == Some("else") {
                scan_statement(items, then_end + 1)
            } else {
                Some(then_end)
            }
        }
        Some("for") | Some("while") => {
            let paren_end = skip_balanced(items, start + 1, '(', ')')?;
            scan_statement(items, paren_end)
        }
        Some("do") => {
            let body_end = scan_statement(items, start + 1)?;
            if find_ident(items, body_end) != Some("while") {
                return None;
            }
            let paren_end = skip_balanced(items, body_end + 1, '(', ')')?;
            match items.get(paren_end).map(|it| &it.tok) {
                Some(Tok::Punct(';')) => Some(paren_end + 1),
                _ => None,
            }
        }
        _ => match items.get(start).map(|it| &it.tok) {
            Some(Tok::Punct('{')) => skip_balanced(items, start, '{', '}'),
            Some(Tok::Punct(';')) => Some(start + 1),
            None => None,
            _ => {
                let mut i = start;
                let mut depth = 0i32;
                loop {
                    match items.get(i).map(|it| &it.tok) {
                        None => return None,
                        Some(Tok::Punct('(')) | Some(Tok::Punct('[')) => depth += 1,
                        Some(Tok::Punct(')')) | Some(Tok::Punct(']')) => depth -= 1,
                        Some(Tok::Punct('{')) | Some(Tok::Punct('}')) => return None,
                        Some(Tok::Punct(';')) if depth == 0 => return Some(i + 1),
                        _ => {}
                    }
                    i += 1;
                }
            }
        },
    }
}

fn is_hungry(items: &[Item], start: usize) -> bool {
    match find_ident(items, start) {
        Some("if") => {
            let paren_end = match skip_balanced(items, start + 1, '(', ')') {
                Some(v) => v,
                None => return false,
            };
            let then_end = match scan_statement(items, paren_end) {
                Some(v) => v,
                None => return false,
            };
            if find_ident(items, then_end) == Some("else") {
                is_hungry(items, then_end + 1)
            } else {
                true
            }
        }
        Some("for") | Some("while") => match skip_balanced(items, start + 1, '(', ')') {
            Some(paren_end) => is_hungry(items, paren_end),
            None => false,
        },
        _ => false,
    }
}

fn looks_like_declaration(items: &[Item], start: usize) -> bool {
    let type_kw = type_keywords();
    match (
        items.get(start).map(|it| &it.tok),
        items.get(start + 1).map(|it| &it.tok),
    ) {
        (Some(Tok::Ident(a)), Some(Tok::Ident(_))) => {
            type_kw.contains(a.as_str()) || matches!(a.as_str(), "const" | "highp" | "mediump" | "lowp")
        }
        _ => false,
    }
}

fn rewrite_control_body(
    items: &[Item],
    body_start: usize,
    body_end: usize,
    has_trailing_else: bool,
    stats: &mut AggressiveStats,
) -> Option<Vec<Item>> {
    if matches!(items.get(body_start).map(|it| &it.tok), Some(Tok::Punct('{'))) {
        let inner_start = body_start + 1;
        let inner_end = body_end - 1;
        let single = inner_start < inner_end && scan_statement(items, inner_start) == Some(inner_end);
        if single {
            let is_decl = looks_like_declaration(items, inner_start);
            let unsafe_hungry = has_trailing_else && is_hungry(items, inner_start);
            if !is_decl && !unsafe_hungry {
                stats.braces_removed += 1;
                let (toks, _) = rewrite_body(items, inner_start, stats)?;
                return Some(toks);
            }
        }
        let mut out = vec![items[body_start].clone()];
        out.extend(rewrite_sequence(items, inner_start, inner_end, stats));
        out.push(items[inner_end].clone());
        Some(out)
    } else {
        let (toks, _) = rewrite_body(items, body_start, stats)?;
        Some(toks)
    }
}

fn rewrite_body(items: &[Item], start: usize, stats: &mut AggressiveStats) -> Option<(Vec<Item>, usize)> {
    match find_ident(items, start) {
        Some("if") => {
            let mut out = vec![items[start].clone()];
            let paren_end = skip_balanced(items, start + 1, '(', ')')?;
            out.extend_from_slice(&items[start + 1..paren_end]);
            let then_end = scan_statement(items, paren_end)?;
            let has_else = find_ident(items, then_end) == Some("else");
            out.extend(rewrite_control_body(items, paren_end, then_end, has_else, stats)?);
            let mut i = then_end;
            if has_else {
                out.push(items[i].clone());
                let else_start = i + 1;
                let else_end = scan_statement(items, else_start)?;
                out.extend(rewrite_control_body(items, else_start, else_end, false, stats)?);
                i = else_end;
            }
            Some((out, i))
        }
        Some("for") | Some("while") => {
            let mut out = vec![items[start].clone()];
            let paren_end = skip_balanced(items, start + 1, '(', ')')?;
            out.extend_from_slice(&items[start + 1..paren_end]);
            let body_end = scan_statement(items, paren_end)?;
            out.extend(rewrite_control_body(items, paren_end, body_end, false, stats)?);
            Some((out, body_end))
        }
        Some("do") => {
            let mut out = vec![items[start].clone()];
            let body_end = scan_statement(items, start + 1)?;
            out.extend(rewrite_control_body(items, start + 1, body_end, false, stats)?);
            let mut i = body_end;
            if find_ident(items, i) != Some("while") {
                return None;
            }
            out.push(items[i].clone());
            let paren_end = skip_balanced(items, i + 1, '(', ')')?;
            out.extend_from_slice(&items[i + 1..paren_end]);
            i = paren_end;
            match items.get(i).map(|it| &it.tok) {
                Some(Tok::Punct(';')) => {
                    out.push(items[i].clone());
                    Some((out, i + 1))
                }
                _ => None,
            }
        }
        _ => match items.get(start).map(|it| &it.tok) {
            Some(Tok::Punct('{')) => {
                let close = skip_balanced(items, start, '{', '}')?;
                let mut out = vec![items[start].clone()];
                out.extend(rewrite_sequence(items, start + 1, close - 1, stats));
                out.push(items[close - 1].clone());
                Some((out, close))
            }
            _ => {
                let end = scan_statement(items, start)?;
                Some((items[start..end].to_vec(), end))
            }
        },
    }
}

fn rewrite_sequence(items: &[Item], start: usize, end: usize, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out = Vec::new();
    let mut i = start;
    while i < end {
        match rewrite_body(items, i, stats) {
            Some((toks, next)) if next > i && next <= end => {
                out.extend(toks);
                i = next;
            }
            _ => {
                out.extend_from_slice(&items[i..end]);
                break;
            }
        }
    }
    out
}

pub fn strip_redundant_braces(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        if matches!(items[i].tok, Tok::Punct('{')) {
            if let Some(close) = skip_balanced(&items, i, '{', '}') {
                out.push(items[i].clone());
                out.extend(rewrite_sequence(&items, i + 1, close - 1, stats));
                out.push(items[close - 1].clone());
                i = close;
                continue;
            }
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

pub fn strip_redundant_parens(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut i = 0;
    while i < items.len() {
        if matches!(items[i].tok, Tok::Punct('(')) {
            let preceded_by_ident = matches!(out.last().map(|it| &it.tok), Some(Tok::Ident(_)));
            if !preceded_by_ident {
                if let Some(close) = skip_balanced(&items, i, '(', ')') {
                    let close_paren_idx = close - 1;
                    if let Some(inner_end) = scan_primary(&items, i + 1) {
                        if inner_end == close_paren_idx {
                            let mut inner: Vec<Item> = items[i + 1..close_paren_idx].to_vec();
                            if let Some(first) = inner.first_mut() {
                                first.space_before = true;
                            }
                            out.extend(inner);
                            stats.redundant_parens_removed += 1;
                            i = close;
                            continue;
                        }
                    }
                }
            }
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

pub fn eliminate_dead_functions(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let defs = crate::callgraph::find_function_definitions(&items);
    if defs.is_empty() {
        return items;
    }

    let names: HashSet<String> = defs.iter().map(|d| d.name.clone()).collect();
    let roots: Vec<String> = ["main", "mainImage"]
        .into_iter()
        .filter(|r| names.contains(*r))
        .map(|r| r.to_string())
        .collect();
    if roots.is_empty() {
        return items;
    }

    let graph = crate::callgraph::CallGraph::build(&items, &defs, &names);
    let reachable = graph.reachable_from(&roots);

    let mut dead_ranges: Vec<(usize, usize)> = defs
        .iter()
        .filter(|d| !reachable.contains(&d.name))
        .map(|d| (d.def_start, d.body_close))
        .collect();
    if dead_ranges.is_empty() {
        return items;
    }
    dead_ranges.sort_unstable();

    let mut out = Vec::with_capacity(items.len());
    let mut i = 0;
    let mut dead_iter = dead_ranges.iter().peekable();
    while i < items.len() {
        if let Some((start, end)) = dead_iter.peek() {
            if *start == i {
                stats.dead_functions_removed += 1;
                i = end + 1;
                dead_iter.next();
                continue;
            }
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}

fn match_precision_statement(items: &[Item], i: usize) -> Option<usize> {
    if items.get(i).map(|it| it.text.as_str()) != Some("precision") {
        return None;
    }
    let qualifier_ok = matches!(
        items.get(i + 1).map(|it| it.text.as_str()),
        Some("highp" | "mediump" | "lowp")
    );
    if !qualifier_ok {
        return None;
    }
    if !matches!(items.get(i + 2).map(|it| &it.tok), Some(Tok::Ident(_))) {
        return None;
    }
    if !matches!(items.get(i + 3).map(|it| &it.tok), Some(Tok::Punct(';'))) {
        return None;
    }
    Some(i + 4)
}

pub fn strip_duplicate_precision(items: Vec<Item>, stats: &mut AggressiveStats) -> Vec<Item> {
    let mut out: Vec<Item> = Vec::with_capacity(items.len());
    let mut seen: std::collections::HashSet<(String, String)> = std::collections::HashSet::new();
    let mut i = 0;
    while i < items.len() {
        if let Some(end) = match_precision_statement(&items, i) {
            let qualifier = items[i + 1].text.clone();
            let ty = items[i + 2].text.clone();
            if !seen.insert((qualifier, ty)) {
                stats.duplicate_precision_removed += 1;
                i = end;
                continue;
            }
        }
        out.push(items[i].clone());
        i += 1;
    }
    out
}
