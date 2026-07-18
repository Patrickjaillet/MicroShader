#include "golf_controls.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

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
    };
}

void render_golf_controls(GolfPassToggles& toggles, std::string& protected_names)
{
    ImGui::Checkbox("Aggressive golf", &toggles.aggressive);

    ImGui::BeginDisabled(!toggles.aggressive);
    if (ImGui::CollapsingHeader("Passes"))
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::Checkbox("Dead locals", &toggles.eliminate_dead_locals);
        ImGui::Checkbox("Dead stores", &toggles.eliminate_dead_stores);
        ImGui::Checkbox("Fold constants", &toggles.fold_constants);
        ImGui::Checkbox("Constant vectors", &toggles.reduce_constant_vectors);
        ImGui::Checkbox("Trailing return", &toggles.strip_trailing_void_return);
        ImGui::Checkbox("Compound assign", &toggles.compound_assignments);
        ImGui::Checkbox("Increment/decrement", &toggles.increment_decrement);
        ImGui::NextColumn();
        ImGui::Checkbox("Ternary", &toggles.ternary_from_if_else);
        ImGui::Checkbox("Merge declarations", &toggles.merge_declarations);
        ImGui::Checkbox("Redundant braces", &toggles.strip_redundant_braces);
        ImGui::Checkbox("Redundant parens", &toggles.strip_redundant_parens);
        ImGui::Checkbox("Duplicate precision", &toggles.strip_duplicate_precision);
        ImGui::Checkbox("Dead functions", &toggles.eliminate_dead_functions);
        ImGui::Checkbox("Inline single-call", &toggles.inline_single_call_functions);
        ImGui::Columns(1);
    }
    ImGui::EndDisabled();

    ImGui::InputTextWithHint("Protected names", "comma-separated, e.g. myUniform", &protected_names);
}
