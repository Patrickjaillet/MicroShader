#include <cassert>
#include <cstdio>
#include <cstddef>
#include <string>
#include <vector>

#include "../src/ui/golf_profile.h"
#include "../src/ui/budget_presets.h"

namespace
{
    bool contains_key(const std::string& text, const std::string& key)
    {
        return text.find("\"" + key + "\"") != std::string::npos;
    }
}

int main()
{
    GolfPassToggles toggles{};
    std::string serialized = serialize_golf_profile(toggles, "iTime,iResolution,mainImage", 0);

    // Every field documented in docs/ushaderprofile-schema.md and
    // docs/ushaderprofile.schema.json must actually be written by the
    // real serializer, so the published schema can't silently drift
    // from the implementation.
    static const char* kDocumentedKeys[] = {
        "schema_version",
        "aggressive",
        "eliminate_dead_locals",
        "eliminate_dead_stores",
        "fold_constants",
        "reduce_constant_vectors",
        "strip_trailing_void_return",
        "compound_assignments",
        "increment_decrement",
        "ternary_from_if_else",
        "merge_declarations",
        "strip_redundant_braces",
        "strip_redundant_parens",
        "strip_duplicate_precision",
        "eliminate_dead_functions",
        "inline_single_call_functions",
        "simplify_algebraic_identities",
        "eliminate_common_subexpressions",
        "protected_names",
        "budget_preset",
    };

    int failures = 0;
    for (const char* key : kDocumentedKeys)
    {
        if (!contains_key(serialized, key))
        {
            std::fprintf(stderr, "documented field \"%s\" missing from serialize_golf_profile output\n", key);
            failures += 1;
        }
    }

    if (serialized.find("\"schema_version\": 1") == std::string::npos)
    {
        std::fprintf(stderr, "expected schema_version 1 to be stamped into the serialized profile\n");
        failures += 1;
    }

    // The schema's budget_preset enum must match the real preset
    // list, or an external tool validating against the schema would
    // reject a perfectly valid uShader-written profile.
    static const char* kDocumentedBudgetPresets[] = {
        "Shadertoy",
        "X/Twitter shader",
        "JS13K-style 13KB",
        "4KB intro",
        "8KB intro",
        "64KB intro",
    };

    std::size_t preset_count = 0;
    const BudgetPreset* presets = budget_presets(preset_count);
    if (preset_count != sizeof(kDocumentedBudgetPresets) / sizeof(kDocumentedBudgetPresets[0]))
    {
        std::fprintf(stderr, "budget preset count changed (%zu) without updating the documented schema enum\n", preset_count);
        failures += 1;
    }
    else
    {
        for (std::size_t i = 0; i < preset_count; ++i)
        {
            if (std::string(presets[i].name) != kDocumentedBudgetPresets[i])
            {
                std::fprintf(stderr, "budget preset %zu is \"%s\" but the documented schema enum says \"%s\"\n",
                    i, presets[i].name, kDocumentedBudgetPresets[i]);
                failures += 1;
            }
        }
    }

    if (failures == 0)
    {
        std::printf("ushaderprofile_schema_test: all tests passed\n");
    }

    return failures;
}
