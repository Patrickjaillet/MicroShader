#pragma once

#include "ushader/golf_core.h"

struct GolfPassToggles
{
    bool aggressive = true;
    bool eliminate_dead_locals = true;
    bool eliminate_dead_stores = true;
    bool fold_constants = true;
    bool reduce_constant_vectors = true;
    bool strip_trailing_void_return = true;
    bool compound_assignments = true;
    bool increment_decrement = true;
    bool ternary_from_if_else = true;
    bool merge_declarations = true;
    bool strip_redundant_braces = true;
    bool strip_redundant_parens = true;
    bool strip_duplicate_precision = true;
    bool eliminate_dead_functions = true;
    bool inline_single_call_functions = true;
    bool simplify_algebraic_identities = true;
    bool eliminate_common_subexpressions = true;
    bool fuse_statement_sequences = true;
    // Phase 29.1 — default on in "Maximum", default off in "Safe" (gated
    // purely for user predictability/diffability, not safety — see golf.md).
    bool frequency_aware_renaming = false;
    // Phase 29.3.
    bool factor_repeated_vector_args = true;
    // Phase 29.2 — SwizzleAlphabetChoice below (kept in sync with
    // rust-core's `SwizzleAlphabet`: Auto = 0, Xyzw = 1, Rgba = 2, Stpq = 3).
    int swizzle_alphabet = 0;
    // Phase 30.1 — off by default even in "Maximum": the one pass in
    // golf.md this whole document that can legitimately make output
    // larger if mis-tuned, so it stays an explicit opt-in even for
    // competitive users (see golf.md Phase 30.1's own stated rationale).
    bool aggressive_inlining = false;
    // Phase 30.2 — on by default, same stability-only exclusion from the
    // shared Rust `all()` helper as `fuse_statement_sequences` above (not
    // a correctness/size-risk exclusion like `aggressive_inlining`).
    bool macro_cse = true;
    // Phase 30.4 — on by default, same rationale as `macro_cse` above.
    bool hoist_declarations = true;
};

// Mirrors rust-core's `SwizzleAlphabet` (golf.md Phase 29.2) for use by
// the `swizzle_alphabet` combo in golf_controls.cpp.
enum class SwizzleAlphabetChoice : int
{
    Auto = 0,
    Xyzw = 1,
    Rgba = 2,
    Stpq = 3,
};

UshaderGolfOptions to_golf_options(const GolfPassToggles& toggles);
