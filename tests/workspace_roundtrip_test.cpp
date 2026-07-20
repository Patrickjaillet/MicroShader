#include <cstdio>
#include <string>

#include "../src/ui/workspace.h"

namespace
{
    int failures = 0;

    void check(bool condition, const char* message)
    {
        if (!condition)
        {
            std::printf("FAIL: %s\n", message);
            ++failures;
        }
    }

    bool toggles_equal(const GolfPassToggles& a, const GolfPassToggles& b)
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
}

int main()
{
    WorkspaceState original;
    original.active_tab = 1;
    original.layout_ini = "layout.ini";

    WorkspaceDocument first;
    first.file_path = "C:\\shaders\\one.glsl";
    first.protected_names = "iChannel0, myUniform";
    first.budget_preset_index = 1;
    first.pass_toggles.fold_constants = false;
    first.pass_toggles.merge_declarations = false;
    original.documents.push_back(first);

    WorkspaceDocument second;
    second.file_path = "D:\\path with spaces\\two.glsl";
    second.protected_names = "";
    second.budget_preset_index = 3;
    second.pass_toggles.aggressive = false;
    original.documents.push_back(second);

    std::string serialized = serialize_workspace(original);

    WorkspaceState parsed;
    check(deserialize_workspace(serialized, parsed), "deserialize_workspace returned false");
    check(parsed.active_tab == original.active_tab, "active_tab mismatch");
    check(parsed.layout_ini == original.layout_ini, "layout_ini mismatch");
    check(parsed.documents.size() == original.documents.size(), "document count mismatch");

    if (parsed.documents.size() == original.documents.size())
    {
        for (std::size_t i = 0; i < parsed.documents.size(); ++i)
        {
            check(parsed.documents[i].file_path == original.documents[i].file_path, "file_path mismatch");
            check(parsed.documents[i].protected_names == original.documents[i].protected_names, "protected_names mismatch");
            check(parsed.documents[i].budget_preset_index == original.documents[i].budget_preset_index, "budget_preset_index mismatch");
            check(toggles_equal(parsed.documents[i].pass_toggles, original.documents[i].pass_toggles), "pass_toggles mismatch");
        }
    }

    WorkspaceState empty_parsed;
    check(deserialize_workspace("not json", empty_parsed) == false, "deserialize should reject non-JSON");

    if (failures == 0)
    {
        std::printf("workspace_roundtrip_test: OK\n");
        return 0;
    }
    std::printf("workspace_roundtrip_test: %d failure(s)\n", failures);
    return 1;
}
