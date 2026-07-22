use std::env;
use std::fs;
use std::io::{self, Read};
use std::path::{Path, PathBuf};
use std::process::ExitCode;
use std::thread;
use std::time::{Duration, SystemTime};
use ushader_core::{
    estimate_budget, golf_with_protected_names, presets, AggressiveOptions, AggressiveStats,
    BudgetPreset, BudgetResult, GolfResult,
};

const HELP: &str = r#"golf — GLSL minification CLI

USAGE:
    golf [OPTIONS] [PATH]...

    With a single file or stdin, prints the golfed result to stdout and
    the stats summary to stderr (so `golf shader.glsl > out.glsl`
    captures only the code).

    With a directory or a glob (containing * or ?), runs in batch mode:
    every matching .glsl file is golfed and written next to it as
    <name>.min.glsl (files already ending in .min.glsl are skipped when
    scanning a directory). Batch mode is what an asset pipeline embeds.

OPTIONS:
    -a, --aggressive        Enable all structural golfing passes. Off
                             by default -- the safe pipeline (rename +
                             shorten numbers + layout) always runs
                             regardless.
    --no-dead-locals        With -a, disable dead-local elimination.
    --no-dead-stores        With -a, disable dead-store elimination.
    --no-fold-constants     With -a, disable integer constant folding.
    --no-reduce-vectors     With -a, disable constant vector reduction.
    --no-trailing-return    With -a, disable trailing void-return removal.
    --no-compound           With -a, disable compound-assignment rewriting.
    --no-inc-dec            With -a, disable +=1/-=1 -> ++/-- rewriting.
    --no-ternary            With -a, disable if/else -> ?: rewriting.
    --no-merge              With -a, disable declaration merging.
    --no-braces             With -a, disable redundant-brace stripping.
    --no-parens             With -a, disable redundant-parenthesis stripping.
    --no-dup-precision      With -a, disable duplicate-precision stripping.
    --no-dead-functions     With -a, disable dead-function elimination.
    --no-inline             With -a, disable single-call function inlining.
    --no-algebraic          With -a, disable algebraic-identity simplification.
    --no-cse                With -a, disable common-subexpression elimination.
    --profile PATH          Load a .ushaderprofile file (the exact
                             pass/protected-names/budget configuration
                             saved from the GUI) as the base config.
                             Explicit flags below still override it.
    --budget PRESET         Fail (non-zero exit) if any file's estimate
                             exceeds the named preset threshold. Valid
                             presets: Shadertoy, "X/Twitter shader",
                             "JS13K-style 13KB", "4KB intro", "8KB intro",
                             "64KB intro".
    --report PATH           Write a per-file report to PATH. The
                             extension selects the format: .json or .csv.
    --diff                  Print a unified source/golfed diff per file
                             to stdout instead of writing golfed output.
    --diff-only             Print only the stats summary (to stdout in
                             this mode), not the golfed code itself --
                             for scripting.
    --pretty                Opt into colored, human-oriented console
                             output. Off by default so CI logs stay clean.
    --watch                 Re-run on every change to a single FILE.
                             Polls every 300ms; stop with Ctrl+C.
    --protect NAMES         Comma-separated identifiers to never rename
                             (added on top of any --profile list).
    -h, --help              Print this message and exit.
"#;

const RESET: &str = "\x1b[0m";
const RED: &str = "\x1b[31m";
const GREEN: &str = "\x1b[32m";
const CYAN: &str = "\x1b[36m";
const BOLD: &str = "\x1b[1m";

fn paint(text: &str, code: &str, pretty: bool) -> String {
    if pretty {
        format!("{code}{text}{RESET}")
    } else {
        text.to_string()
    }
}

fn has_glob(s: &str) -> bool {
    s.contains('*') || s.contains('?')
}

fn json_escape(value: &str) -> String {
    let mut out = String::with_capacity(value.len());
    for c in value.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            _ => out.push(c),
        }
    }
    out
}

fn csv_field(value: &str) -> String {
    if value.contains(',') || value.contains('"') || value.contains('\n') || value.contains('\r') {
        format!("\"{}\"", value.replace('"', "\"\""))
    } else {
        value.to_string()
    }
}

struct Profile {
    aggressive: bool,
    options: AggressiveOptions,
    protected: Vec<String>,
    budget: Option<String>,
}

fn profile_bool(text: &str, key: &str, default: bool) -> bool {
    let needle = format!("\"{key}\"");
    let Some(key_pos) = text.find(&needle) else {
        return default;
    };
    let after = &text[key_pos + needle.len()..];
    let Some(colon) = after.find(':') else {
        return default;
    };
    let slice = &after[colon + 1..];
    let end = slice.find([',', '}', '\n']).unwrap_or(slice.len());
    let value = &slice[..end];
    if value.contains("true") {
        true
    } else if value.contains("false") {
        false
    } else {
        default
    }
}

