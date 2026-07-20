#![allow(dead_code)]

use crate::aggressive::{is_unary_prefix, skip_balanced, Item};
use crate::lexer::Tok;

#[derive(Debug, Clone, PartialEq)]
pub(crate) enum ExprKind {
    Number(String),
    Ident(String),
    Unary(char, Box<Expr>),
    Binary(String, Box<Expr>, Box<Expr>),
    Ternary(Box<Expr>, Box<Expr>, Box<Expr>),
    Call(String, Vec<Expr>),
    Index(Box<Expr>, Box<Expr>),
    Member(Box<Expr>, String),
    Paren(Box<Expr>),
}

#[derive(Debug, Clone, PartialEq)]
pub(crate) struct Expr {
    pub kind: ExprKind,
    pub start: usize,
    pub end: usize,
}

impl Expr {
    pub(crate) fn structurally_eq(&self, other: &Expr) -> bool {
        self.kind.structurally_eq(&other.kind)
    }
}

impl ExprKind {
    fn structurally_eq(&self, other: &ExprKind) -> bool {
        use ExprKind::*;
        if let Paren(inner) = self {
            return inner.kind.structurally_eq(other);
        }
        if let Paren(inner) = other {
            return self.structurally_eq(&inner.kind);
        }
        match (self, other) {
            (Number(a), Number(b)) => a == b,
            (Ident(a), Ident(b)) => a == b,
            (Unary(op_a, a), Unary(op_b, b)) => op_a == op_b && a.kind.structurally_eq(&b.kind),
            (Binary(op_a, a1, a2), Binary(op_b, b1, b2)) => {
                op_a == op_b && a1.kind.structurally_eq(&b1.kind) && a2.kind.structurally_eq(&b2.kind)
            }
            (Ternary(c1, t1, e1), Ternary(c2, t2, e2)) => {
                c1.kind.structurally_eq(&c2.kind) && t1.kind.structurally_eq(&t2.kind) && e1.kind.structurally_eq(&e2.kind)
            }
            (Call(n1, a1), Call(n2, a2)) => {
                n1 == n2 && a1.len() == a2.len() && a1.iter().zip(a2).all(|(x, y)| x.kind.structurally_eq(&y.kind))
            }
            (Index(b1, i1), Index(b2, i2)) => b1.kind.structurally_eq(&b2.kind) && i1.kind.structurally_eq(&i2.kind),
            (Member(b1, f1), Member(b2, f2)) => f1 == f2 && b1.kind.structurally_eq(&b2.kind),
            _ => false,
        }
    }
}

pub(crate) fn parse_expr(items: &[Item], start: usize) -> Option<Expr> {
    parse_ternary(items, start)
}

fn parse_ternary(items: &[Item], start: usize) -> Option<Expr> {
    let cond = parse_binary(items, start, 0)?;
    if !matches!(items.get(cond.end).map(|it| &it.tok), Some(Tok::Punct('?'))) {
        return Some(cond);
    }
    let then_branch = parse_ternary(items, cond.end + 1)?;
    if !matches!(items.get(then_branch.end).map(|it| &it.tok), Some(Tok::Punct(':'))) {
        return None;
    }
    let else_branch = parse_ternary(items, then_branch.end + 1)?;
    let end = else_branch.end;
    Some(Expr {
        start,
        end,
        kind: ExprKind::Ternary(Box::new(cond), Box::new(then_branch), Box::new(else_branch)),
    })
}

fn two_char_prec(op: &str) -> Option<u8> {
    Some(match op {
        "||" => 1,
        "&&" => 2,
        "==" | "!=" => 6,
        "<=" | ">=" => 7,
        "<<" | ">>" => 8,
        _ => return None,
    })
}

fn single_char_prec(c: char) -> Option<u8> {
    Some(match c {
        '|' => 3,
        '^' => 4,
        '&' => 5,
        '<' | '>' => 7,
        '+' | '-' => 9,
        '*' | '/' | '%' => 10,
        _ => return None,
    })
}

fn binary_op_at(items: &[Item], i: usize) -> Option<(String, usize, u8)> {
    let c1 = match items.get(i).map(|it| &it.tok) {
        Some(Tok::Punct(c)) => *c,
        _ => return None,
    };
    if let Some(next_item) = items.get(i + 1) {
        if !next_item.space_before {
            if let Tok::Punct(c2) = next_item.tok {
                let two: String = [c1, c2].iter().collect();
                if let Some(prec) = two_char_prec(&two) {
                    return Some((two, i + 2, prec));
                }
                if two == "++" || two == "--" {
                    return None;
                }
            }
        }
    }
    single_char_prec(c1).map(|prec| (c1.to_string(), i + 1, prec))
}

