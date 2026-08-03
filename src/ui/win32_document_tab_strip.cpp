#include "win32_document_tab_strip.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "../platform/utf8.h"
#include "../platform/accessibility_core.h"

bool Win32DocumentTabStrip::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(12.0f), L"en-us", &text_format)))
    {
        return false;
    }
    text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    return SUCCEEDED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush));
}

void Win32DocumentTabStrip::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32DocumentTabStrip::layout(int x, int y, int width)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
}

void Win32DocumentTabStrip::set_documents(const std::vector<std::string>& display_names, int active_index, int dirty_mask_value)
{
    names = display_names;
    active = active_index;
    dirty_mask = dirty_mask_value;
}

float Win32DocumentTabStrip::tab_width() const
{
    if (names.empty())
    {
        return kTabMinWidth;
    }
    float available = static_cast<float>(width_px) - kNewButtonWidth;
    float even_share = available / static_cast<float>(names.size());
    return std::max(kTabMinWidth, std::min(kTabMaxWidth, even_share));
}

RECT Win32DocumentTabStrip::tab_rect(int index) const
{
    float w = tab_width();
    LONG left = origin_x + static_cast<LONG>(w * static_cast<float>(index));
    return RECT{ left, origin_y, left + static_cast<LONG>(w), origin_y + static_cast<LONG>(kHeight) };
}

RECT Win32DocumentTabStrip::close_rect(int index) const
{
    RECT tab = tab_rect(index);
    return RECT{ tab.right - static_cast<LONG>(kCloseWidth) - 6, tab.top + 6, tab.right - 6, tab.bottom - 6 };
}

RECT Win32DocumentTabStrip::new_button_rect() const
{
    float w = tab_width();
    LONG left = origin_x + static_cast<LONG>(w * static_cast<float>(names.size()));
    return RECT{ left + 4, origin_y + 4, left + static_cast<LONG>(kNewButtonWidth) - 4, origin_y + static_cast<LONG>(kHeight) - 4 };
}

Win32DocumentTabStrip::HitResult Win32DocumentTabStrip::hit_test(int x, int y) const
{
    POINT pt{ x, y };
    if (y < origin_y || y >= origin_y + static_cast<int>(kHeight))
    {
        return HitResult{};
    }

    RECT new_button = new_button_rect();
    if (PtInRect(&new_button, pt))
    {
        return HitResult{ HitKind::NewDocument, -1 };
    }

    for (int i = 0; i < static_cast<int>(names.size()); ++i)
    {
        RECT close = close_rect(i);
        if (names.size() > 1 && PtInRect(&close, pt))
        {
            return HitResult{ HitKind::Close, i };
        }
        RECT tab = tab_rect(i);
        if (PtInRect(&tab, pt))
        {
            return HitResult{ HitKind::Document, i };
        }
    }
    return HitResult{};
}

void Win32DocumentTabStrip::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F strip_bg = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y) + kHeight);
    render_target->FillRectangle(strip_bg, brushes.bg_app);

    if (dynamic_brush == nullptr || text_format == nullptr)
    {
        return;
    }

    for (int i = 0; i < static_cast<int>(names.size()); ++i)
    {
        RECT tab = tab_rect(i);
        D2D1_RECT_F tab_f = D2D1::RectF(static_cast<float>(tab.left) + 2.0f, static_cast<float>(tab.top) + 3.0f,
            static_cast<float>(tab.right) - 2.0f, static_cast<float>(tab.bottom) - 1.0f);
        bool is_active = i == active;
        D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(tab_f, 3.0f, 3.0f);
        render_target->FillRoundedRectangle(rounded, is_active ? brushes.bg_panel_raised : brushes.bg_panel);
        if (is_active)
        {
            render_target->DrawRoundedRectangle(rounded, brushes.accent, 1.0f);
        }

        bool dirty = (dirty_mask & (1 << i)) != 0;
        std::string label = (dirty ? std::string("* ") : std::string()) + (i < static_cast<int>(names.size()) ? names[static_cast<size_t>(i)] : std::string());
        std::wstring wide_label = utf8_to_wide(label);
        D2D1_RECT_F label_rect = D2D1::RectF(tab_f.left + 8.0f, tab_f.top,
            tab_f.right - kCloseWidth - 4.0f, tab_f.bottom);
        render_target->DrawText(wide_label.c_str(), static_cast<UINT32>(wide_label.size()), text_format,
            label_rect, is_active ? brushes.text_primary : brushes.text_secondary);

        if (names.size() > 1)
        {
            RECT close = close_rect(i);
            D2D1_RECT_F close_f = D2D1::RectF(static_cast<float>(close.left), static_cast<float>(close.top),
                static_cast<float>(close.right), static_cast<float>(close.bottom));
            dynamic_brush->SetColor(D2D1::ColorF(tokens::text_disabled.x, tokens::text_disabled.y, tokens::text_disabled.z));
            render_target->DrawText(L"x", 1, text_format, close_f, dynamic_brush);
        }

        std::string a11y_name = "Document: " + label + (is_active ? " (active)" : "");
        accessibility_register(a11y_name.c_str(), AccessibleRole::Button,
            tab_f.left, tab_f.top, tab_f.right - tab_f.left, tab_f.bottom - tab_f.top, true);
    }

    RECT new_button = new_button_rect();
    D2D1_RECT_F new_f = D2D1::RectF(static_cast<float>(new_button.left), static_cast<float>(new_button.top),
        static_cast<float>(new_button.right), static_cast<float>(new_button.bottom));
    D2D1_ROUNDED_RECT new_rounded = D2D1::RoundedRect(new_f, 3.0f, 3.0f);
    dynamic_brush->SetColor(D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
    render_target->DrawRoundedRectangle(new_rounded, dynamic_brush, 1.0f);
    dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
    render_target->DrawText(L"+", 1, text_format, new_f, dynamic_brush);

    accessibility_register("New document", AccessibleRole::Button,
        new_f.left, new_f.top, new_f.right - new_f.left, new_f.bottom - new_f.top, true);
}