fn profile_string(text: &str, key: &str) -> String {
    let needle = format!("\"{key}\"");
    let Some(key_pos) = text.find(&needle) else {
        return String::new();
    };
    let after = &text[key_pos + needle.len()..];
    let Some(colon) = after.find(':') else {
        return String::new();
    };
    let rest = &after[colon + 1..];
    let Some(open) = rest.find('"') else {
        return String::new();
    };
    let body = &rest[open + 1..];
    let mut out = String::new();
    let mut chars = body.chars();
    while let Some(c) = chars.next() {
        match c {
            '"' => break,
            '\\' => {
                if let Some(next) = chars.next() {
                    match next {
                        'n' => out.push('\n'),
                        'r' => out.push('\r'),
                        't' => out.push('\t'),
                        other => out.push(other),
                    }
                }
            }
            other => out.push(other),
        }
    }
    out
}

fn parse_protected(value: &str) -> Vec<String> {
    value
        .split(',')
        .map(|s| s.trim().to_string())
        .filter(|s| !s.is_empty())
        .collect()
}

fn parse_profile(text: &str) -> Result<Profile, String> {
    if !text.contains('{') {
        return Err("profile is not a valid .ushaderprofile document".to_string());
    }
    let options = AggressiveOptions {
        eliminate_dead_locals: profile_bool(text, "eliminate_dead_locals", true),
        eliminate_dead_stores: profile_bool(text, "eliminate_dead_stores", true),
        fold_constants: profile_bool(text, "fold_constants", true),
        reduce_constant_vectors: profile_bool(text, "reduce_constant_vectors", true),
        strip_trailing_void_return: profile_bool(text, "strip_trailing_void_return", true),
        compound_assignments: profile_bool(text, "compound_assignments", true),
        increment_decrement: profile_bool(text, "increment_decrement", true),
        ternary_from_if_else: profile_bool(text, "ternary_from_if_else", true),
        merge_declarations: profile_bool(text, "merge_declarations", true),
        strip_redundant_braces: profile_bool(text, "strip_redundant_braces", true),
        strip_redundant_parens: profile_bool(text, "strip_redundant_parens", true),
        strip_duplicate_precision: profile_bool(text, "strip_duplicate_precision", true),
        eliminate_dead_functions: profile_bool(text, "eliminate_dead_functions", true),
        inline_single_call_functions: profile_bool(text, "inline_single_call_functions", true),
        simplify_algebraic_identities: profile_bool(text, "simplify_algebraic_identities", true),
        eliminate_common_subexpressions: profile_bool(text, "eliminate_common_subexpressions", true),
    };
    let budget_name = profile_string(text, "budget_preset");
    Ok(Profile {
        aggressive: profile_bool(text, "aggressive", true),
        options,
        protected: parse_protected(&profile_string(text, "protected_names")),
        budget: if budget_name.is_empty() {
            None
        } else {
            Some(budget_name)
        },
    })
}

fn find_preset(name: &str) -> Option<&'static BudgetPreset> {
    presets().iter().find(|p| p.name == name)
}

fn budget_pass(preset: &BudgetPreset, result: &BudgetResult) -> bool {
    if let Some(limit) = preset.raw_limit {
        if result.raw_bytes > limit {
            return false;
        }
    }
    if let Some(limit) = preset.deflate_limit {
        if result.deflate_bytes > limit {
            return false;
        }
    }
    true
}

fn wildcard_match(pattern: &str, text: &str) -> bool {
    let p: Vec<char> = pattern.chars().collect();
    let t: Vec<char> = text.chars().collect();
    let (mut pi, mut ti) = (0usize, 0usize);
    let (mut star, mut mark) = (usize::MAX, 0usize);
    while ti < t.len() {
        if pi < p.len() && (p[pi] == '?' || p[pi] == t[ti]) {
            pi += 1;
            ti += 1;
        } else if pi < p.len() && p[pi] == '*' {
            star = pi;
            mark = ti;
            pi += 1;
        } else if star != usize::MAX {
            pi = star + 1;
            mark += 1;
            ti = mark;
        } else {
            return false;
        }
    }
    while pi < p.len() && p[pi] == '*' {
        pi += 1;
    }
    pi == p.len()
}

fn walk_glsl(dir: &Path, out: &mut Vec<PathBuf>) {
    let Ok(entries) = fs::read_dir(dir) else {
        return;
    };
    let mut paths: Vec<PathBuf> = entries.filter_map(|e| e.ok().map(|e| e.path())).collect();
    paths.sort();
    for path in paths {
        if path.is_dir() {
            walk_glsl(&path, out);
        } else if is_golf_input(&path) {
            out.push(path);
        }
    }
}

fn is_golf_input(path: &Path) -> bool {
    let Some(name) = path.file_name().and_then(|n| n.to_str()) else {
        return false;
    };
    let lower = name.to_ascii_lowercase();
    lower.ends_with(".glsl") && !lower.ends_with(".min.glsl")
}

