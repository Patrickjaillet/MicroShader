use crate::budget::{estimate_budget, estimate_twigl_geekest_budget};
use crate::golfer::{
    golf_with_protected_names, golf_with_protected_names_traced, AggressiveOptions, GolferTrace,
    GolfStats,
};
use crate::twigl::{
    rewrite_twigl_shader, rewrite_twigl_shader_mrt, twigl_export_uniform_names, twigl_snippet,
    twigl_snippets, TwiglMode,
};
use std::ffi::{CStr, CString};
use std::os::raw::c_char;

#[repr(C)]
pub struct UshaderGolfOptions {
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
    /// golf.md Phase 30.1 -- see `AggressiveOptions::aggressive_inlining`.
    pub aggressive_inlining: bool,
    /// golf.md Phase 30.2 -- see `AggressiveOptions::macro_cse`.
    pub macro_cse: bool,
    /// golf.md Phase 30.4 -- see `AggressiveOptions::hoist_declarations`.
    pub hoist_declarations: bool,
}

impl From<UshaderGolfOptions> for AggressiveOptions {
    fn from(o: UshaderGolfOptions) -> Self {
        AggressiveOptions {
            eliminate_dead_locals: o.eliminate_dead_locals,
            eliminate_dead_stores: o.eliminate_dead_stores,
            fold_constants: o.fold_constants,
            reduce_constant_vectors: o.reduce_constant_vectors,
            strip_trailing_void_return: o.strip_trailing_void_return,
            compound_assignments: o.compound_assignments,
            increment_decrement: o.increment_decrement,
            ternary_from_if_else: o.ternary_from_if_else,
            merge_declarations: o.merge_declarations,
            strip_redundant_braces: o.strip_redundant_braces,
            strip_redundant_parens: o.strip_redundant_parens,
            strip_duplicate_precision: o.strip_duplicate_precision,
            eliminate_dead_functions: o.eliminate_dead_functions,
            inline_single_call_functions: o.inline_single_call_functions,
            simplify_algebraic_identities: o.simplify_algebraic_identities,
            eliminate_common_subexpressions: o.eliminate_common_subexpressions,
            fuse_statement_sequences: false,
            aggressive_inlining: o.aggressive_inlining,
            macro_cse: o.macro_cse,
            macro_cse_compression_budget: false,
            hoist_declarations: o.hoist_declarations,
            loop_header_golf: false,
            loop_form_golf: false,
            frequency_aware_renaming: false,
            factor_repeated_vector_args: false,
            swizzle_alphabet: crate::swizzle::SwizzleAlphabet::Auto,
        }
    }
}

