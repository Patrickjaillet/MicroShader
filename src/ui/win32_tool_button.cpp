#include "win32_tool_button.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"

bool Win32ToolButton::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &text_format)))
    {
        return false;
    }
    text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    return SUCCEEDED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush));
}

void Win32ToolButton::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32ToolButton::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
}

bool Win32ToolButton::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

void Win32ToolButton::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes, const std::wstring& label, bool active) const
{
    if (dynamic_brush == nullptr || text_format == nullptr)
    {
        return;
    }

    D2D1_RECT_F rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(rect, 3.0f, 3.0f);

    dynamic_brush->SetColor(active
        ? D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z, 1.0f)
        : D2D1::ColorF(tokens::bg_panel_raised.x, tokens::bg_panel_raised.y, tokens::bg_panel_raised.z, 1.0f));
    render_target->FillRoundedRectangle(rounded, dynamic_brush);

    dynamic_brush->SetColor(D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
    render_target->DrawRoundedRectangle(rounded, dynamic_brush, 1.0f);

    dynamic_brush->SetColor(active
        ? D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)
        : D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
    render_target->DrawText(label.c_str(), static_cast<UINT32>(label.size()), text_format, rect, dynamic_brush);
}