fn glob_walk(dir: &Path, segs: &[String], out: &mut Vec<PathBuf>) {
    if segs.is_empty() {
        return;
    }
    if segs[0] == "**" {
        glob_walk(dir, &segs[1..], out);
        if let Ok(entries) = fs::read_dir(dir) {
            let mut dirs: Vec<PathBuf> = entries
                .filter_map(|e| e.ok().map(|e| e.path()))
                .filter(|p| p.is_dir())
                .collect();
            dirs.sort();
            for sub in dirs {
                glob_walk(&sub, segs, out);
            }
        }
        return;
    }
    let Ok(entries) = fs::read_dir(dir) else {
        return;
    };
    let mut paths: Vec<PathBuf> = entries.filter_map(|e| e.ok().map(|e| e.path())).collect();
    paths.sort();
    let last = segs.len() == 1;
    for path in paths {
        let Some(name) = path.file_name().and_then(|n| n.to_str()) else {
            continue;
        };
        if !wildcard_match(&segs[0], name) {
            continue;
        }
        if last {
            if path.is_file() {
                out.push(path);
            }
        } else if path.is_dir() {
            glob_walk(&path, &segs[1..], out);
        }
    }
}

fn expand_glob(pattern: &str) -> Vec<PathBuf> {
    let normalized = pattern.replace('\\', "/");
    let absolute = normalized.starts_with('/');
    let parts: Vec<&str> = normalized.split('/').filter(|s| !s.is_empty()).collect();
    let mut root = if absolute {
        PathBuf::from("/")
    } else {
        PathBuf::from(".")
    };
    let mut idx = 0;
    while idx + 1 < parts.len() && !has_glob(parts[idx]) {
        root.push(parts[idx]);
        idx += 1;
    }
    let segs: Vec<String> = parts[idx..].iter().map(|s| s.to_string()).collect();
    let mut out = Vec::new();
    glob_walk(&root, &segs, &mut out);
    out.sort();
    out.dedup();
    out
}

fn output_path(input: &Path) -> PathBuf {
    let name = input
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or("shader.glsl");
    let stem = if let Some(rest) = name.strip_suffix(".glsl") {
        rest
    } else {
        name
    };
    let mut out = input.to_path_buf();
    out.set_file_name(format!("{stem}.min.glsl"));
    out
}

fn diff_ops<'a>(a: &[&'a str], b: &[&'a str]) -> Vec<(u8, &'a str)> {
    let n = a.len();
    let m = b.len();
    let mut dp = vec![vec![0u32; m + 1]; n + 1];
    for i in (0..n).rev() {
        for j in (0..m).rev() {
            dp[i][j] = if a[i] == b[j] {
                dp[i + 1][j + 1] + 1
            } else {
                dp[i + 1][j].max(dp[i][j + 1])
            };
        }
    }
    let mut ops = Vec::new();
    let (mut i, mut j) = (0usize, 0usize);
    while i < n && j < m {
        if a[i] == b[j] {
            ops.push((b' ', a[i]));
            i += 1;
            j += 1;
        } else if dp[i + 1][j] >= dp[i][j + 1] {
            ops.push((b'-', a[i]));
            i += 1;
        } else {
            ops.push((b'+', b[j]));
            j += 1;
        }
    }
    while i < n {
        ops.push((b'-', a[i]));
        i += 1;
    }
    while j < m {
        ops.push((b'+', b[j]));
        j += 1;
    }
    ops
}

struct Line {
    tag: u8,
    text: String,
    a_ln: usize,
    b_ln: usize,
}

