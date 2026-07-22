#include "golf_controls.h"

UshaderGolfOptions to_golf_options(const GolfPassToggles& toggles)
{
    if (!toggles.aggressive)
    {
        return UshaderGolfOptions{};
    }
    return UshaderGolfOptions{
        toggles.eliminate_dead_locals,
        toggles.eliminate_dead_stores,
        toggles.fold_constants,
        toggles.reduce_constant_vectors,
        toggles.strip_trailing_void_return,
        toggles.compound_assignments,
        toggles.increment_decrement,
        toggles.ternary_from_if_else,
        toggles.merge_declarations,
        toggles.strip_redundant_braces,
        toggles.strip_redundant_parens,
        toggles.strip_duplicate_precision,
        toggles.eliminate_dead_functions,
        toggles.inline_single_call_functions,
        toggles.simplify_algebraic_identities,
        toggles.eliminate_common_subexpressions,
    };
}
