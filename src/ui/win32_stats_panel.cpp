#include "win32_stats_panel.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include <cstdio>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "budget_presets.h"
#include "../platform/utf8.h"

namespace
{
    struct CounterEntry
    {
        const char* label;
        uintptr_t value;
    };
}

bool Win32StatsPanel::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &text_format)))
    {
        return false;
    }
    text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    return SUCCEEDED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush));
}

void Win32StatsPanel::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32StatsPanel::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
}

void Win32StatsPanel::set_stats(const UshaderGolfStats& stats, std::size_t golfed_byte_size,
    const UshaderBudgetResult& budget, int budget_preset_index, bool has_stats_data)
{
    last_stats = stats;
    last_golfed_bytes = golfed_byte_size;
    last_budget = budget;
    last_preset_index = budget_preset_index;
    has_data = has_stats_data;
}

void Win32StatsPanel::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_app);

    if (dynamic_brush == nullptr || text_format == nullptr)
    {
        return;
    }

    float y = static_cast<float>(origin_y) + 12.0f;
    const float row_height = 22.0f;
    const float left = static_cast<float>(origin_x) + 8.0f;
    const float right = static_cast<float>(origin_x + width_px) - 8.0f;

    auto draw_line = [&](const std::string& text, ID2D1SolidColorBrush* color_brush)
    {
        std::wstring wide = utf8_to_wide(text);
        D2D1_RECT_F rect = D2D1::RectF(left, y, right, y + row_height);
        render_target->DrawText(wide.c_str(), static_cast<UINT32>(wide.size()), text_format, rect, color_brush);
        y += row_height;
    };

    if (!has_data)
    {
        draw_line("Run golf to see stats.", brushes.text_secondary);
        return;
    }

    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "%zu -> %zu chars (%.1f%% reduction)",
        static_cast<size_t>(last_stats.input_chars), static_cast<size_t>(last_stats.output_chars), last_stats.reduction_pct);
    draw_line(buffer, brushes.text_primary);

    std::snprintf(buffer, sizeof(buffer), "%zu bytes golfed", last_golfed_bytes);
    draw_line(buffer, brushes.text_primary);

    std::snprintf(buffer, sizeof(buffer), "Renamed: %zu   Numbers shortened: %zu",
        static_cast<size_t>(last_stats.renamed_count), static_cast<size_t>(last_stats.numbers_shortened));
    draw_line(buffer, brushes.text_primary);

    y += 8.0f;
    draw_line("PER-PASS COUNTERS", brushes.text_secondary);
    y += 4.0f;

    CounterEntry column1[] = {
        { "Dead locals", last_stats.dead_locals_removed },
        { "Dead stores", last_stats.dead_stores_removed },
        { "Constants folded", last_stats.constants_folded },
        { "Constant vectors", last_stats.constant_vectors_reduced },
        { "Trailing returns", last_stats.trailing_void_returns_removed },
        { "Compound assigns", last_stats.compound_assignments },
        { "Inc/dec", last_stats.increments_decrements },
        { "Algebraic identities", last_stats.algebraic_identities_simplified },
    };
    CounterEntry column2[] = {
        { "Ternaries", last_stats.ternaries_from_if_else },
        { "Declarations merged", last_stats.declarations_merged },
        { "Braces removed", last_stats.braces_removed },
        { "Parens removed", last_stats.redundant_parens_removed },
        { "Duplicate precision", last_stats.duplicate_precision_removed },
        { "Dead functions", last_stats.dead_functions_removed },
        { "Functions inlined", last_stats.functions_inlined },
        { "Common subexpressions", last_stats.common_subexpressions_eliminated },
    };

    float column_top = y;
    float column2_x = static_cast<float>(origin_x) + static_cast<float>(width_px) * 0.5f;
    for (const CounterEntry& entry : column1)
    {
        std::snprintf(buffer, sizeof(buffer), "%s: %zu", entry.label, static_cast<size_t>(entry.value));
        std::wstring wide = utf8_to_wide(buffer);
        D2D1_RECT_F rect = D2D1::RectF(left, y, column2_x - 8.0f, y + row_height);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
        render_target->DrawText(wide.c_str(), static_cast<UINT32>(wide.size()), text_format, rect, dynamic_brush);
        y += row_height;
    }
    y = column_top;
    for (const CounterEntry& entry : column2)
    {
        std::snprintf(buffer, sizeof(buffer), "%s: %zu", entry.label, static_cast<size_t>(entry.value));
        std::wstring wide = utf8_to_wide(buffer);
        D2D1_RECT_F rect = D2D1::RectF(column2_x, y, right, y + row_height);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
        render_target->DrawText(wide.c_str(), static_cast<UINT32>(wide.size()), text_format, rect, dynamic_brush);
        y += row_height;
    }

    y = column_top + 8.0f * row_height + 12.0f;

    std::snprintf(buffer, sizeof(buffer), "Compressed estimate: %zu bytes (deflate, fixed Huffman)",
        static_cast<size_t>(last_budget.deflate_bytes));
    D2D1_RECT_F est_rect = D2D1::RectF(left, y, right, y + row_height);
    dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
    std::wstring est_wide = utf8_to_wide(buffer);
    render_target->DrawText(est_wide.c_str(), static_cast<UINT32>(est_wide.size()), text_format, est_rect, dynamic_brush);
    y += row_height;

    std::size_t preset_count = 0;
    const BudgetPreset* presets = budget_presets(preset_count);
    if (last_preset_index >= 0 && static_cast<std::size_t>(last_preset_index) < preset_count)
    {
        const BudgetPreset& preset = presets[static_cast<size_t>(last_preset_index)];

        auto draw_badge = [&](long long limit, std::size_t actual)
        {
            if (limit < 0)
            {
                return;
            }
            auto limit_u = static_cast<std::size_t>(limit);
            bool over = actual > limit_u;
            bool near_limit = !over && limit_u > 0 && actual * 10 >= limit_u * 9;
            if (over)
            {
                dynamic_brush->SetColor(D2D1::ColorF(tokens::status_error.x, tokens::status_error.y, tokens::status_error.z));
            }
            else if (near_limit)
            {
                dynamic_brush->SetColor(D2D1::ColorF(tokens::status_warning.x, tokens::status_warning.y, tokens::status_warning.z));
            }
            else
            {
                dynamic_brush->SetColor(D2D1::ColorF(tokens::status_ok.x, tokens::status_ok.y, tokens::status_ok.z));
            }
            char badge_text[96];
            std::snprintf(badge_text, sizeof(badge_text), "%s %s: %zu / %lld",
                over ? "X" : "OK", preset.name, actual, limit);
            std::wstring wide = utf8_to_wide(badge_text);
            D2D1_RECT_F rect = D2D1::RectF(left, y, right, y + row_height);
            render_target->DrawText(wide.c_str(), static_cast<UINT32>(wide.size()), text_format, rect, dynamic_brush);
            y += row_height;
        };

        draw_badge(preset.raw_limit, static_cast<std::size_t>(last_budget.raw_bytes));
        draw_badge(preset.deflate_limit, static_cast<std::size_t>(last_budget.deflate_bytes));
    }
}