fn unified_diff(a: &str, b: &str, a_name: &str, b_name: &str, pretty: bool) -> String {
    let al: Vec<&str> = a.lines().collect();
    let bl: Vec<&str> = b.lines().collect();
    let ops = diff_ops(&al, &bl);
    let mut lines = Vec::with_capacity(ops.len());
    let (mut ai, mut bi) = (0usize, 0usize);
    for (tag, text) in ops {
        let line = Line {
            tag,
            text: text.to_string(),
            a_ln: ai,
            b_ln: bi,
        };
        match tag {
            b' ' => {
                ai += 1;
                bi += 1;
            }
            b'-' => ai += 1,
            _ => bi += 1,
        }
        lines.push(line);
    }

    let context = 3usize;
    let mut interesting = vec![false; lines.len()];
    for (idx, line) in lines.iter().enumerate() {
        if line.tag != b' ' {
            let lo = idx.saturating_sub(context);
            let hi = (idx + context).min(lines.len().saturating_sub(1));
            for flag in interesting.iter_mut().take(hi + 1).skip(lo) {
                *flag = true;
            }
        }
    }

    let mut out = String::new();
    let mut header_written = false;
    let mut idx = 0;
    while idx < lines.len() {
        if !interesting[idx] {
            idx += 1;
            continue;
        }
        let start = idx;
        while idx < lines.len() && interesting[idx] {
            idx += 1;
        }
        let hunk = &lines[start..idx];
        if !header_written {
            out.push_str(&paint(&format!("--- {a_name}\n"), BOLD, pretty));
            out.push_str(&paint(&format!("+++ {b_name}\n"), BOLD, pretty));
            header_written = true;
        }
        let a_count = hunk.iter().filter(|l| l.tag != b'+').count();
        let b_count = hunk.iter().filter(|l| l.tag != b'-').count();
        let a_start = hunk.first().map(|l| l.a_ln + 1).unwrap_or(0);
        let b_start = hunk.first().map(|l| l.b_ln + 1).unwrap_or(0);
        out.push_str(&paint(
            &format!("@@ -{a_start},{a_count} +{b_start},{b_count} @@\n"),
            CYAN,
            pretty,
        ));
        for line in hunk {
            let rendered = format!("{}{}\n", line.tag as char, line.text);
            let colored = match line.tag {
                b'+' => paint(&rendered, GREEN, pretty),
                b'-' => paint(&rendered, RED, pretty),
                _ => rendered,
            };
            out.push_str(&colored);
        }
    }
    if !header_written {
        return String::new();
    }
    out
}

fn stats_summary(result: &GolfResult, aggressive: bool) -> String {
    let mut line = format!(
        "{} -> {} chars ({:.1}% reduction, {} identifiers renamed, {} numbers shortened",
        result.stats.input_chars,
        result.stats.output_chars,
        result.stats.reduction_pct,
        result.stats.renamed_count,
        result.stats.numbers_shortened,
    );
    if aggressive {
        let a = &result.stats.aggressive;
        line.push_str(&format!(
            ", {} dead locals removed, {} dead stores removed, {} constants folded, {} constant vectors reduced, {} trailing returns removed, {} compound assignments, {} increments/decrements, {} ternaries from if/else, {} declarations merged, {} brace blocks removed, {} redundant parens removed, {} duplicate precision qualifiers removed, {} dead functions removed, {} functions inlined, {} algebraic identities simplified, {} common subexpressions eliminated",
            a.dead_locals_removed,
            a.dead_stores_removed,
            a.constants_folded,
            a.constant_vectors_reduced,
            a.trailing_void_returns_removed,
            a.compound_assignments,
            a.increments_decrements,
            a.ternaries_from_if_else,
            a.declarations_merged,
            a.braces_removed,
            a.redundant_parens_removed,
            a.duplicate_precision_removed,
            a.dead_functions_removed,
            a.functions_inlined,
            a.algebraic_identities_simplified,
            a.common_subexpressions_eliminated,
        ));
    }
    line.push(')');
    line
}

struct FileReport {
    path: String,
    raw_bytes: usize,
    golfed_bytes: usize,
    compressed_bytes: usize,
    reduction_pct: f64,
    renamed_count: usize,
    numbers_shortened: usize,
    aggressive: AggressiveStats,
    budget_pass: Option<bool>,
}

const AGGRESSIVE_COLUMNS: [&str; 16] = [
    "compound_assignments",
    "declarations_merged",
    "braces_removed",
    "constants_folded",
    "dead_locals_removed",
    "dead_stores_removed",
    "constant_vectors_reduced",
    "trailing_void_returns_removed",
    "increments_decrements",
    "ternaries_from_if_else",
    "redundant_parens_removed",
    "duplicate_precision_removed",
    "dead_functions_removed",
    "functions_inlined",
    "algebraic_identities_simplified",
    "common_subexpressions_eliminated",
];

fn aggressive_values(a: &AggressiveStats) -> [usize; 16] {
    [
        a.compound_assignments,
        a.declarations_merged,
        a.braces_removed,
        a.constants_folded,
        a.dead_locals_removed,
        a.dead_stores_removed,
        a.constant_vectors_reduced,
        a.trailing_void_returns_removed,
        a.increments_decrements,
        a.ternaries_from_if_else,
        a.redundant_parens_removed,
        a.duplicate_precision_removed,
        a.dead_functions_removed,
        a.functions_inlined,
        a.algebraic_identities_simplified,
        a.common_subexpressions_eliminated,
    ]
}

