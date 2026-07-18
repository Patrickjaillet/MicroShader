use crate::golfer::{golf_with_protected_names, AggressiveOptions};
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
        }
    }
}

#[no_mangle]
pub extern "C" fn ushader_golf(
    source: *const c_char,
    options: UshaderGolfOptions,
    protected_names: *const c_char,
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
