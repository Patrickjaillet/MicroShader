use crate::golfer::{golf_with_protected_names, AggressiveOptions, GolfStats};
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
