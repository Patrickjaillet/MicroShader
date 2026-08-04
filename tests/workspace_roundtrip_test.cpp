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
            && a.eliminate_common_subexpressions == b.eliminate_common_subexpressions
            && a.frequency_aware_renaming == b.frequency_aware_renaming;
    }
}

int main()
{
    WorkspaceState original;
    original.active_tab = 1;
    original.active_document = 2;
    original.layout_ini = "layout.ini";

    WorkspaceDocument first;
    first.file_path = "C:\\shaders\\one.glsl";
    first.protected_names = "iChannel0, myUniform";
    first.budget_preset_index = 1;
    first.pass_toggles.fold_constants = false;
    first.pass_toggles.merge_declarations = false;
    // ROADMAP.md/roadmap_twigl.md Phase 44.3 -- deliberately every field
    // set away from its default, so a bug that silently fell back to
    // defaults on restore would be caught rather than masked.
    first.twigl_mode = 3;
    first.twigl_es300 = true;
    first.twigl_mrt_targets = 2;
    first.twigl_has_backbuffer = true;
    first.twigl_has_sound = true;
    original.documents.push_back(first);

    WorkspaceDocument second;
    second.file_path = "D:\\path with spaces\\two.glsl";
    second.protected_names = "";
    second.budget_preset_index = 3;
    second.pass_toggles.aggressive = false;
    original.documents.push_back(second);

    WorkspaceDocument third;
    third.file_path = "";
    third.unsaved_source = "void mainImage(out vec4 fragColor,in vec2 fragCoord)\n{\n    fragColor=vec4(1.0);\n}\n";
    third.protected_names = "mainImage";
    third.budget_preset_index = -1;
    original.documents.push_back(third);

    // Regression test: a never-saved document the user cleared to an empty
    // buffer must round-trip as empty, not silently coerce back to the
    // default shader on restore (that coercion decision belongs to the
    // caller, not this serializer -- see main_win32.cpp's restore path).
    WorkspaceDocument fourth;
    fourth.file_path = "";
    fourth.unsaved_source = "";
    fourth.protected_names = "";
    fourth.budget_preset_index = -1;
    original.documents.push_back(fourth);

    std::string serialized = serialize_workspace(original);

    WorkspaceState parsed;
    check(deserialize_workspace(serialized, parsed), "deserialize_workspace returned false");
    check(parsed.active_tab == original.active_tab, "active_tab mismatch");
    check(parsed.active_document == original.active_document, "active_document mismatch");
    check(parsed.layout_ini == original.layout_ini, "layout_ini mismatch");
    check(parsed.documents.size() == original.documents.size(), "document count mismatch");

    if (parsed.documents.size() == original.documents.size())
    {
        for (std::size_t i = 0; i < parsed.documents.size(); ++i)
        {
            check(parsed.documents[i].file_path == original.documents[i].file_path, "file_path mismatch");
            check(parsed.documents[i].protected_names == original.documents[i].protected_names, "protected_names mismatch");
            check(parsed.documents[i].budget_preset_index == original.documents[i].budget_preset_index, "budget_preset_index mismatch");
            check(parsed.documents[i].unsaved_source == original.documents[i].unsaved_source, "unsaved_source mismatch");
            check(toggles_equal(parsed.documents[i].pass_toggles, original.documents[i].pass_toggles), "pass_toggles mismatch");
            check(parsed.documents[i].twigl_mode == original.documents[i].twigl_mode, "twigl_mode mismatch");
            check(parsed.documents[i].twigl_es300 == original.documents[i].twigl_es300, "twigl_es300 mismatch");
            check(parsed.documents[i].twigl_mrt_targets == original.documents[i].twigl_mrt_targets, "twigl_mrt_targets mismatch");
            check(parsed.documents[i].twigl_has_backbuffer == original.documents[i].twigl_has_backbuffer, "twigl_has_backbuffer mismatch");
            check(parsed.documents[i].twigl_has_sound == original.documents[i].twigl_has_sound, "twigl_has_sound mismatch");
        }
    }

    WorkspaceState empty_parsed;
    check(deserialize_workspace("not json", empty_parsed) == false, "deserialize should reject non-JSON");

    // ROADMAP.md/roadmap_twigl.md Phase 44.3 -- a profile saved before this
    // field existed has no "twigl_*" keys at all; deserialize_workspace
    // must fall back to WorkspaceDocument's own defaults rather than fail
    // or leave the fields uninitialized, matching every other schema field
    // in this file's own backward-compatibility precedent.
    {
        std::string legacy_json =
            "{\n"
            "  \"version\": 1,\n"
            "  \"active_tab\": 0,\n"
            "  \"active_document\": 0,\n"
            "  \"ui_font_size\": 18.0,\n"
            "  \"layout_ini\": \"\",\n"
            "  \"documents\": [\n"
            "    { \"path\": \"C:\\\\legacy.glsl\", \"unsaved_source\": \"\" }\n"
            "  ]\n"
            "}\n";
        WorkspaceState legacy_parsed;
        check(deserialize_workspace(legacy_json, legacy_parsed), "legacy deserialize_workspace returned false");
        check(legacy_parsed.documents.size() == 1, "legacy document count mismatch");
        if (legacy_parsed.documents.size() == 1)
        {
            const WorkspaceDocument& doc = legacy_parsed.documents[0];
            check(doc.twigl_mode == 0, "legacy twigl_mode should default to 0");
            check(doc.twigl_es300 == false, "legacy twigl_es300 should default to false");
            check(doc.twigl_mrt_targets == 1, "legacy twigl_mrt_targets should default to 1");
            check(doc.twigl_has_backbuffer == false, "legacy twigl_has_backbuffer should default to false");
            check(doc.twigl_has_sound == false, "legacy twigl_has_sound should default to false");
        }
    }

    if (failures == 0)
    {
        std::printf("workspace_roundtrip_test: OK\n");
        return 0;
    }
    std::printf("workspace_roundtrip_test: %d failure(s)\n", failures);
    return 1;
}
