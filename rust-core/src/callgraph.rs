use crate::aggressive::{skip_balanced, Item};
use crate::lexer::Tok;
use std::collections::{HashMap, HashSet};

pub(crate) struct FunctionDef {
    pub name: String,
    pub def_start: usize,
    pub body_close: usize,
}

fn matching_open_paren(items: &[Item], close_paren: usize) -> Option<usize> {
    if !matches!(items.get(close_paren).map(|it| &it.tok), Some(Tok::Punct(')'))) {
        return None;
    }
    let mut depth = 0i32;
    let mut i = close_paren;
    loop {
        match items.get(i).map(|it| &it.tok) {
            Some(Tok::Punct(')')) => depth += 1,
            Some(Tok::Punct('(')) => {
                depth -= 1;
                if depth == 0 {
                    return Some(i);
                }
            }
            None => return None,
            _ => {}
        }
        if i == 0 {
            return None;
        }
        i -= 1;
    }
}

pub(crate) fn find_function_definitions(items: &[Item]) -> Vec<FunctionDef> {
    let mut defs = Vec::new();
    let mut i = 0;
    while i < items.len() {
        if matches!(items[i].tok, Tok::Punct('{')) {
            if let Some(close) = skip_balanced(items, i, '{', '}') {
                let body_close = close - 1;
                if i >= 1 {
                    if let Some(open_paren) = matching_open_paren(items, i - 1) {
                        if open_paren >= 2 {
                            let name_idx = open_paren - 1;
                            let type_idx = open_paren - 2;
                            if let (Tok::Ident(name), Tok::Ident(_)) = (&items[name_idx].tok, &items[type_idx].tok) {
                                defs.push(FunctionDef {
                                    name: name.clone(),
                                    def_start: type_idx,
                                    body_close,
                                });
                            }
                        }
                    }
                }
                i = close;
                continue;
            }
        }
        i += 1;
    }
    defs
}

pub(crate) struct CallGraph {
    edges: HashMap<String, HashMap<String, usize>>,
}

impl CallGraph {
    pub(crate) fn build(items: &[Item], defs: &[FunctionDef], names: &HashSet<String>) -> Self {
        let mut edges: HashMap<String, HashMap<String, usize>> = HashMap::new();
        for def in defs {
            let entry = edges.entry(def.name.clone()).or_default();
            let own_name_idx = def.def_start + 1;
            for (idx, item) in items[def.def_start..=def.body_close].iter().enumerate() {
                if def.def_start + idx == own_name_idx {
                    continue;
                }
                if let Tok::Ident(callee) = &item.tok {
                    if names.contains(callee) {
                        *entry.entry(callee.clone()).or_insert(0) += 1;
                    }
                }
            }
        }
        CallGraph { edges }
    }

    pub(crate) fn reachable_from(&self, roots: &[String]) -> HashSet<String> {
        let mut reachable: HashSet<String> = HashSet::new();
        let mut queue: Vec<String> = roots.to_vec();
        while let Some(name) = queue.pop() {
            if !reachable.insert(name.clone()) {
                continue;
            }
            if let Some(callees) = self.edges.get(&name) {
                for callee in callees.keys() {
                    if !reachable.contains(callee) {
                        queue.push(callee.clone());
                    }
                }
            }
        }
        reachable
    }

    #[allow(dead_code)]
    pub(crate) fn total_calls_to(&self, name: &str) -> usize {
        self.edges.values().map(|callees| callees.get(name).copied().unwrap_or(0)).sum()
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

    #[test]
    fn counts_multiple_calls_to_the_same_function() {
        let items = items_from("void helper(){}void mainImage(){helper();helper();helper();}");
        let defs = find_function_definitions(&items);
        let names: HashSet<String> = defs.iter().map(|d| d.name.clone()).collect();
        let graph = CallGraph::build(&items, &defs, &names);
        assert_eq!(graph.total_calls_to("helper"), 3);
        assert_eq!(graph.total_calls_to("mainImage"), 0);
    }

    #[test]
    fn sums_calls_across_multiple_distinct_callers() {
        let items = items_from(
            "void helper(){}void a(){helper();}void b(){helper();helper();}void mainImage(){a();b();}",
        );
        let defs = find_function_definitions(&items);
        let names: HashSet<String> = defs.iter().map(|d| d.name.clone()).collect();
        let graph = CallGraph::build(&items, &defs, &names);
        assert_eq!(graph.total_calls_to("helper"), 3);
        assert_eq!(graph.total_calls_to("a"), 1);
        assert_eq!(graph.total_calls_to("b"), 1);
    }

    #[test]
    fn reachable_from_matches_transitive_call_chain() {
        let items = items_from("void deadFn(){}void a(){}void b(){a();}void mainImage(){b();}");
        let defs = find_function_definitions(&items);
        let names: HashSet<String> = defs.iter().map(|d| d.name.clone()).collect();
        let graph = CallGraph::build(&items, &defs, &names);
        let reachable = graph.reachable_from(&["mainImage".to_string()]);
        assert!(reachable.contains("mainImage"));
        assert!(reachable.contains("b"));
        assert!(reachable.contains("a"));
        assert!(!reachable.contains("deadFn"));
    }

    #[test]
    fn unreached_functions_have_zero_total_calls() {
        let items = items_from("void deadFn(){}void mainImage(){}");
        let defs = find_function_definitions(&items);
        let names: HashSet<String> = defs.iter().map(|d| d.name.clone()).collect();
        let graph = CallGraph::build(&items, &defs, &names);
        assert_eq!(graph.total_calls_to("deadFn"), 0);
    }
}