#[repr(C)]
pub struct UshaderGolfStats {
    pub input_chars: usize,
    pub output_chars: usize,
    pub reduction_pct: f64,
    pub renamed_count: usize,
    pub numbers_shortened: usize,
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

impl From<GolfStats> for UshaderGolfStats {
    fn from(s: GolfStats) -> Self {
        UshaderGolfStats {
            input_chars: s.input_chars,
            output_chars: s.output_chars,
            reduction_pct: s.reduction_pct,
            renamed_count: s.renamed_count,
            numbers_shortened: s.numbers_shortened,
            compound_assignments: s.aggressive.compound_assignments,
            declarations_merged: s.aggressive.declarations_merged,
            braces_removed: s.aggressive.braces_removed,
            constants_folded: s.aggressive.constants_folded,
            dead_locals_removed: s.aggressive.dead_locals_removed,
            dead_stores_removed: s.aggressive.dead_stores_removed,
            constant_vectors_reduced: s.aggressive.constant_vectors_reduced,
            trailing_void_returns_removed: s.aggressive.trailing_void_returns_removed,
            increments_decrements: s.aggressive.increments_decrements,
            ternaries_from_if_else: s.aggressive.ternaries_from_if_else,
            redundant_parens_removed: s.aggressive.redundant_parens_removed,
            duplicate_precision_removed: s.aggressive.duplicate_precision_removed,
            dead_functions_removed: s.aggressive.dead_functions_removed,
            functions_inlined: s.aggressive.functions_inlined,
            algebraic_identities_simplified: s.aggressive.algebraic_identities_simplified,
            common_subexpressions_eliminated: s.aggressive.common_subexpressions_eliminated,
        }
    }
}

#[no_mangle]
pub extern "C" fn ushader_golf(
    source: *const c_char,
    options: UshaderGolfOptions,
    protected_names: *const c_char,
    out_stats: *mut UshaderGolfStats,
) -> *mut c_char {
    if source.is_null() {
        return std::ptr::null_mut();
    }

    let source = match unsafe { CStr::from_ptr(source) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    let names: Vec<String> = if protected_names.is_null() {
        Vec::new()
    } else {
        match unsafe { CStr::from_ptr(protected_names) }.to_str() {
            Ok(s) => s
                .split(',')
                .map(|s| s.trim().to_string())
                .filter(|s| !s.is_empty())
                .collect(),
            Err(_) => Vec::new(),
        }
    };

    let result = golf_with_protected_names(source, options.into(), &names);

    if !out_stats.is_null() {
        unsafe {
            *out_stats = result.stats.into();
        }
    }

    match CString::new(result.code) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn ushader_free_string(s: *mut c_char) {
    if s.is_null() {
        return;
    }
    unsafe {
        drop(CString::from_raw(s));
    }
}

#[repr(C)]
pub struct UshaderBudgetResult {
    pub raw_bytes: usize,
    pub deflate_bytes: usize,
}

#[no_mangle]
pub extern "C" fn ushader_estimate_budget(golfed: *const c_char) -> UshaderBudgetResult {
    if golfed.is_null() {
        return UshaderBudgetResult {
            raw_bytes: 0,
            deflate_bytes: 0,
        };
    }
    let source = match unsafe { CStr::from_ptr(golfed) }.to_str() {
        Ok(s) => s,
        Err(_) => {
            return UshaderBudgetResult {
                raw_bytes: 0,
                deflate_bytes: 0,
            }
        }
    };
    let result = estimate_budget(source);
    UshaderBudgetResult {
        raw_bytes: result.raw_bytes,
        deflate_bytes: result.deflate_bytes,
    }
}

fn json_escape_into(out: &mut String, s: &str) {
    for c in s.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            c if (c as u32) < 0x20 => out.push_str(&format!("\\u{:04x}", c as u32)),
            c => out.push(c),
        }
    }
}

fn trace_to_json(trace: &GolferTrace) -> String {
    let mut out = String::from("[");
    for (i, step) in trace.steps.iter().enumerate() {
        if i > 0 {
            out.push(',');
        }
        out.push_str("{\"pass_name\":\"");
        json_escape_into(&mut out, step.pass_name);
        out.push_str("\",\"before\":\"");
        json_escape_into(&mut out, &step.before);
        out.push_str("\",\"after\":\"");
        json_escape_into(&mut out, &step.after);
        out.push_str("\",\"count\":");
        out.push_str(&step.count.to_string());
        out.push('}');
    }
    out.push(']');
    out
}

#[no_mangle]
pub extern "C" fn ushader_golf_traced(
    source: *const c_char,
    options: UshaderGolfOptions,
    protected_names: *const c_char,
    out_stats: *mut UshaderGolfStats,
    out_trace_json: *mut *mut c_char,
) -> *mut c_char {
    if !out_trace_json.is_null() {
        unsafe {
            *out_trace_json = std::ptr::null_mut();
        }
    }

    if source.is_null() {
        return std::ptr::null_mut();
    }

    let source = match unsafe { CStr::from_ptr(source) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };

    let names: Vec<String> = if protected_names.is_null() {
        Vec::new()
    } else {
        match unsafe { CStr::from_ptr(protected_names) }.to_str() {
            Ok(s) => s
                .split(',')
                .map(|s| s.trim().to_string())
                .filter(|s| !s.is_empty())
                .collect(),
            Err(_) => Vec::new(),
        }
    };

    let (result, trace) = golf_with_protected_names_traced(source, options.into(), &names);

    if !out_stats.is_null() {
        unsafe {
            *out_stats = result.stats.into();
        }
    }

    if !out_trace_json.is_null() {
        if let Ok(c_string) = CString::new(trace_to_json(&trace)) {
            unsafe {
                *out_trace_json = c_string.into_raw();
            }
        }
    }

    match CString::new(result.code) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

fn twigl_mode_from_code(mode: i32) -> TwiglMode {
    match mode {
        1 => TwiglMode::Geek,
        2 => TwiglMode::Geeker,
        3 => TwiglMode::Geekest,
        _ => TwiglMode::Classic,
    }
}

#[no_mangle]
pub extern "C" fn ushader_twigl_rewrite(
    source: *const c_char,
    mode: i32,
    es300: bool,
) -> *mut c_char {
    if source.is_null() {
        return std::ptr::null_mut();
    }
    let source = match unsafe { CStr::from_ptr(source) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let rewritten = rewrite_twigl_shader(source, twigl_mode_from_code(mode), es300);
    match CString::new(rewritten) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn ushader_twigl_rewrite_mrt(
    source: *const c_char,
    mode: i32,
    mrt_targets: u8,
) -> *mut c_char {
    if source.is_null() {
        return std::ptr::null_mut();
    }
    let source = match unsafe { CStr::from_ptr(source) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let rewritten = rewrite_twigl_shader_mrt(source, twigl_mode_from_code(mode), mrt_targets);
    match CString::new(rewritten) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn ushader_twigl_snippet(name: *const c_char) -> *mut c_char {
    if name.is_null() {
        return std::ptr::null_mut();
    }
    let name = match unsafe { CStr::from_ptr(name) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    match twigl_snippet(name).and_then(|s| CString::new(s).ok()) {
        Some(c_string) => c_string.into_raw(),
        None => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn ushader_twigl_snippets_json() -> *mut c_char {
    let mut out = String::from("[");
    for (i, snippet) in twigl_snippets().iter().enumerate() {
        if i > 0 {
            out.push(',');
        }
        out.push_str("{\"name\":\"");
        json_escape_into(&mut out, snippet.name);
        out.push_str("\",\"source\":\"");
        json_escape_into(&mut out, snippet.source);
        out.push_str("\"}");
    }
    out.push(']');
    match CString::new(out) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn ushader_estimate_twigl_geekest_budget(source: *const c_char) -> UshaderBudgetResult {
    if source.is_null() {
        return UshaderBudgetResult { raw_bytes: 0, deflate_bytes: 0 };
    }
    let source = match unsafe { CStr::from_ptr(source) }.to_str() {
        Ok(s) => s,
        Err(_) => return UshaderBudgetResult { raw_bytes: 0, deflate_bytes: 0 },
    };
    let result = estimate_twigl_geekest_budget(source);
    UshaderBudgetResult { raw_bytes: result.raw_bytes, deflate_bytes: result.deflate_bytes }
}

#[no_mangle]
pub extern "C" fn ushader_twigl_export_uniform_names_json(
    mode: i32,
    mrt_targets: u8,
    has_backbuffer: bool,
    has_sound: bool,
) -> *mut c_char {
    let names = twigl_export_uniform_names(twigl_mode_from_code(mode), mrt_targets, has_backbuffer, has_sound);
    let mut out = String::from("[");
    for (i, name) in names.iter().enumerate() {
        if i > 0 {
            out.push(',');
        }
        out.push('"');
        json_escape_into(&mut out, name);
        out.push('"');
    }
    out.push(']');
    match CString::new(out) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}
