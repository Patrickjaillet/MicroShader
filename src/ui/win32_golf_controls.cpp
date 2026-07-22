#include "win32_golf_controls.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include <array>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "budget_presets.h"
#include "../platform/utf8.h"
#include "../platform/accessibility_core.h"

namespace
{
    struct CheckboxEntry
    {
        bool* value;
        const wchar_t* label;
    };

    std::array<CheckboxEntry, 16> checkbox_entries(GolfPassToggles& t)
    {
        return {{
            { &t.eliminate_dead_locals, L"Dead locals" },
            { &t.eliminate_dead_stores, L"Dead stores" },
            { &t.fold_constants, L"Fold constants" },
            { &t.reduce_constant_vectors, L"Constant vectors" },
            { &t.strip_trailing_void_return, L"Trailing return" },
            { &t.compound_assignments, L"Compound assign" },
            { &t.increment_decrement, L"Increment/decrement" },
            { &t.simplify_algebraic_identities, L"Algebraic identities" },
            { &t.ternary_from_if_else, L"Ternary" },
            { &t.merge_declarations, L"Merge declarations" },
            { &t.strip_redundant_braces, L"Redundant braces" },
            { &t.strip_redundant_parens, L"Redundant parens" },
            { &t.strip_duplicate_precision, L"Duplicate precision" },
            { &t.eliminate_dead_functions, L"Dead functions" },
            { &t.inline_single_call_functions, L"Inline single-call" },
            { &t.eliminate_common_subexpressions, L"Common subexpressions" },
        }};
    }
}

bool Win32GolfControls::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
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

void Win32GolfControls::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32GolfControls::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
}

bool Win32GolfControls::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

RECT Win32GolfControls::aggressive_rect() const
{
    LONG top = origin_y + 12;
    return RECT{ origin_x + 8, top, origin_x + 8 + static_cast<LONG>(kCheckboxSize), top + static_cast<LONG>(kCheckboxSize) };
}

RECT Win32GolfControls::checkbox_hit_rect(int index, bool second_column) const
{
    LONG col_x = origin_x + (second_column ? width_px / 2 : 8);
    LONG row_top = origin_y + 12 + static_cast<LONG>(kRowHeight) + 24 + static_cast<LONG>(index) * static_cast<LONG>(kRowHeight);
    return RECT{ col_x, row_top, col_x + static_cast<LONG>(kCheckboxSize), row_top + static_cast<LONG>(kCheckboxSize) };
}

RECT Win32GolfControls::field_rect() const
{
    LONG top = origin_y + 12 + static_cast<LONG>(kRowHeight) + 24 + 8 * static_cast<LONG>(kRowHeight) + 16;
    return RECT{ origin_x + 8, top, origin_x + 8 + 320, top + static_cast<LONG>(kRowHeight) };
}

RECT Win32GolfControls::preset_rect() const
{
    RECT field = field_rect();
    LONG top = field.bottom + 12;
    return RECT{ origin_x + 8, top, origin_x + 8 + 240, top + static_cast<LONG>(kRowHeight) };
}

bool Win32GolfControls::on_mouse_down(int client_x, int client_y)
{
    POINT pt{ client_x, client_y };

    RECT agg = aggressive_rect();
    RECT agg_hit{ agg.left - 4, agg.top - 4, agg.left + 160, agg.bottom + 4 };
    if (PtInRect(&agg_hit, pt))
    {
        golf_toggles.aggressive = !golf_toggles.aggressive;
        return true;
    }

    std::array<CheckboxEntry, 16> entries = checkbox_entries(golf_toggles);
    for (int i = 0; golf_toggles.aggressive && i < 8; ++i)
    {
        RECT box = checkbox_hit_rect(i, false);
        RECT hit{ box.left - 4, box.top - 4, box.left + 170, box.bottom + 4 };
        if (PtInRect(&hit, pt))
        {
            *entries[static_cast<size_t>(i)].value = !*entries[static_cast<size_t>(i)].value;
            return true;
        }
        RECT box2 = checkbox_hit_rect(i, true);
        RECT hit2{ box2.left - 4, box2.top - 4, box2.left + 190, box2.bottom + 4 };
        if (PtInRect(&hit2, pt))
        {
            *entries[static_cast<size_t>(i + 8)].value = !*entries[static_cast<size_t>(i + 8)].value;
            return true;
        }
    }

    RECT field = field_rect();
    if (PtInRect(&field, pt))
    {
        field_focused = true;
        return true;
    }
    field_focused = false;

    RECT preset = preset_rect();
    if (PtInRect(&preset, pt))
    {
        std::size_t preset_count = 0;
        budget_presets(preset_count);
        budget_index = (budget_index + 1 >= static_cast<int>(preset_count)) ? -1 : budget_index + 1;
        return true;
    }

    return false;
}

bool Win32GolfControls::on_char(wchar_t character)
{
    if (!field_focused)
    {
        return false;
    }
    if (character == L'\b' || character < 32)
    {
        return true;
    }
    protected_names_text.push_back(character < 128 ? static_cast<char>(character) : '?');
    return true;
}

bool Win32GolfControls::on_key_down(WPARAM key)
{
    if (!field_focused)
    {
        return false;
    }
    if (key == VK_BACK && !protected_names_text.empty())
    {
        protected_names_text.pop_back();
        return true;
    }
    return field_focused;
}