fn render_report_json(reports: &[FileReport], budget: Option<&str>) -> String {
    let mut out = String::from("{\n");
    match budget {
        Some(name) => out.push_str(&format!("  \"budget_preset\": \"{}\",\n", json_escape(name))),
        None => out.push_str("  \"budget_preset\": null,\n"),
    }
    out.push_str("  \"files\": [\n");
    for (i, report) in reports.iter().enumerate() {
        out.push_str("    {\n");
        out.push_str(&format!("      \"path\": \"{}\",\n", json_escape(&report.path)));
        out.push_str(&format!("      \"raw_bytes\": {},\n", report.raw_bytes));
        out.push_str(&format!("      \"golfed_bytes\": {},\n", report.golfed_bytes));
        out.push_str(&format!(
            "      \"compressed_bytes\": {},\n",
            report.compressed_bytes
        ));
        out.push_str(&format!(
            "      \"reduction_pct\": {:.1},\n",
            report.reduction_pct
        ));
        out.push_str(&format!(
            "      \"renamed_count\": {},\n",
            report.renamed_count
        ));
        out.push_str(&format!(
            "      \"numbers_shortened\": {},\n",
            report.numbers_shortened
        ));
        out.push_str("      \"passes\": {\n");
        let values = aggressive_values(&report.aggressive);
        for (k, (name, value)) in AGGRESSIVE_COLUMNS.iter().zip(values.iter()).enumerate() {
            let comma = if k + 1 < AGGRESSIVE_COLUMNS.len() {
                ","
            } else {
                ""
            };
            out.push_str(&format!("        \"{name}\": {value}{comma}\n"));
        }
        out.push_str("      },\n");
        match report.budget_pass {
            Some(true) => out.push_str("      \"budget_pass\": true\n"),
            Some(false) => out.push_str("      \"budget_pass\": false\n"),
            None => out.push_str("      \"budget_pass\": null\n"),
        }
        let comma = if i + 1 < reports.len() { "," } else { "" };
        out.push_str(&format!("    }}{comma}\n"));
    }
    out.push_str("  ]\n}\n");
    out
}

fn render_report_csv(reports: &[FileReport]) -> String {
    let mut out = String::new();
    out.push_str("path,raw_bytes,golfed_bytes,compressed_bytes,reduction_pct,renamed_count,numbers_shortened,");
    out.push_str(&AGGRESSIVE_COLUMNS.join(","));
    out.push_str(",budget_pass\n");
    for report in reports {
        out.push_str(&csv_field(&report.path));
        out.push_str(&format!(
            ",{},{},{},{:.1},{},{}",
            report.raw_bytes,
            report.golfed_bytes,
            report.compressed_bytes,
            report.reduction_pct,
            report.renamed_count,
            report.numbers_shortened,
        ));
        for value in aggressive_values(&report.aggressive) {
            out.push_str(&format!(",{value}"));
        }
        let budget = match report.budget_pass {
            Some(true) => "pass",
            Some(false) => "fail",
            None => "",
        };
        out.push_str(&format!(",{budget}\n"));
    }
    out
}

struct Config {
    aggressive: bool,
    options: AggressiveOptions,
    protected: Vec<String>,
    budget: Option<String>,
    report: Option<String>,
    diff: bool,
    diff_only: bool,
    pretty: bool,
    watch: bool,
    inputs: Vec<String>,
}

fn parse_args(args: Vec<String>) -> Result<Option<Config>, String> {
    if args.iter().any(|a| a == "-h" || a == "--help") {
        print!("{HELP}");
        return Ok(None);
    }

    let mut aggressive_flag = false;
    let mut disabled: Vec<&str> = Vec::new();
    let mut protected: Vec<String> = Vec::new();
    let mut profile_path: Option<String> = None;
    let mut budget: Option<String> = None;
    let mut report: Option<String> = None;
    let mut diff = false;
    let mut diff_only = false;
    let mut pretty = false;
    let mut watch = false;
    let mut inputs: Vec<String> = Vec::new();

    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "-a" | "--aggressive" => aggressive_flag = true,
            "--no-dead-locals" => disabled.push("eliminate_dead_locals"),
            "--no-dead-stores" => disabled.push("eliminate_dead_stores"),
            "--no-fold-constants" => disabled.push("fold_constants"),
            "--no-reduce-vectors" => disabled.push("reduce_constant_vectors"),
            "--no-trailing-return" => disabled.push("strip_trailing_void_return"),
            "--no-compound" => disabled.push("compound_assignments"),
            "--no-inc-dec" => disabled.push("increment_decrement"),
            "--no-ternary" => disabled.push("ternary_from_if_else"),
            "--no-merge" => disabled.push("merge_declarations"),
            "--no-braces" => disabled.push("strip_redundant_braces"),
            "--no-parens" => disabled.push("strip_redundant_parens"),
            "--no-dup-precision" => disabled.push("strip_duplicate_precision"),
            "--no-dead-functions" => disabled.push("eliminate_dead_functions"),
            "--no-inline" => disabled.push("inline_single_call_functions"),
            "--no-algebraic" => disabled.push("simplify_algebraic_identities"),
            "--no-cse" => disabled.push("eliminate_common_subexpressions"),
            "--diff" => diff = true,
            "--diff-only" => diff_only = true,
            "--pretty" => pretty = true,
            "--watch" => watch = true,
            "--profile" => {
                i += 1;
                profile_path = Some(
                    args.get(i)
                        .cloned()
                        .ok_or_else(|| "--profile requires a path".to_string())?,
                );
            }
            "--budget" => {
                i += 1;
                budget = Some(
                    args.get(i)
                        .cloned()
                        .ok_or_else(|| "--budget requires a preset name".to_string())?,
                );
            }
            "--report" => {
                i += 1;
                report = Some(
                    args.get(i)
                        .cloned()
                        .ok_or_else(|| "--report requires a path".to_string())?,
                );
            }
            "--protect" => {
                i += 1;
                let value = args
                    .get(i)
                    .cloned()
                    .ok_or_else(|| "--protect requires a value (comma-separated list)".to_string())?;
                protected.extend(parse_protected(&value));
            }
            s if s.starts_with('-') && s != "-" => {
                return Err(format!("unknown option: {s} (try --help)"));
            }
            _ => inputs.push(args[i].clone()),
        }
        i += 1;
    }

    let mut aggressive = false;
    let mut options = AggressiveOptions::all();
    if let Some(path) = &profile_path {
        let text = fs::read_to_string(path).map_err(|e| format!("failed to read {path}: {e}"))?;
        let profile = parse_profile(&text)?;
        aggressive = profile.aggressive;
        options = profile.options;
        let mut merged = profile.protected;
        merged.extend(protected);
        protected = merged;
        if budget.is_none() {
            budget = profile.budget;
        }
    }
    if aggressive_flag {
        aggressive = true;
    }
    for name in disabled {
        set_option(&mut options, name, false);
    }

    if let Some(name) = &budget {
        if find_preset(name).is_none() {
            return Err(format!(
                "unknown budget preset: {name} (see --help for valid names)"
            ));
        }
    }
    if let Some(path) = &report {
        let lower = path.to_ascii_lowercase();
        if !lower.ends_with(".json") && !lower.ends_with(".csv") {
            return Err("--report path must end in .json or .csv".to_string());
        }
    }

    Ok(Some(Config {
        aggressive,
        options,
        protected,
        budget,
        report,
        diff,
        diff_only,
        pretty,
        watch,
        inputs,
    }))
}