fn parse_binary(items: &[Item], start: usize, min_prec: u8) -> Option<Expr> {
    let mut lhs = parse_unary(items, start)?;
    while let Some((op, next, prec)) = binary_op_at(items, lhs.end) {
        if prec < min_prec {
            break;
        }
        let Some(rhs) = parse_binary(items, next, prec + 1) else {
            break;
        };
        let end = rhs.end;
        lhs = Expr {
            start,
            end,
            kind: ExprKind::Binary(op, Box::new(lhs), Box::new(rhs)),
        };
    }
    Some(lhs)
}

fn is_prefix_incdec(items: &[Item], i: usize) -> bool {
    let c1 = match items.get(i).map(|it| &it.tok) {
        Some(Tok::Punct(c)) if *c == '+' || *c == '-' => *c,
        _ => return false,
    };
    match items.get(i + 1) {
        Some(it) if !it.space_before => matches!(it.tok, Tok::Punct(c2) if c2 == c1),
        _ => false,
    }
}

fn parse_unary(items: &[Item], start: usize) -> Option<Expr> {
    if is_prefix_incdec(items, start) {
        return None;
    }
    if let Some(Tok::Punct(c)) = items.get(start).map(|it| &it.tok) {
        if is_unary_prefix(*c) {
            let operand = parse_unary(items, start + 1)?;
            let end = operand.end;
            return Some(Expr {
                start,
                end,
                kind: ExprKind::Unary(*c, Box::new(operand)),
            });
        }
    }
    parse_postfix(items, start)
}

fn parse_postfix(items: &[Item], start: usize) -> Option<Expr> {
    let mut e = parse_primary(items, start)?;
    loop {
        match items.get(e.end).map(|it| &it.tok) {
            Some(Tok::Punct('.')) => {
                let name = match items.get(e.end + 1).map(|it| &it.tok) {
                    Some(Tok::Ident(name)) => name.clone(),
                    _ => break,
                };
                let end = e.end + 2;
                e = Expr {
                    start,
                    end,
                    kind: ExprKind::Member(Box::new(e), name),
                };
            }
            Some(Tok::Punct('[')) => {
                let close = skip_balanced(items, e.end, '[', ']')?;
                let index_expr = parse_expr(items, e.end + 1)?;
                if index_expr.end != close - 1 {
                    return None;
                }
                e = Expr {
                    start,
                    end: close,
                    kind: ExprKind::Index(Box::new(e), Box::new(index_expr)),
                };
            }
            Some(Tok::Punct('(')) => {
                let name = match &e.kind {
                    ExprKind::Ident(n) => n.clone(),
                    _ => break,
                };
                let close = skip_balanced(items, e.end, '(', ')')?;
                let args = parse_arg_list(items, e.end + 1, close - 1)?;
                e = Expr {
                    start,
                    end: close,
                    kind: ExprKind::Call(name, args),
                };
            }
            _ => break,
        }
    }
    Some(e)
}

pub(crate) fn parse_arg_list(items: &[Item], start: usize, end_before: usize) -> Option<Vec<Expr>> {
    let mut args = Vec::new();
    if start == end_before {
        return Some(args);
    }
    let mut i = start;
    loop {
        let e = parse_expr(items, i)?;
        i = e.end;
        args.push(e);
        match items.get(i).map(|it| &it.tok) {
            Some(Tok::Punct(',')) => i += 1,
            _ => break,
        }
    }
    if i != end_before {
        return None;
    }
    Some(args)
}

