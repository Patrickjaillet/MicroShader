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