fn set_option(options: &mut AggressiveOptions, name: &str, value: bool) {
    match name {
        "eliminate_dead_locals" => options.eliminate_dead_locals = value,
        "eliminate_dead_stores" => options.eliminate_dead_stores = value,
        "fold_constants" => options.fold_constants = value,
        "reduce_constant_vectors" => options.reduce_constant_vectors = value,
        "strip_trailing_void_return" => options.strip_trailing_void_return = value,
        "compound_assignments" => options.compound_assignments = value,
        "increment_decrement" => options.increment_decrement = value,
        "ternary_from_if_else" => options.ternary_from_if_else = value,
        "merge_declarations" => options.merge_declarations = value,
        "strip_redundant_braces" => options.strip_redundant_braces = value,
        "strip_redundant_parens" => options.strip_redundant_parens = value,
        "strip_duplicate_precision" => options.strip_duplicate_precision = value,
        "eliminate_dead_functions" => options.eliminate_dead_functions = value,
        "inline_single_call_functions" => options.inline_single_call_functions = value,
        "simplify_algebraic_identities" => options.simplify_algebraic_identities = value,
        "eliminate_common_subexpressions" => options.eliminate_common_subexpressions = value,
        _ => {}
    }
}

fn golf_source(source: &str, config: &Config) -> GolfResult {
    let options = if config.aggressive {
        config.options
    } else {
        AggressiveOptions::none()
    };
    golf_with_protected_names(source, options, &config.protected)
}

fn make_report(path: &str, source: &str, result: &GolfResult, config: &Config) -> FileReport {
    let budget_result = estimate_budget(&result.code);
    let budget_pass = config
        .budget
        .as_deref()
        .and_then(find_preset)
        .map(|preset| budget_pass(preset, &budget_result));
    FileReport {
        path: path.to_string(),
        raw_bytes: source.len(),
        golfed_bytes: result.code.len(),
        compressed_bytes: budget_result.deflate_bytes,
        reduction_pct: result.stats.reduction_pct,
        renamed_count: result.stats.renamed_count,
        numbers_shortened: result.stats.numbers_shortened,
        aggressive: result.stats.aggressive,
        budget_pass,
    }
}

fn collect_inputs(inputs: &[String]) -> Result<(Vec<PathBuf>, bool), String> {
    let mut files = Vec::new();
    let mut batch = false;
    for arg in inputs {
        if has_glob(arg) {
            batch = true;
            files.extend(expand_glob(arg).into_iter().filter(|p| is_golf_input(p)));
        } else {
            let path = Path::new(arg);
            if path.is_dir() {
                batch = true;
                walk_glsl(path, &mut files);
            } else {
                files.push(path.to_path_buf());
            }
        }
    }
    if files.len() > 1 {
        batch = true;
    }
    Ok((files, batch))
}