fn parse_primary(items: &[Item], start: usize) -> Option<Expr> {
    match items.get(start).map(|it| &it.tok) {
        Some(Tok::Ident(name)) => Some(Expr {
            start,
            end: start + 1,
            kind: ExprKind::Ident(name.clone()),
        }),
        Some(Tok::Number(text)) => Some(Expr {
            start,
            end: start + 1,
            kind: ExprKind::Number(text.clone()),
        }),
        Some(Tok::Punct('(')) => {
            let close = skip_balanced(items, start, '(', ')')?;
            let inner = parse_expr(items, start + 1)?;
            if inner.end != close - 1 {
                return None;
            }
            Some(Expr {
                start,
                end: close,
                kind: ExprKind::Paren(Box::new(inner)),
            })
        }
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lexer::tokenize_spaced;

    fn items_from(src: &str) -> Vec<Item> {
        tokenize_spaced(src)
            .into_iter()
            .map(|(tok, space_before)| {
                let text = match &tok {
                    Tok::Ident(s) | Tok::Number(s) | Tok::Preproc(s) => s.clone(),
                    Tok::Punct(c) => c.to_string(),
                };
                Item { tok, text, space_before }
            })
            .collect()
    }

    fn parse(src: &str) -> Expr {
        let items = items_from(src);
        parse_expr(&items, 0).unwrap_or_else(|| panic!("expected a parse for {src:?}"))
    }

    #[test]
    fn parses_a_bare_identifier_and_number() {
        assert_eq!(parse("x").kind, ExprKind::Ident("x".to_string()));
        assert_eq!(parse("1.5").kind, ExprKind::Number("1.5".to_string()));
    }

    #[test]
    fn full_span_covers_the_whole_input() {
        let items = items_from("a+b*c");
        let e = parse_expr(&items, 0).unwrap();
        assert_eq!((e.start, e.end), (0, items.len()));
    }

    #[test]
    fn multiplication_binds_tighter_than_addition() {
        let e = parse("2+3*4");
        match e.kind {
            ExprKind::Binary(op, lhs, rhs) => {
                assert_eq!(op, "+");
                assert_eq!(lhs.kind, ExprKind::Number("2".to_string()));
                match rhs.kind {
                    ExprKind::Binary(op2, l2, r2) => {
                        assert_eq!(op2, "*");
                        assert_eq!(l2.kind, ExprKind::Number("3".to_string()));
                        assert_eq!(r2.kind, ExprKind::Number("4".to_string()));
                    }
                    other => panic!("expected nested multiplication, got {other:?}"),
                }
            }
            other => panic!("expected top-level addition, got {other:?}"),
        }
    }

    #[test]
    fn additive_chain_is_left_associative() {
        let e = parse("2*3+4");
        match e.kind {
            ExprKind::Binary(op, lhs, rhs) => {
                assert_eq!(op, "+");
                assert_eq!(rhs.kind, ExprKind::Number("4".to_string()));
                assert!(matches!(lhs.kind, ExprKind::Binary(ref o, _, _) if o == "*"));
            }
            other => panic!("expected top-level addition, got {other:?}"),
        }
    }

    #[test]
    fn subtraction_chain_is_left_associative() {
        let e = parse("a-b-c");
        match e.kind {
            ExprKind::Binary(op, lhs, rhs) => {
                assert_eq!(op, "-");
                assert_eq!(rhs.kind, ExprKind::Ident("c".to_string()));
                match lhs.kind {
                    ExprKind::Binary(op2, l2, r2) => {
                        assert_eq!(op2, "-");
                        assert_eq!(l2.kind, ExprKind::Ident("a".to_string()));
                        assert_eq!(r2.kind, ExprKind::Ident("b".to_string()));
                    }
                    other => panic!("expected (a-b) on the left, got {other:?}"),
                }
            }
            other => panic!("expected top-level subtraction, got {other:?}"),
        }
    }

    #[test]
    fn two_char_operators_are_recognized() {
        for (src, op) in [("a==b", "=="), ("a!=b", "!="), ("a<=b", "<="), ("a>=b", ">="), ("a&&b", "&&"), ("a||b", "||"), ("a<<b", "<<"), ("a>>b", ">>")] {
            match parse(src).kind {
                ExprKind::Binary(got, _, _) => assert_eq!(got, op, "for {src:?}"),
                other => panic!("expected a binary {op} for {src:?}, got {other:?}"),
            }
        }
    }

    #[test]
    fn logical_or_binds_loosest_of_the_ones_tested() {
        let e = parse("a||b&&c");
        match e.kind {
            ExprKind::Binary(op, lhs, rhs) => {
                assert_eq!(op, "||");
                assert_eq!(lhs.kind, ExprKind::Ident("a".to_string()));
                assert!(matches!(rhs.kind, ExprKind::Binary(ref o, _, _) if o == "&&"));
            }
            other => panic!("expected top-level ||, got {other:?}"),
        }
    }

    #[test]
    fn ternary_is_right_associative() {
        let e = parse("a?b:c?d:e");
        match e.kind {
            ExprKind::Ternary(_, then_branch, else_branch) => {
                assert_eq!(then_branch.kind, ExprKind::Ident("b".to_string()));
                assert!(matches!(else_branch.kind, ExprKind::Ternary(_, _, _)));
            }
            other => panic!("expected a top-level ternary, got {other:?}"),
        }
    }

    #[test]
    fn parens_are_preserved_as_a_node_but_dont_change_the_tree_shape_otherwise() {
        let e = parse("(a+b)*c");
        match e.kind {
            ExprKind::Binary(op, lhs, rhs) => {
                assert_eq!(op, "*");
                assert_eq!(rhs.kind, ExprKind::Ident("c".to_string()));
                match lhs.kind {
                    ExprKind::Paren(inner) => assert!(matches!(inner.kind, ExprKind::Binary(ref o, _, _) if o == "+")),
                    other => panic!("expected a Paren wrapping the addition, got {other:?}"),
                }
            }
            other => panic!("expected top-level multiplication, got {other:?}"),
        }
    }

    #[test]
    fn member_index_and_call_chains_parse_left_to_right() {
        let e = parse("foo(a)[0].xy");
        match e.kind {
            ExprKind::Member(base, field) => {
                assert_eq!(field, "xy");
                match base.kind {
                    ExprKind::Index(base2, idx) => {
                        assert_eq!(idx.kind, ExprKind::Number("0".to_string()));
                        match base2.kind {
                            ExprKind::Call(name, args) => {
                                assert_eq!(name, "foo");
                                assert_eq!(args.len(), 1);
                                assert_eq!(args[0].kind, ExprKind::Ident("a".to_string()));
                            }
                            other => panic!("expected a call at the base, got {other:?}"),
                        }
                    }
                    other => panic!("expected an index, got {other:?}"),
                }
            }
            other => panic!("expected a top-level member access, got {other:?}"),
        }
    }

    #[test]
    fn call_with_multiple_args_and_nested_calls() {
        let e = parse("mix(a,vec3(1.,2.,3.),t)");
        match e.kind {
            ExprKind::Call(name, args) => {
                assert_eq!(name, "mix");
                assert_eq!(args.len(), 3);
                assert!(matches!(&args[1].kind, ExprKind::Call(n, a) if n == "vec3" && a.len() == 3));
            }
            other => panic!("expected a call, got {other:?}"),
        }
    }

    #[test]
    fn unary_prefix_chains() {
        let e = parse("-!x");
        match e.kind {
            ExprKind::Unary(c1, inner) => {
                assert_eq!(c1, '-');
                assert!(matches!(inner.kind, ExprKind::Unary('!', _)));
            }
            other => panic!("expected a unary chain, got {other:?}"),
        }
    }

    #[test]
    fn double_unary_minus_with_space_is_not_a_decrement() {
        let e = parse("- -x");
        assert!(matches!(e.kind, ExprKind::Unary('-', _)));
    }

    #[test]
    fn prefix_increment_is_declined_entirely() {
        let items = items_from("--x");
        assert!(parse_expr(&items, 0).is_none());
    }

    #[test]
    fn postfix_increment_stops_the_parse_before_it_instead_of_misreading_it() {
        let items = items_from("x++ + 1");
        let e = parse_expr(&items, 0).unwrap();
        assert_eq!(e.kind, ExprKind::Ident("x".to_string()));
        assert_eq!(e.end, 1);
    }

    #[test]
    fn unmatched_parenthesis_is_declined_not_guessed_at() {
        let items = items_from("(a+b");
        assert!(parse_expr(&items, 0).is_none());
    }

    #[test]
    fn trailing_dangling_operator_is_declined() {
        let items = items_from("a+");
        let e = parse_expr(&items, 0).unwrap();
        assert_ne!(e.end, items.len());
    }

    #[test]
    fn structural_equality_ignores_spans_and_parens() {
        let a = parse("a+b");
        let b = parse("(a+b)");
        assert!(a.structurally_eq(&b));
        let c = parse("a+c");
        assert!(!a.structurally_eq(&c));
    }

    #[test]
    fn empty_and_garbage_input_never_panics() {
        let items = items_from("");
        assert!(parse_expr(&items, 0).is_none());
        let items = items_from(")]}{,;?:");
        for i in 0..items.len() {
            let _ = parse_expr(&items, i);
        }
    }
}
