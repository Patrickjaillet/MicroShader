#include "golf_controls.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <optional>
#include <string>

#include "budget_presets.h"
#include "exclude_list_import.h"
#include "golf_profile.h"
#include "theme.h"
#include "theme_tokens.h"
#include "../platform/file_dialog.h"
#include "../platform/paths.h"
#include "../platform/utf8.h"

namespace
{
    const wchar_t kProfileFilter[] = L"uShader profile (*.ushaderprofile)\0*.ushaderprofile\0All files (*.*)\0*.*\0";
}

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

void save_golf_profile_action(const GolfPassToggles& toggles, const std::string& protected_names, int budget_preset_index, std::string& last_profile_path, SDL_Window* window)
{
    std::wstring default_name = last_profile_path.empty() ? L"profile.ushaderprofile" : utf8_to_wide(last_profile_path);
    std::optional<std::string> path = show_save_file_dialog(window, kProfileFilter, L"ushaderprofile", default_name.c_str());
    if (path.has_value())
    {
        write_utf8_file(*path, serialize_golf_profile(toggles, protected_names, budget_preset_index));
        last_profile_path = *path;
        save_last_profile_path(last_profile_path);
    }
}

void load_golf_profile_action(GolfPassToggles& toggles, std::string& protected_names, int& budget_preset_index, std::string& last_profile_path, SDL_Window* window)
{
    std::wstring default_path = utf8_to_wide(last_profile_path);
    std::optional<std::string> path = show_open_file_dialog(window, kProfileFilter, L"ushaderprofile", last_profile_path.empty() ? nullptr : default_path.c_str());
    if (path.has_value())
    {
        deserialize_golf_profile(read_utf8_file(*path), toggles, protected_names, budget_preset_index);
        last_profile_path = *path;
        save_last_profile_path(last_profile_path);
    }
}

void render_golf_controls(GolfPassToggles& toggles, std::string& protected_names, int& budget_preset_index, std::string& last_profile_path, SDL_Window* window)
{
    if (ImGui::Button("Save profile..."))
    {
        save_golf_profile_action(toggles, protected_names, budget_preset_index, last_profile_path, window);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load profile..."))
    {
        load_golf_profile_action(toggles, protected_names, budget_preset_index, last_profile_path, window);
    }

    const char* current_profile_name = builtin_profile_name(toggles);
    if (ImGui::BeginCombo("Profile", current_profile_name))
    {
        static const char* kBuiltinProfileNames[] = {"Maximum", "Safe", "None"};
        for (const char* name : kBuiltinProfileNames)
        {
            bool selected = std::string(current_profile_name) == name;
            if (ImGui::Selectable(name, selected))
            {
                if (std::string(name) == "Maximum")
                {
                    toggles = builtin_profile_maximum();
                }
                else if (std::string(name) == "Safe")
                {
                    toggles = builtin_profile_safe();
                }
                else
                {
                    toggles = builtin_profile_none();
                }
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

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
        themed_checkbox("Algebraic identities", &toggles.simplify_algebraic_identities);
        ImGui::NextColumn();
        themed_checkbox("Ternary", &toggles.ternary_from_if_else);
        themed_checkbox("Merge declarations", &toggles.merge_declarations);
        themed_checkbox("Redundant braces", &toggles.strip_redundant_braces);
        themed_checkbox("Redundant parens", &toggles.strip_redundant_parens);
        themed_checkbox("Duplicate precision", &toggles.strip_duplicate_precision);
        themed_checkbox("Dead functions", &toggles.eliminate_dead_functions);
        themed_checkbox("Inline single-call", &toggles.inline_single_call_functions);
        themed_checkbox("Common subexpressions", &toggles.eliminate_common_subexpressions);
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
    ImGui::SameLine();
    if (ImGui::Button("Import exclude list..."))
    {
        import_exclude_list_action(protected_names, window);
    }

    std::size_t preset_count = 0;
    const BudgetPreset* presets = budget_presets(preset_count);
    const char* current_name =
        (budget_preset_index >= 0 && static_cast<std::size_t>(budget_preset_index) < preset_count)
            ? presets[budget_preset_index].name
            : "Custom";
    if (ImGui::BeginCombo("Budget preset", current_name))
    {
        for (std::size_t i = 0; i < preset_count; ++i)
        {
            bool selected = static_cast<int>(i) == budget_preset_index;
            if (ImGui::Selectable(presets[i].name, selected))
            {
                budget_preset_index = static_cast<int>(i);
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}
