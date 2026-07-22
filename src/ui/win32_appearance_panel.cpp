#include "win32_appearance_panel.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "../platform/accessibility_core.h"

bool Win32AppearancePanel::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
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

void Win32AppearancePanel::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32AppearancePanel::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
}

bool Win32AppearancePanel::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

RECT Win32AppearancePanel::slider_rect() const
{
    LONG top = origin_y + 44;
    return RECT{ origin_x + 8, top, origin_x + 8 + static_cast<LONG>(kSliderWidth), top + static_cast<LONG>(kRowHeight) };
}

RECT Win32AppearancePanel::checkbox_rect() const
{
    RECT slider = slider_rect();
    LONG top = slider.bottom + 32;
    return RECT{ origin_x + 8, top, origin_x + 8 + static_cast<LONG>(kCheckboxSize), top + static_cast<LONG>(kCheckboxSize) };
}

void Win32AppearancePanel::set_font_size_from_x(int client_x)
{
    RECT slider = slider_rect();
    float t = static_cast<float>(client_x - slider.left) / static_cast<float>(slider.right - slider.left);
    t = std::clamp(t, 0.0f, 1.0f);
    float value = kMinUiFontSize + t * (kMaxUiFontSize - kMinUiFontSize);
    value = std::round(value);
    if (value != g_ui_font_size)
    {
        g_ui_font_size = value;
        pending_font_change = true;
    }
}

bool Win32AppearancePanel::on_mouse_down(int client_x, int client_y)
{
    POINT pt{ client_x, client_y };

    RECT slider = slider_rect();
    RECT slider_hit{ slider.left - 8, slider.top - 8, slider.right + 8, slider.bottom + 8 };
    if (PtInRect(&slider_hit, pt))
    {
        dragging_slider = true;
        set_font_size_from_x(client_x);
        return true;
    }

    RECT reset_hit{ slider.right + 16, slider.top, slider.right + 130, slider.bottom };
    if (PtInRect(&reset_hit, pt))
    {
        if (g_ui_font_size != kDefaultUiFontSize)
        {
            g_ui_font_size = kDefaultUiFontSize;
            pending_font_change = true;
        }
        return true;
    }

    RECT box = checkbox_rect();
    RECT box_hit{ box.left - 4, box.top - 4, box.left + 260, box.bottom + 4 };
    if (PtInRect(&box_hit, pt))
    {
        g_colorblind_safe_indicators = !g_colorblind_safe_indicators;
        return true;
    }

    return false;
}

void Win32AppearancePanel::on_mouse_move(int client_x, int)
{
    if (dragging_slider)
    {
        set_font_size_from_x(client_x);
    }
}

bool Win32AppearancePanel::on_mouse_up()
{
    bool was_dragging = dragging_slider;
    dragging_slider = false;
    return was_dragging;
}

bool Win32AppearancePanel::font_size_changed_and_clear()
{
    bool changed = pending_font_change;
    pending_font_change = false;
    return changed;
}

void Win32AppearancePanel::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_app);

    if (dynamic_brush == nullptr || text_format == nullptr)
    {
        return;
    }

    D2D1_RECT_F label_rect = D2D1::RectF(static_cast<float>(origin_x) + 8.0f, static_cast<float>(origin_y) + 12.0f,
        static_cast<float>(origin_x) + 400.0f, static_cast<float>(origin_y) + 32.0f);
    char label_buffer[64];
    std::snprintf(label_buffer, sizeof(label_buffer), "UI text size: %d pt", static_cast<int>(g_ui_font_size));
    wchar_t wide_label[64];
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, label_buffer, -1, wide_label, 64);
    dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
    render_target->DrawText(wide_label, static_cast<UINT32>(wide_len > 0 ? wide_len - 1 : 0), text_format, label_rect, dynamic_brush);

    RECT slider = slider_rect();
    D2D1_RECT_F track_rect = D2D1::RectF(static_cast<float>(slider.left), static_cast<float>(slider.top) + kRowHeight * 0.5f - 2.0f,
        static_cast<float>(slider.right), static_cast<float>(slider.top) + kRowHeight * 0.5f + 2.0f);
    render_target->FillRectangle(track_rect, brushes.bg_panel_raised);
    dynamic_brush->SetColor(D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
    render_target->DrawRectangle(track_rect, dynamic_brush, 1.0f);

    float t = (g_ui_font_size - kMinUiFontSize) / (kMaxUiFontSize - kMinUiFontSize);
    float thumb_x = static_cast<float>(slider.left) + t * static_cast<float>(slider.right - slider.left);
    D2D1_ELLIPSE thumb = D2D1::Ellipse(D2D1::Point2F(thumb_x, static_cast<float>(slider.top) + kRowHeight * 0.5f), 7.0f, 7.0f);
    dynamic_brush->SetColor(D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z));
    render_target->FillEllipse(thumb, dynamic_brush);

    D2D1_RECT_F reset_rect = D2D1::RectF(static_cast<float>(slider.right) + 16.0f, static_cast<float>(slider.top),
        static_cast<float>(slider.right) + 130.0f, static_cast<float>(slider.bottom));
    dynamic_brush->SetColor(D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z));
    render_target->DrawText(L"Reset to default", 17, text_format, reset_rect, dynamic_brush);

    accessibility_register("UI text size", AccessibleRole::Button,
        static_cast<float>(slider.left), static_cast<float>(slider.top),
        static_cast<float>(slider.right - slider.left), static_cast<float>(slider.bottom - slider.top), true);
    accessibility_register("Reset to default", AccessibleRole::Button,
        reset_rect.left, reset_rect.top, reset_rect.right - reset_rect.left, reset_rect.bottom - reset_rect.top, true);

    RECT box = checkbox_rect();
    D2D1_RECT_F box_rect = D2D1::RectF(static_cast<float>(box.left), static_cast<float>(box.top),
        static_cast<float>(box.right), static_cast<float>(box.bottom));
    dynamic_brush->SetColor(g_colorblind_safe_indicators
        ? D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z)
        : D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
    render_target->DrawRectangle(box_rect, dynamic_brush, 1.0f);
    if (g_colorblind_safe_indicators)
    {
        D2D1_RECT_F fill_rect = D2D1::RectF(box_rect.left + 3.0f, box_rect.top + 3.0f, box_rect.right - 3.0f, box_rect.bottom - 3.0f);
        render_target->FillRectangle(fill_rect, dynamic_brush);
    }
    D2D1_RECT_F box_label_rect = D2D1::RectF(box_rect.right + 6.0f, box_rect.top - 5.0f, box_rect.right + 260.0f, box_rect.bottom + 5.0f);
    dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
    render_target->DrawText(L"Colorblind-safe status indicators", 34, text_format, box_label_rect, dynamic_brush);

    accessibility_register_toggle("Colorblind-safe status indicators", AccessibleRole::CheckBox,
        box_rect.left, box_rect.top, box_rect.right - box_rect.left, box_rect.bottom - box_rect.top,
        true, g_colorblind_safe_indicators);
}
