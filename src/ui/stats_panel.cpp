#include "stats_panel.h"

#include <cstdio>

#include <imgui.h>

#include "budget_presets.h"
#include "theme_tokens.h"

namespace
{
    void render_budget_badge(const char* label, long long limit, std::size_t actual)
    {
        if (limit < 0)
        {
            return;
        }
        auto limit_u = static_cast<std::size_t>(limit);
        bool over = actual > limit_u;
        bool near_limit = !over && limit_u > 0 && actual * 10 >= limit_u * 9;
        ImVec4 color = over ? tokens::status_error : (near_limit ? tokens::status_warning : tokens::status_ok);
        const char* glyph = over ? "\xc3\x97" : "\xe2\x9c\x93";
        char text[96];
        std::snprintf(text, sizeof(text), "%s %s: %zu / %lld", glyph, label, actual, limit);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
    }
}

void render_stats_panel(
    const UshaderGolfStats& stats,
    std::size_t golfed_byte_size,
    const UshaderBudgetResult& budget,
    int preset_index)
{
    ImGui::Text("%zu -> %zu chars (%.1f%% reduction)",
        static_cast<std::size_t>(stats.input_chars), static_cast<std::size_t>(stats.output_chars), stats.reduction_pct);
    ImGui::Text("%zu bytes golfed", golfed_byte_size);
    ImGui::Text("Renamed: %zu   Numbers shortened: %zu",
        static_cast<std::size_t>(stats.renamed_count), static_cast<std::size_t>(stats.numbers_shortened));

    if (ImGui::CollapsingHeader("PER-PASS COUNTERS"))
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::Text("Dead locals: %zu", static_cast<std::size_t>(stats.dead_locals_removed));
        ImGui::Text("Dead stores: %zu", static_cast<std::size_t>(stats.dead_stores_removed));
        ImGui::Text("Constants folded: %zu", static_cast<std::size_t>(stats.constants_folded));
        ImGui::Text("Constant vectors: %zu", static_cast<std::size_t>(stats.constant_vectors_reduced));
        ImGui::Text("Trailing returns: %zu", static_cast<std::size_t>(stats.trailing_void_returns_removed));
        ImGui::Text("Compound assigns: %zu", static_cast<std::size_t>(stats.compound_assignments));
        ImGui::Text("Inc/dec: %zu", static_cast<std::size_t>(stats.increments_decrements));
        ImGui::Text("Algebraic identities: %zu", static_cast<std::size_t>(stats.algebraic_identities_simplified));
        ImGui::NextColumn();
        ImGui::Text("Ternaries: %zu", static_cast<std::size_t>(stats.ternaries_from_if_else));
        ImGui::Text("Declarations merged: %zu", static_cast<std::size_t>(stats.declarations_merged));
        ImGui::Text("Braces removed: %zu", static_cast<std::size_t>(stats.braces_removed));
        ImGui::Text("Parens removed: %zu", static_cast<std::size_t>(stats.redundant_parens_removed));
        ImGui::Text("Duplicate precision: %zu", static_cast<std::size_t>(stats.duplicate_precision_removed));
        ImGui::Text("Dead functions: %zu", static_cast<std::size_t>(stats.dead_functions_removed));
        ImGui::Text("Functions inlined: %zu", static_cast<std::size_t>(stats.functions_inlined));
        ImGui::Text("Common subexpressions: %zu", static_cast<std::size_t>(stats.common_subexpressions_eliminated));
        ImGui::Columns(1);
    }

    std::size_t preset_count = 0;
    const BudgetPreset* presets = budget_presets(preset_count);
    const BudgetPreset* preset =
        (preset_index >= 0 && static_cast<std::size_t>(preset_index) < preset_count)
            ? &presets[preset_index]
            : nullptr;

    ImGui::Text("Compressed estimate: %zu bytes (deflate, fixed Huffman)",
        static_cast<std::size_t>(budget.deflate_bytes));

    if (preset != nullptr)
    {
        ImGui::TextUnformatted("Budget:");
        ImGui::SameLine();
        render_budget_badge(preset->name, preset->raw_limit, static_cast<std::size_t>(budget.raw_bytes));
        ImGui::SameLine();
        render_budget_badge(preset->name, preset->deflate_limit, static_cast<std::size_t>(budget.deflate_bytes));
    }
}