fn run_watch(path: &str, config: &Config) -> ExitCode {
    let mut last_modified: Option<SystemTime> = None;
    eprintln!("watching {path} (Ctrl+C to stop)...");
    loop {
        match fs::metadata(path).and_then(|m| m.modified()) {
            Ok(modified) => {
                if last_modified != Some(modified) {
                    last_modified = Some(modified);
                    match fs::read_to_string(path) {
                        Ok(source) => {
                            let result = golf_source(&source, config);
                            eprintln!("-- {}", stats_summary(&result, config.aggressive));
                            if !config.diff_only {
                                println!("{}", result.code);
                            }
                        }
                        Err(e) => eprintln!("failed to read {path}: {e}"),
                    }
                }
            }
            Err(e) => {
                eprintln!("failed to read {path}: {e}");
                return ExitCode::FAILURE;
            }
        }
        thread::sleep(Duration::from_millis(300));
    }
}

fn run(config: Config) -> ExitCode {
    if config.watch {
        let Some(path) = config.inputs.first().cloned() else {
            eprintln!("--watch requires a file argument (stdin is not supported)");
            return ExitCode::FAILURE;
        };
        return run_watch(&path, &config);
    }

    let mut reports: Vec<FileReport> = Vec::new();
    let mut had_failure = false;
    let mut budget_exceeded = false;

    if config.inputs.is_empty() {
        let mut source = String::new();
        if io::stdin().read_to_string(&mut source).is_err() {
            eprintln!("no input provided (pass a file/dir/glob or pipe stdin)");
            return ExitCode::FAILURE;
        }
        let result = golf_source(&source, &config);
        let report = make_report("<stdin>", &source, &result, &config);
        if report.budget_pass == Some(false) {
            budget_exceeded = true;
        }
        emit_single(&source, &result, &report, &config);
        reports.push(report);
    } else {
        let (files, batch) = match collect_inputs(&config.inputs) {
            Ok(v) => v,
            Err(e) => {
                eprintln!("{e}");
                return ExitCode::FAILURE;
            }
        };
        if files.is_empty() {
            eprintln!("no .glsl files matched the given path(s)");
            return ExitCode::FAILURE;
        }
        for file in files {
            let display = file.display().to_string();
            let source = match fs::read_to_string(&file) {
                Ok(s) => s,
                Err(e) => {
                    eprintln!("failed to read {display}: {e}");
                    had_failure = true;
                    continue;
                }
            };
            let result = golf_source(&source, &config);
            let report = make_report(&display, &source, &result, &config);
            if report.budget_pass == Some(false) {
                budget_exceeded = true;
            }
            if batch {
                if !emit_batch(&file, &display, &source, &result, &report, &config) {
                    had_failure = true;
                }
            } else {
                emit_single(&source, &result, &report, &config);
            }
            reports.push(report);
        }
    }

    if let Some(path) = &config.report {
        let content = if path.to_ascii_lowercase().ends_with(".csv") {
            render_report_csv(&reports)
        } else {
            render_report_json(&reports, config.budget.as_deref())
        };
        if let Err(e) = fs::write(path, content) {
            eprintln!("failed to write report {path}: {e}");
            had_failure = true;
        } else {
            eprintln!("wrote report {path}");
        }
    }

    if had_failure || budget_exceeded {
        ExitCode::FAILURE
    } else {
        ExitCode::SUCCESS
    }
}

fn emit_single(source: &str, result: &GolfResult, report: &FileReport, config: &Config) {
    if config.diff {
        print!(
            "{}",
            unified_diff(source, &result.code, "a", "b", config.pretty)
        );
    } else if config.diff_only {
        println!("-- {}", stats_summary(result, config.aggressive));
    } else {
        eprintln!("-- {}", stats_summary(result, config.aggressive));
        println!("{}", result.code);
    }
    report_budget_line(report, config);
}

fn emit_batch(
    input: &Path,
    display: &str,
    source: &str,
    result: &GolfResult,
    report: &FileReport,
    config: &Config,
) -> bool {
    if config.diff {
        let a = format!("a/{display}");
        let b = format!("b/{display}");
        print!("{}", unified_diff(source, &result.code, &a, &b, config.pretty));
        eprintln!("-- {display}: {}", stats_summary(result, config.aggressive));
        report_budget_line(report, config);
        return true;
    }
    let out = output_path(input);
    if let Err(e) = fs::write(&out, &result.code) {
        eprintln!("failed to write {}: {e}", out.display());
        return false;
    }
    eprintln!(
        "-- {display} -> {}: {}",
        out.display(),
        stats_summary(result, config.aggressive)
    );
    report_budget_line(report, config);
    true
}

