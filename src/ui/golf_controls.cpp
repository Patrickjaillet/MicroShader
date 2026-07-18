#include "golf_controls.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "theme.h"
#include "theme_tokens.h"

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
    themed_checkbox("Aggressive golf", &toggles.aggressive);

    ImGui::BeginDisabled(!toggles.aggressive);
    if (ImGui::CollapsingHeader("PASSES"))
    {
        ImGui::Columns(2, nullptr, false);
        themed_checkbox("Dead locals", &toggles.eliminate_dead_locals);
        themed_checkbox("Dead stores", &toggles.eliminate_dead_stores);
        themed_checkbox("Fold constants", &toggles.fold_constants);
        themed_checkbox("Constant vectors", &toggles.reduce_constant_vectors);
        themed_checkbox("Trailing return", &toggles.strip_trailing_void_return);
        themed_checkbox("Compound assign", &toggles.compound_assignments);
        themed_checkbox("Increment/decrement", &toggles.increment_decrement);
        ImGui::NextColumn();
        themed_checkbox("Ternary", &toggles.ternary_from_if_else);
        themed_checkbox("Merge declarations", &toggles.merge_declarations);
        themed_checkbox("Redundant braces", &toggles.strip_redundant_braces);
        themed_checkbox("Redundant parens", &toggles.strip_redundant_parens);
        themed_checkbox("Duplicate precision", &toggles.strip_duplicate_precision);
        themed_checkbox("Dead functions", &toggles.eliminate_dead_functions);
        themed_checkbox("Inline single-call", &toggles.inline_single_call_functions);
        ImGui::Columns(1);
    }
    ImGui::EndDisabled();

    ImGui::InputTextWithHint("Protected names", "comma-separated, e.g. myUniform", &protected_names);
    if (ImGui::IsItemActive())
    {
        ImGui::GetWindowDrawList()->AddRect(
            ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
            ImGui::GetColorU32(tokens::accent), ImGui::GetStyle().FrameRounding, 0, 1.5f);
    }
}
