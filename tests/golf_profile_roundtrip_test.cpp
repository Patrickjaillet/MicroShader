#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "ushader/golf_core.h"
#include "../src/ui/golf_profile.h"

namespace
{
    std::string read_fixture(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool toggles_field_equal(const GolfPassToggles& a, const GolfPassToggles& b)
    {
        return a.aggressive == b.aggressive
            && a.eliminate_dead_locals == b.eliminate_dead_locals
            && a.eliminate_dead_stores == b.eliminate_dead_stores
            && a.fold_constants == b.fold_constants
            && a.reduce_constant_vectors == b.reduce_constant_vectors
            && a.strip_trailing_void_return == b.strip_trailing_void_return
            && a.compound_assignments == b.compound_assignments
            && a.increment_decrement == b.increment_decrement
            && a.ternary_from_if_else == b.ternary_from_if_else
            && a.merge_declarations == b.merge_declarations
            && a.strip_redundant_braces == b.strip_redundant_braces
            && a.strip_redundant_parens == b.strip_redundant_parens
            && a.strip_duplicate_precision == b.strip_duplicate_precision
            && a.eliminate_dead_functions == b.eliminate_dead_functions
            && a.inline_single_call_functions == b.inline_single_call_functions
            && a.simplify_algebraic_identities == b.simplify_algebraic_identities
            && a.eliminate_common_subexpressions == b.eliminate_common_subexpressions;
    }

    UshaderGolfOptions to_golf_options_local(const GolfPassToggles& toggles)
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

    bool golf_options_equal(const UshaderGolfOptions& a, const UshaderGolfOptions& b)
    {
        return std::memcmp(&a, &b, sizeof(UshaderGolfOptions)) == 0;
    }
}

int main()
{
    int failures = 0;

    std::string fixture_text = read_fixture(USHADER_FIXTURE_PATH);
    if (fixture_text.empty())
    {
        std::fprintf(stderr, "could not read fixture at %s\n", USHADER_FIXTURE_PATH);
        return 1;
    }

    GolfPassToggles loaded{};
    std::string loaded_protected_names;
    int loaded_budget_preset_index = -2;
    if (!deserialize_golf_profile(fixture_text, loaded, loaded_protected_names, loaded_budget_preset_index))
    {
        std::fprintf(stderr, "deserialize_golf_profile failed on checked-in fixture\n");
        return 1;
    }

    GolfPassToggles expected{};
    expected.aggressive = true;
    expected.eliminate_dead_locals = true;
    expected.eliminate_dead_stores = true;
    expected.fold_constants = false;
    expected.reduce_constant_vectors = true;
    expected.strip_trailing_void_return = false;
    expected.compound_assignments = true;
    expected.increment_decrement = false;
    expected.ternary_from_if_else = true;
    expected.merge_declarations = false;
    expected.strip_redundant_braces = true;
    expected.strip_redundant_parens = false;
    expected.strip_duplicate_precision = true;
    expected.eliminate_dead_functions = false;
    expected.inline_single_call_functions = true;
    expected.simplify_algebraic_identities = false;
    expected.eliminate_common_subexpressions = true;

    if (!toggles_field_equal(loaded, expected))
    {
        std::fprintf(stderr, "fixture toggles do not match the expected AggressiveOptions\n");
        failures += 1;
    }
    if (loaded_protected_names != "iTime,iResolution,mainImage")
    {
        std::fprintf(stderr, "fixture protected_names mismatch: %s\n", loaded_protected_names.c_str());
        failures += 1;
    }
    if (loaded_budget_preset_index < 0)
    {
        std::fprintf(stderr, "fixture budget_preset did not resolve to a known preset\n");
        failures += 1;
    }

    std::string resaved_text = serialize_golf_profile(loaded, loaded_protected_names, loaded_budget_preset_index);

    GolfPassToggles reloaded{};
    std::string reloaded_protected_names;
    int reloaded_budget_preset_index = -2;
    if (!deserialize_golf_profile(resaved_text, reloaded, reloaded_protected_names, reloaded_budget_preset_index))
    {
        std::fprintf(stderr, "deserialize_golf_profile failed on the re-saved profile\n");
        return 1;
    }

    if (!toggles_field_equal(loaded, reloaded))
    {
        std::fprintf(stderr, "save then reload did not reproduce the identical AggressiveOptions\n");
        failures += 1;
    }
    if (loaded_protected_names != reloaded_protected_names)
    {
        std::fprintf(stderr, "save then reload did not reproduce the identical protected_names\n");
        failures += 1;
    }
    if (loaded_budget_preset_index != reloaded_budget_preset_index)
    {
        std::fprintf(stderr, "save then reload did not reproduce the identical budget_preset\n");
        failures += 1;
    }

    UshaderGolfOptions options_from_loaded = to_golf_options_local(loaded);
    UshaderGolfOptions options_from_reloaded = to_golf_options_local(reloaded);
    if (!golf_options_equal(options_from_loaded, options_from_reloaded))
    {
        std::fprintf(stderr, "UshaderGolfOptions produced from the round-tripped profile diverged\n");
        failures += 1;
    }

    UshaderGolfStats stats{};
    char* golfed = ushader_golf(
        "void mainImage(out vec4 fragColor,in vec2 fragCoord){float x=1.0;x=2.0;fragColor=vec4(x);}",
        options_from_reloaded,
        loaded_protected_names.c_str(),
        &stats);
    if (golfed == nullptr)
    {
        std::fprintf(stderr, "the round-tripped AggressiveOptions failed to golf through the real Rust engine\n");
        failures += 1;
    }
    else
    {
        std::printf("round-tripped profile golfs to: %s\n", golfed);
        ushader_free_string(golfed);
    }

    if (failures == 0)
    {
        std::printf("all checks passed\n");
    }

    return failures;
}
