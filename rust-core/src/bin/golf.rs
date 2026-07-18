use std::env;
use std::fs;
use std::io::{self, Read};
use std::process::ExitCode;
use std::thread;
use std::time::{Duration, SystemTime};
use ushader_core::{golf_with_protected_names, AggressiveOptions, GolfResult};

const HELP: &str = r#"golf — GLSL minification CLI

USAGE:
    golf [OPTIONS] [FILE]

    Reads GLSL source from FILE, or from stdin if FILE is omitted.
    Prints the golfed result to stdout; the stats summary goes to
    stderr (so `golf shader.glsl > out.glsl` captures only the code).

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
    --diff-only             Print only the stats summary (to stdout in
                             this mode), not the golfed code itself --
                             for scripting.
    --watch                 Re-run on every change to FILE (requires a
                             FILE argument -- not compatible with stdin).
                             Polls every 300ms; stop with Ctrl+C.
    --protect NAMES         Comma-separated identifiers to never rename
                             (e.g. custom uniforms a host app binds by
                             name). Applies in both safe and aggressive
                             mode.
    -h, --help              Print this message and exit.
"#;

fn print_stats(result: &GolfResult, aggressive: bool, to_stdout: bool) {
    let line = format!(
        "-- {} -> {} chars ({:.1}% reduction, {} identifiers renamed, {} numbers shortened{})",
        result.stats.input_chars,
        result.stats.output_chars,
        result.stats.reduction_pct,
        result.stats.renamed_count,
        result.stats.numbers_shortened,
        if aggressive {
            format!(
                ", {} dead locals removed, {} dead stores removed, {} constants folded, {} constant vectors reduced, {} trailing returns removed, {} compound assignments, {} increments/decrements, {} ternaries from if/else, {} declarations merged, {} brace blocks removed, {} redundant parens removed, {} duplicate precision qualifiers removed, {} dead functions removed, {} functions inlined",
                result.stats.aggressive.dead_locals_removed,
                result.stats.aggressive.dead_stores_removed,
                result.stats.aggressive.constants_folded,
                result.stats.aggressive.constant_vectors_reduced,
                result.stats.aggressive.trailing_void_returns_removed,
                result.stats.aggressive.compound_assignments,
                result.stats.aggressive.increments_decrements,
                result.stats.aggressive.ternaries_from_if_else,
                result.stats.aggressive.declarations_merged,
                result.stats.aggressive.braces_removed,
                result.stats.aggressive.redundant_parens_removed,
                result.stats.aggressive.duplicate_precision_removed,
                result.stats.aggressive.dead_functions_removed,
                result.stats.aggressive.functions_inlined,
            )
        } else {
            String::new()
        },
    );
    if to_stdout {
        println!("{line}");
    } else {
        eprintln!("{line}");
    }
}

fn run_golf(source: &str, aggressive: bool, options: AggressiveOptions, protected: &[String], diff_only: bool) {
    let effective_options = if aggressive { options } else { AggressiveOptions::none() };
    let result = golf_with_protected_names(source, effective_options, protected);
    print_stats(&result, aggressive, diff_only);
    if !diff_only {
        println!("{}", result.code);
    }
}

fn read_source(args: &[String]) -> Result<String, String> {
    if let Some(path) = args.first() {
        fs::read_to_string(path).map_err(|e| format!("failed to read {path}: {e}"))
    } else {
        let mut s = String::new();
        io::stdin()
            .read_to_string(&mut s)
            .map_err(|_| "no input provided (pass a file argument or pipe stdin)".to_string())?;
        Ok(s)
    }
}

fn run_watch(path: &str, aggressive: bool, options: AggressiveOptions, protected: &[String], diff_only: bool) -> ExitCode {
    let mut last_modified: Option<SystemTime> = None;
    eprintln!("watching {path} (Ctrl+C to stop)...");
    loop {
        match fs::metadata(path).and_then(|m| m.modified()) {
            Ok(modified) => {
                if last_modified != Some(modified) {
                    last_modified = Some(modified);
                    match fs::read_to_string(path) {
                        Ok(source) => run_golf(&source, aggressive, options, protected, diff_only),
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

fn main() -> ExitCode {
    let mut args: Vec<String> = env::args().skip(1).collect();

    if args.iter().any(|a| a == "-h" || a == "--help") {
        print!("{HELP}");
        return ExitCode::SUCCESS;
    }

    let mut aggressive = false;
    let mut options = AggressiveOptions::all();
    let mut protected: Vec<String> = Vec::new();
    let mut diff_only = false;
    let mut watch = false;

    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "-a" | "--aggressive" => {
                aggressive = true;
                args.remove(i);
            }
            "--no-dead-locals" => {
                options.eliminate_dead_locals = false;
                args.remove(i);
            }
            "--no-dead-stores" => {
                options.eliminate_dead_stores = false;
                args.remove(i);
            }
            "--no-fold-constants" => {
                options.fold_constants = false;
                args.remove(i);
            }
            "--no-reduce-vectors" => {
                options.reduce_constant_vectors = false;
                args.remove(i);
            }
            "--no-trailing-return" => {
                options.strip_trailing_void_return = false;
                args.remove(i);
            }
            "--no-compound" => {
                options.compound_assignments = false;
                args.remove(i);
            }
            "--no-inc-dec" => {
                options.increment_decrement = false;
                args.remove(i);
            }
            "--no-ternary" => {
                options.ternary_from_if_else = false;
                args.remove(i);
            }
            "--no-merge" => {
                options.merge_declarations = false;
                args.remove(i);
            }
            "--no-braces" => {
                options.strip_redundant_braces = false;
                args.remove(i);
            }
            "--no-parens" => {
                options.strip_redundant_parens = false;
                args.remove(i);
            }
            "--no-dup-precision" => {
                options.strip_duplicate_precision = false;
                args.remove(i);
            }
            "--no-dead-functions" => {
                options.eliminate_dead_functions = false;
                args.remove(i);
            }
            "--diff-only" => {
                diff_only = true;
                args.remove(i);
            }
            "--watch" => {
                watch = true;
                args.remove(i);
            }
            "--protect" => {
                args.remove(i);
                if i >= args.len() {
                    eprintln!("--protect requires a value (comma-separated list)");
                    return ExitCode::FAILURE;
                }
                let value = args.remove(i);
                protected.extend(value.split(',').map(|s| s.trim().to_string()).filter(|s| !s.is_empty()));
            }
            s if s.starts_with('-') => {
                eprintln!("unknown option: {s} (try --help)");
                return ExitCode::FAILURE;
            }
            _ => i += 1,
        }
    }

    if watch {
        let Some(path) = args.first().cloned() else {
            eprintln!("--watch requires a file argument (stdin is not supported)");
            return ExitCode::FAILURE;
        };
        return run_watch(&path, aggressive, options, &protected, diff_only);
    }

    let source = match read_source(&args) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("{e}");
            return ExitCode::FAILURE;
        }
    };
    run_golf(&source, aggressive, options, &protected, diff_only);
    ExitCode::SUCCESS
}