fn report_budget_line(report: &FileReport, config: &Config) {
    let Some(name) = &config.budget else {
        return;
    };
    match report.budget_pass {
        Some(true) => eprintln!(
            "   budget [{name}]: {}",
            paint("PASS", GREEN, config.pretty)
        ),
        Some(false) => eprintln!(
            "   budget [{name}]: {} (raw {} B, deflate {} B)",
            paint("FAIL", RED, config.pretty),
            report.golfed_bytes,
            report.compressed_bytes
        ),
        None => {}
    }
}

fn main() -> ExitCode {
    let args: Vec<String> = env::args().skip(1).collect();
    match parse_args(args) {
        Ok(Some(config)) => run(config),
        Ok(None) => ExitCode::SUCCESS,
        Err(e) => {
            eprintln!("{e}");
            ExitCode::FAILURE
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn wildcard_matches_star_and_question() {
        assert!(wildcard_match("*.glsl", "shader.glsl"));
        assert!(wildcard_match("a?c.glsl", "abc.glsl"));
        assert!(!wildcard_match("a?c.glsl", "ac.glsl"));
        assert!(wildcard_match("*", "anything"));
        assert!(!wildcard_match("*.glsl", "shader.frag"));
        assert!(wildcard_match("shader.glsl", "shader.glsl"));
    }

    #[test]
    fn profile_round_trips_the_gui_format() {
        let text = r#"{
  "aggressive": true,
  "eliminate_dead_locals": true,
  "eliminate_dead_stores": false,
  "fold_constants": true,
  "reduce_constant_vectors": true,
  "strip_trailing_void_return": true,
  "compound_assignments": true,
  "increment_decrement": true,
  "ternary_from_if_else": true,
  "merge_declarations": true,
  "strip_redundant_braces": true,
  "strip_redundant_parens": true,
  "strip_duplicate_precision": true,
  "eliminate_dead_functions": true,
  "inline_single_call_functions": true,
  "simplify_algebraic_identities": true,
  "eliminate_common_subexpressions": true,
  "protected_names": "iChannel0, u",
  "budget_preset": "4KB intro"
}
"#;
        let profile = parse_profile(text).unwrap();
        assert!(profile.aggressive);
        assert!(profile.options.eliminate_dead_locals);
        assert!(!profile.options.eliminate_dead_stores);
        assert_eq!(profile.protected, vec!["iChannel0".to_string(), "u".to_string()]);
        assert_eq!(profile.budget.as_deref(), Some("4KB intro"));
    }

    #[test]
    fn budget_pass_respects_raw_and_deflate_limits() {
        let preset = find_preset("X/Twitter shader").unwrap();
        let under = BudgetResult {
            raw_bytes: 100,
            deflate_bytes: 90,
        };
        let over = BudgetResult {
            raw_bytes: 500,
            deflate_bytes: 300,
        };
        assert!(budget_pass(preset, &under));
        assert!(!budget_pass(preset, &over));
    }

    #[test]
    fn unified_diff_marks_added_and_removed_lines() {
        let a = "one\ntwo\nthree\n";
        let b = "one\n2\nthree\n";
        let diff = unified_diff(a, b, "a", "b", false);
        assert!(diff.contains("-two"));
        assert!(diff.contains("+2"));
        assert!(diff.contains(" one"));
        assert!(diff.contains("@@"));
    }

    #[test]
    fn identical_inputs_produce_no_diff() {
        let same = "void mainImage(){}\n";
        assert!(unified_diff(same, same, "a", "b", false).is_empty());
    }

    #[test]
    fn csv_report_has_header_and_row_per_file() {
        let reports = vec![FileReport {
            path: "a,b.glsl".to_string(),
            raw_bytes: 200,
            golfed_bytes: 120,
            compressed_bytes: 90,
            reduction_pct: 40.0,
            renamed_count: 3,
            numbers_shortened: 1,
            aggressive: AggressiveStats::default(),
            budget_pass: Some(false),
        }];
        let csv = render_report_csv(&reports);
        assert!(csv.starts_with("path,raw_bytes,golfed_bytes,compressed_bytes"));
        assert!(csv.contains("\"a,b.glsl\""));
        assert!(csv.trim_end().ends_with("fail"));
    }

    #[test]
    fn json_report_is_wellformed_and_lists_budget() {
        let reports = vec![FileReport {
            path: "c:\\shaders\\one.glsl".to_string(),
            raw_bytes: 200,
            golfed_bytes: 120,
            compressed_bytes: 90,
            reduction_pct: 40.0,
            renamed_count: 3,
            numbers_shortened: 1,
            aggressive: AggressiveStats::default(),
            budget_pass: Some(true),
        }];
        let json = render_report_json(&reports, Some("4KB intro"));
        assert!(json.contains("\"budget_preset\": \"4KB intro\""));
        assert!(json.contains("\"path\": \"c:\\\\shaders\\\\one.glsl\""));
        assert!(json.contains("\"budget_pass\": true"));
        assert!(json.contains("\"common_subexpressions_eliminated\": 0"));
    }
}
