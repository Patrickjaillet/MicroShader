use crate::budget::estimate_budget;
use crate::gif::{encode_gif, GifFrame};
use crate::golfer::{
    golf_with_protected_names_traced, AggressiveOptions, GolferTrace,
    GolfStats,
};
use crate::search::{golf_harder, golf_harder_deep, AppliedChange, SearchObjective};
use crate::twigl::{
    resolve_rename_collisions, rewrite_twigl_shader_full, twigl_snippet, unrewrite_twigl_shader,
    TwiglMode,
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
    /// golf.md Phase 30.3 -- see `AggressiveOptions::fuse_statement_sequences`.
    pub fuse_statement_sequences: bool,
    /// golf.md Phase 29.1 -- see `AggressiveOptions::frequency_aware_renaming`.
    pub frequency_aware_renaming: bool,
    /// golf.md Phase 29.3 -- see `AggressiveOptions::factor_repeated_vector_args`.
    pub factor_repeated_vector_args: bool,
    /// golf.md Phase 29.2 -- see `AggressiveOptions::swizzle_alphabet`. 0 =
    /// Auto, 1 = xyzw, 2 = rgba, 3 = stpq; any other value falls back to Auto.
    pub swizzle_alphabet: i32,
    /// golf.md Phase 30.1 -- see `AggressiveOptions::aggressive_inlining`.
    pub aggressive_inlining: bool,
    /// golf.md Phase 30.2 -- see `AggressiveOptions::macro_cse`.
    pub macro_cse: bool,
    /// golf.md Phase 30.4 -- see `AggressiveOptions::hoist_declarations`.
    pub hoist_declarations: bool,
}