void Win32GolfControls::set_field_focus(bool focused)
{
    field_focused = focused;
}

void Win32GolfControls::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_app);

    if (dynamic_brush == nullptr || text_format == nullptr)
    {
        return;
    }

    auto draw_checkbox = [&](RECT box, bool checked, const wchar_t* label, float alpha)
    {
        D2D1_RECT_F box_rect = D2D1::RectF(static_cast<float>(box.left), static_cast<float>(box.top),
            static_cast<float>(box.right), static_cast<float>(box.bottom));
        dynamic_brush->SetColor(checked
            ? D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z, alpha)
            : D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z, alpha));
        render_target->DrawRectangle(box_rect, dynamic_brush, 1.0f);
        if (checked)
        {
            dynamic_brush->SetColor(D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z, alpha));
            D2D1_RECT_F fill_rect = D2D1::RectF(box_rect.left + 3.0f, box_rect.top + 3.0f, box_rect.right - 3.0f, box_rect.bottom - 3.0f);
            render_target->FillRectangle(fill_rect, dynamic_brush);
        }

        D2D1_RECT_F label_rect = D2D1::RectF(box_rect.right + 6.0f, box_rect.top - 5.0f, box_rect.right + 200.0f, box_rect.bottom + 5.0f);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z, alpha));
        render_target->DrawText(label, static_cast<UINT32>(wcslen(label)), text_format, label_rect, dynamic_brush);

        accessibility_register_toggle(wide_to_utf8(label).c_str(), AccessibleRole::CheckBox,
            static_cast<float>(box.left), static_cast<float>(box.top),
            static_cast<float>(box.right - box.left), static_cast<float>(box.bottom - box.top),
            alpha >= 1.0f, checked);
    };

    draw_checkbox(aggressive_rect(), golf_toggles.aggressive, L"Aggressive golf", 1.0f);

    D2D1_RECT_F header_rect = D2D1::RectF(static_cast<float>(origin_x) + 8.0f, static_cast<float>(origin_y) + 12.0f + kRowHeight,
        static_cast<float>(origin_x) + 300.0f, static_cast<float>(origin_y) + 12.0f + kRowHeight + 20.0f);
    dynamic_brush->SetColor(D2D1::ColorF(tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z));
    render_target->DrawText(L"PASSES", 6, text_format, header_rect, dynamic_brush);

    std::array<CheckboxEntry, 16> entries = checkbox_entries(const_cast<GolfPassToggles&>(golf_toggles));
    float disabled_alpha = golf_toggles.aggressive ? 1.0f : 0.4f;
    for (int i = 0; i < 8; ++i)
    {
        draw_checkbox(checkbox_hit_rect(i, false), *entries[static_cast<size_t>(i)].value, entries[static_cast<size_t>(i)].label, disabled_alpha);
        draw_checkbox(checkbox_hit_rect(i, true), *entries[static_cast<size_t>(i + 8)].value, entries[static_cast<size_t>(i + 8)].label, disabled_alpha);
    }

    RECT field = field_rect();
    D2D1_RECT_F field_bg = D2D1::RectF(static_cast<float>(field.left), static_cast<float>(field.top),
        static_cast<float>(field.right), static_cast<float>(field.bottom));
    render_target->FillRectangle(field_bg, brushes.bg_panel_raised);
    dynamic_brush->SetColor(field_focused
        ? D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z)
        : D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
    render_target->DrawRectangle(field_bg, dynamic_brush, 1.0f);

    std::wstring field_text = protected_names_text.empty()
        ? std::wstring(L"Protected names (comma-separated)...")
        : utf8_to_wide(protected_names_text);
    D2D1_RECT_F field_text_rect = D2D1::RectF(field_bg.left + 6.0f, field_bg.top, field_bg.right - 6.0f, field_bg.bottom);
    dynamic_brush->SetColor(protected_names_text.empty()
        ? D2D1::ColorF(tokens::text_disabled.x, tokens::text_disabled.y, tokens::text_disabled.z)
        : D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
    render_target->DrawText(field_text.c_str(), static_cast<UINT32>(field_text.size()), text_format, field_text_rect, dynamic_brush);

    RECT preset = preset_rect();
    std::size_t preset_count = 0;
    const BudgetPreset* presets = budget_presets(preset_count);
    std::wstring preset_label = L"Budget preset: ";
    preset_label += (budget_index >= 0 && static_cast<std::size_t>(budget_index) < preset_count)
        ? utf8_to_wide(presets[static_cast<size_t>(budget_index)].name)
        : L"Custom";
    D2D1_RECT_F preset_rect_f = D2D1::RectF(static_cast<float>(preset.left), static_cast<float>(preset.top),
        static_cast<float>(preset.right), static_cast<float>(preset.bottom));
    dynamic_brush->SetColor(D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z));
    render_target->DrawText(preset_label.c_str(), static_cast<UINT32>(preset_label.size()), text_format, preset_rect_f, dynamic_brush);

    accessibility_register(wide_to_utf8(preset_label.c_str()).c_str(), AccessibleRole::Button,
        static_cast<float>(preset.left), static_cast<float>(preset.top),
        static_cast<float>(preset.right - preset.left), static_cast<float>(preset.bottom - preset.top), true);
}
