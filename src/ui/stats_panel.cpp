#include "stats_panel.h"

#include <imgui.h>

#include "theme_tokens.h"

namespace
{
    void render_badge(const char* label, std::size_t limit, std::size_t actual)
    {
        bool fits = actual <= limit;
        ImVec4 color = fits ? tokens::status_ok : tokens::text_disabled;
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s %s", fits ? "\xe2\x9c\x93" : "\xc2\xb7", label);
        ImGui::PopStyleColor();
    }
}

void render_stats_panel(const UshaderGolfStats& stats, std::size_t golfed_byte_size)
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
        ImGui::NextColumn();
        ImGui::Text("Ternaries: %zu", static_cast<std::size_t>(stats.ternaries_from_if_else));
        ImGui::Text("Declarations merged: %zu", static_cast<std::size_t>(stats.declarations_merged));
        ImGui::Text("Braces removed: %zu", static_cast<std::size_t>(stats.braces_removed));
        ImGui::Text("Parens removed: %zu", static_cast<std::size_t>(stats.redundant_parens_removed));
        ImGui::Text("Duplicate precision: %zu", static_cast<std::size_t>(stats.duplicate_precision_removed));
        ImGui::Text("Dead functions: %zu", static_cast<std::size_t>(stats.dead_functions_removed));
        ImGui::Text("Functions inlined: %zu", static_cast<std::size_t>(stats.functions_inlined));
        ImGui::Columns(1);
    }

    ImGui::TextUnformatted("Size budgets:");
    ImGui::SameLine();
    render_badge("280", 280, golfed_byte_size);
    ImGui::SameLine();
    render_badge("512", 512, golfed_byte_size);
    ImGui::SameLine();
    render_badge("1024", 1024, golfed_byte_size);
}