impl From<UshaderGolfOptions> for AggressiveOptions {
    fn from(o: UshaderGolfOptions) -> Self {
        let swizzle_alphabet = match o.swizzle_alphabet {
            1 => crate::swizzle::SwizzleAlphabet::Xyzw,
            2 => crate::swizzle::SwizzleAlphabet::Rgba,
            3 => crate::swizzle::SwizzleAlphabet::Stpq,
            _ => crate::swizzle::SwizzleAlphabet::Auto,
        };
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
            fuse_statement_sequences: o.fuse_statement_sequences,
            aggressive_inlining: o.aggressive_inlining,
            macro_cse: o.macro_cse,
            macro_cse_compression_budget: false,
            hoist_declarations: o.hoist_declarations,
            loop_header_golf: false,
            loop_form_golf: false,
            frequency_aware_renaming: o.frequency_aware_renaming,
            factor_repeated_vector_args: o.factor_repeated_vector_args,
            swizzle_alphabet,
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
    /// ROADMAP.md Phase 37.4 -- see `AggressiveStats::statement_sequences_fused`.
    pub statement_sequences_fused: usize,
    /// ROADMAP.md Phase 37.4 -- see `AggressiveStats::vector_args_factored`.
    pub vector_args_factored: usize,
    /// ROADMAP.md Phase 37.4 -- see `AggressiveStats::swizzles_recolored`.
    pub swizzles_recolored: usize,
    /// ROADMAP.md Phase 37.4 -- see `AggressiveStats::loop_headers_golfed`.
    pub loop_headers_golfed: usize,
    /// ROADMAP.md Phase 37.4 -- see `AggressiveStats::loop_forms_normalized`.
    pub loop_forms_normalized: usize,
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
            statement_sequences_fused: s.aggressive.statement_sequences_fused,
            vector_args_factored: s.aggressive.vector_args_factored,
            swizzles_recolored: s.aggressive.swizzles_recolored,
            loop_headers_golfed: s.aggressive.loop_headers_golfed,
            loop_forms_normalized: s.aggressive.loop_forms_normalized,
        }
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

// ROADMAP.md/roadmap_twigl.md Phase 45.1 -- binary (non-string) output, so
// this uses a small owned-buffer struct instead of the *mut c_char
// convention every other FFI function in this file uses. Always call
// ushader_free_byte_buffer on the result, even when `data` is null
// (`len` will be 0 in that case, and freeing is then a safe no-op) --
// keeps the calling convention uniform regardless of success/failure.
#[repr(C)]
pub struct UshaderByteBuffer {
    pub data: *mut u8,
    pub len: usize,
}

impl UshaderByteBuffer {
    fn empty() -> Self {
        UshaderByteBuffer { data: std::ptr::null_mut(), len: 0 }
    }
}

// `frames_rgba` is an array of `frame_count` pointers, each pointing to a
// `width * height * 4` byte RGBA8 buffer (row-major, top-to-bottom) --
// i.e. exactly what a `glReadPixels(..., GL_RGBA, GL_UNSIGNED_BYTE, ...)`
// call already produces per frame, so the caller can pass its existing
// per-frame pixel-read buffers directly with no reformatting. Returns an
// empty buffer (`data == null`, `len == 0`) if `frames_rgba` is null, any
// contained pointer is null, or the underlying `encode_gif` call fails
// (empty frame list, zero width/height, or a length mismatch it cannot
// detect from raw pointers alone -- the caller is trusted to pass buffers
// of the declared size, same trust boundary every other raw-pointer FFI
// function in this file already relies on).
#[no_mangle]
pub extern "C" fn ushader_encode_gif(
    frames_rgba: *const *const u8,
    frame_count: usize,
    width: u16,
    height: u16,
    delay_centiseconds: u16,
) -> UshaderByteBuffer {
    if frames_rgba.is_null() || frame_count == 0 || width == 0 || height == 0 {
        return UshaderByteBuffer::empty();
    }
    let frame_len = width as usize * height as usize * 4;
    let frame_ptrs: &[*const u8] = unsafe { std::slice::from_raw_parts(frames_rgba, frame_count) };
    if frame_ptrs.iter().any(|p| p.is_null()) {
        return UshaderByteBuffer::empty();
    }
    let slices: Vec<&[u8]> = frame_ptrs
        .iter()
        .map(|&p| unsafe { std::slice::from_raw_parts(p, frame_len) })
        .collect();
    let frames: Vec<GifFrame> = slices.iter().map(|s| GifFrame { rgba: s }).collect();

    match encode_gif(&frames, width, height, delay_centiseconds) {
        Some(bytes) => {
            let boxed = bytes.into_boxed_slice();
            let len = boxed.len();
            let data = Box::into_raw(boxed) as *mut u8;
            UshaderByteBuffer { data, len }
        }
        None => UshaderByteBuffer::empty(),
    }
}

#[no_mangle]
pub extern "C" fn ushader_free_byte_buffer(buffer: UshaderByteBuffer) {
    if buffer.data.is_null() {
        return;
    }
    unsafe {
        let slice = std::slice::from_raw_parts_mut(buffer.data, buffer.len);
        drop(Box::from_raw(slice as *mut [u8]));
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

fn applied_changes_to_json(applied: &[AppliedChange]) -> String {
    let mut out = String::from("[");
    for (i, change) in applied.iter().enumerate() {
        if i > 0 {
            out.push(',');
        }
        out.push_str("{\"pass_name\":\"");
        json_escape_into(&mut out, change.pass_name);
        out.push_str("\",\"from\":\"");
        json_escape_into(&mut out, &change.from);
        out.push_str("\",\"to\":\"");
        json_escape_into(&mut out, &change.to);
        out.push_str("\"}");
    }
    out.push(']');
    out
}

/// golf.md Phase 32.1 -- runs the bounded "Golf harder" pass-order/subset
/// search and returns the best combination found. `out_improved` is set to
/// whether it beat `options` alone (the caller should offer the result as a
/// one-click diff/apply, never replace the current output silently, per
/// this document's "nothing changes silently" precedent). `out_applied_json`
/// is a JSON array of `{pass_name, from, to}` describing every toggle the
/// search flipped relative to `options`, for UI explanation text.
#[no_mangle]
pub extern "C" fn ushader_golf_harder(
    source: *const c_char,
    options: UshaderGolfOptions,
    protected_names: *const c_char,
    compression_based: bool,
    out_stats: *mut UshaderGolfStats,
    out_improved: *mut bool,
    out_applied_json: *mut *mut c_char,
) -> *mut c_char {
    if !out_improved.is_null() {
        unsafe {
            *out_improved = false;
        }
    }
    if !out_applied_json.is_null() {
        unsafe {
            *out_applied_json = std::ptr::null_mut();
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

    let outcome = golf_harder(source, options.into(), &names, compression_based);

    if !out_stats.is_null() {
        unsafe {
            *out_stats = outcome.result.stats.into();
        }
    }
    if !out_improved.is_null() {
        unsafe {
            *out_improved = outcome.improved;
        }
    }
    if !out_applied_json.is_null() {
        if let Ok(c_string) = CString::new(applied_changes_to_json(&outcome.applied)) {
            unsafe {
                *out_applied_json = c_string.into_raw();
            }
        }
    }

    match CString::new(outcome.result.code) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

fn search_objective_from_code(code: i32) -> SearchObjective {
    match code {
        2 => SearchObjective::TwiglGeekest280,
        1 => SearchObjective::DeflateBytes,
        _ => SearchObjective::RawBytes,
    }
}

/// ROADMAP.md Phase 37.1/37.3 -- "Golf harder", extended: a simulated-
/// annealing search over the same candidate set as `ushader_golf_harder`,
/// scored against `objective` (0 = raw bytes, 1 = DEFLATE-estimated bytes,
/// 2 = the Twigl `geekest`-mode 280-character tweet budget), bounded by
/// both `max_iterations` and `max_duration_ms` (a wall-clock safety net --
/// pass a generous `max_iterations` and let `max_duration_ms` govern in
/// practice, per ROADMAP.md Phase 37.1's 2-second default). Same
/// never-changes-silently contract as `ushader_golf_harder`: the caller
/// must offer the result as a diff/apply, never replace the current
/// output automatically.
#[no_mangle]
pub extern "C" fn ushader_golf_harder_deep(
    source: *const c_char,
    options: UshaderGolfOptions,
    protected_names: *const c_char,
    objective: i32,
    max_iterations: usize,
    max_duration_ms: u64,
    out_stats: *mut UshaderGolfStats,
    out_improved: *mut bool,
    out_applied_json: *mut *mut c_char,
) -> *mut c_char {
    if !out_improved.is_null() {
        unsafe {
            *out_improved = false;
        }
    }
    if !out_applied_json.is_null() {
        unsafe {
            *out_applied_json = std::ptr::null_mut();
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

    let outcome = golf_harder_deep(
        source,
        options.into(),
        &names,
        search_objective_from_code(objective),
        max_iterations,
        std::time::Duration::from_millis(max_duration_ms),
    );

    if !out_stats.is_null() {
        unsafe {
            *out_stats = outcome.result.stats.into();
        }
    }
    if !out_improved.is_null() {
        unsafe {
            *out_improved = outcome.improved;
        }
    }
    if !out_applied_json.is_null() {
        if let Ok(c_string) = CString::new(applied_changes_to_json(&outcome.applied)) {
            unsafe {
                *out_applied_json = c_string.into_raw();
            }
        }
    }

    match CString::new(outcome.result.code) {
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

// The single combined entry point the C++ shell should call for every
// twigl-related output -- the live Export-panel preview, the budget badge,
// and the clipboard "Copy for twigl.app" action all call this so they can
// never diverge again. See ROADMAP.md/roadmap_twigl.md Phase 42.3/42.4.
#[no_mangle]
pub extern "C" fn ushader_twigl_rewrite_full(
    source: *const c_char,
    mode: i32,
    es300: bool,
    mrt_targets: u8,
    has_backbuffer: bool,
    has_sound: bool,
) -> *mut c_char {
    if source.is_null() {
        return std::ptr::null_mut();
    }
    let source = match unsafe { CStr::from_ptr(source) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let rewritten = rewrite_twigl_shader_full(
        source,
        twigl_mode_from_code(mode),
        es300,
        mrt_targets,
        has_backbuffer,
        has_sound,
    );
    match CString::new(rewritten) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

// ROADMAP.md/roadmap_twigl.md Phase 43.2 -- the reverse of
// ushader_twigl_rewrite: takes twigl-mode source (typed by hand, pasted from
// twigl.app, or found in the wild) and reconstructs a Shadertoy-compatible
// `void main(){}` fragment shader with standard iXxx uniform names, ready to
// paste into the Source editor.
#[no_mangle]
pub extern "C" fn ushader_twigl_unrewrite(source: *const c_char, mode: i32) -> *mut c_char {
    if source.is_null() {
        return std::ptr::null_mut();
    }
    let source = match unsafe { CStr::from_ptr(source) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let rewritten = unrewrite_twigl_shader(source, twigl_mode_from_code(mode));
    match CString::new(rewritten) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

// rewrite_twigl_shader_full already resolves every rename-target collision
// automatically (resolve_rename_collisions, called internally) -- this
// returns a single string joining (with "; ") a human-readable note for
// each rename it actually applied for this source/mode/es300/mrt_targets
// combination, so the UI can tell the user their shader was adjusted (e.g. a
// local `r` became `r_0`) instead of the export just silently differing from
// what they typed. Empty (not null) when no renames were needed. `mrt_targets`
// must match the value passed to ushader_twigl_rewrite_full for the same
// export, or this can miss/misreport renames (MRT and single-target use
// different collision checks -- see resolve_rename_collisions).
#[no_mangle]
pub extern "C" fn ushader_twigl_rename_collision_warnings(
    source: *const c_char,
    mode: i32,
    es300: bool,
    mrt_targets: u8,
) -> *mut c_char {
    if source.is_null() {
        return std::ptr::null_mut();
    }
    let source = match unsafe { CStr::from_ptr(source) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let (_, applied) =
        resolve_rename_collisions(source, twigl_mode_from_code(mode), es300, mrt_targets);
    match CString::new(applied.join("; ")) {
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

#[cfg(test)]
mod tests {
    // This module targets the FFI boundary itself (null-pointer handling,
    // the raw-pointer-array unsafe path in ushader_encode_gif, and the
    // owned-buffer free functions) rather than the golfing/twigl logic
    // those calls delegate to -- that logic already has its own extensive
    // test coverage in golfer.rs/twigl.rs/gif.rs. Before this module
    // existed, ushader_encode_gif's unsafe `slice::from_raw_parts` calls
    // over a caller-supplied pointer array had no test coverage anywhere,
    // Rust or C++.
    use super::*;
    use std::ffi::CString;

    fn zeroed_options() -> UshaderGolfOptions {
        // All-POD (bool/i32) struct -- zero-initializing is safe and gives
        // every pass "off", which is all these tests need.
        unsafe { std::mem::zeroed() }
    }

    #[test]
    fn ushader_golf_traced_returns_null_for_null_source() {
        let mut stats = unsafe { std::mem::zeroed() };
        let result = ushader_golf_traced(
            std::ptr::null(),
            zeroed_options(),
            std::ptr::null(),
            &mut stats,
            std::ptr::null_mut(),
        );
        assert!(result.is_null());
    }

    #[test]
    fn ushader_golf_traced_golfs_and_emits_trace_json() {
        let source = CString::new(
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}",
        )
        .unwrap();
        let mut stats = unsafe { std::mem::zeroed() };
        let mut trace_json: *mut c_char = std::ptr::null_mut();
        let golfed = ushader_golf_traced(
            source.as_ptr(),
            zeroed_options(),
            std::ptr::null(),
            &mut stats,
            &mut trace_json,
        );
        assert!(!golfed.is_null());
        assert!(!trace_json.is_null());
        let trace_text = unsafe { CStr::from_ptr(trace_json) }.to_str().unwrap();
        assert!(trace_text.starts_with('['), "trace should be a JSON array: {trace_text}");
        ushader_free_string(golfed);
        ushader_free_string(trace_json);
    }

    #[test]
    fn ushader_free_string_on_null_is_a_safe_noop() {
        ushader_free_string(std::ptr::null_mut());
    }

    #[test]
    fn ushader_encode_gif_returns_empty_buffer_for_null_frame_array() {
        let buffer = ushader_encode_gif(std::ptr::null(), 1, 4, 4, 10);
        assert!(buffer.data.is_null());
        assert_eq!(buffer.len, 0);
        ushader_free_byte_buffer(buffer);
    }

    #[test]
    fn ushader_encode_gif_returns_empty_buffer_for_zero_dimensions_or_frame_count() {
        let frame = [0u8; 16];
        let ptr: *const u8 = frame.as_ptr();
        let frames: [*const u8; 1] = [ptr];

        let zero_count = ushader_encode_gif(frames.as_ptr(), 0, 2, 2, 10);
        assert!(zero_count.data.is_null());
        ushader_free_byte_buffer(zero_count);

        let zero_width = ushader_encode_gif(frames.as_ptr(), 1, 0, 2, 10);
        assert!(zero_width.data.is_null());
        ushader_free_byte_buffer(zero_width);

        let zero_height = ushader_encode_gif(frames.as_ptr(), 1, 2, 0, 10);
        assert!(zero_height.data.is_null());
        ushader_free_byte_buffer(zero_height);
    }

    #[test]
    fn ushader_encode_gif_returns_empty_buffer_for_a_null_frame_pointer() {
        let frames: [*const u8; 2] = [std::ptr::null(), std::ptr::null()];
        let buffer = ushader_encode_gif(frames.as_ptr(), 2, 2, 2, 10);
        assert!(buffer.data.is_null());
        ushader_free_byte_buffer(buffer);
    }

    #[test]
    fn ushader_encode_gif_encodes_a_solid_frame_and_round_trips_through_free() {
        // A single 2x2 opaque-red RGBA8 frame -- exactly the layout
        // `glReadPixels(..., GL_RGBA, GL_UNSIGNED_BYTE, ...)` would produce.
        let frame: [u8; 16] = [
            255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255,
        ];
        let frames: [*const u8; 1] = [frame.as_ptr()];
        let buffer = ushader_encode_gif(frames.as_ptr(), 1, 2, 2, 10);
        assert!(!buffer.data.is_null());
        assert!(buffer.len > 0);
        // GIF89a magic bytes.
        let bytes = unsafe { std::slice::from_raw_parts(buffer.data, buffer.len) };
        assert_eq!(&bytes[0..3], b"GIF");
        ushader_free_byte_buffer(buffer);
    }

    #[test]
    fn ushader_estimate_budget_returns_zero_for_null_input() {
        let result = ushader_estimate_budget(std::ptr::null());
        assert_eq!(result.raw_bytes, 0);
        assert_eq!(result.deflate_bytes, 0);
    }

    #[test]
    fn ushader_estimate_budget_returns_nonzero_for_real_source() {
        let source = CString::new("void mainImage(out vec4 a,in vec2 b){a=vec4(1.);}").unwrap();
        let result = ushader_estimate_budget(source.as_ptr());
        assert!(result.raw_bytes > 0);
        assert!(result.deflate_bytes > 0);
    }
}

