#include "win32_diff_view.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "../platform/utf8.h"

bool Win32DiffView::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    HRESULT hr = dwrite_factory->CreateTextFormat(
        L"Consolas", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &text_format);
    if (FAILED(hr))
    {
        return false;
    }
    text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    IDWriteTextLayout* probe = nullptr;
    if (SUCCEEDED(dwrite_factory->CreateTextLayout(L"M", 1, text_format, 1000.0f, 1000.0f, &probe)))
    {
        DWRITE_TEXT_METRICS metrics{};
        probe->GetMetrics(&metrics);
        if (metrics.width > 0.0f) { char_width = metrics.width; }
        if (metrics.height > 0.0f) { line_height = metrics.height; }
        probe->Release();
    }

    return SUCCEEDED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush));
}

void Win32DiffView::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32DiffView::set_diff(const std::vector<DiffSpan>& new_spans)
{
    spans = new_spans;
    scroll_top_row = 0;
    rebuild_layout();
}

void Win32DiffView::layout(int x, int y, int width, int height)
{
    bool width_changed = width != width_px;
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
    if (width_changed)
    {
        rebuild_layout();
    }
}

bool Win32DiffView::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

void Win32DiffView::rebuild_layout()
{
    draw_spans.clear();
    row_count = 1;
    if (width_px <= 0)
    {
        return;
    }

    float left_margin = 4.0f;
    float right_edge = static_cast<float>(width_px) - 4.0f;
    float x = left_margin;
    int row = 0;

    for (const DiffSpan& span : spans)
    {
        if (span.text.empty())
        {
            continue;
        }
        std::wstring wide = utf8_to_wide(span.text);
        if (wide == L"\n")
        {
            row++;
            x = left_margin;
            continue;
        }

        float span_width = static_cast<float>(wide.size()) * char_width;
        if (x + span_width > right_edge && x > left_margin)
        {
            row++;
            x = left_margin;
        }

        DrawSpan ds;
        ds.x = x;
        ds.row = static_cast<float>(row);
        ds.text = wide;
        ds.kind = span.kind;
        draw_spans.push_back(ds);
        x += span_width;
    }
    row_count = row + 1;
}

void Win32DiffView::on_mouse_wheel(int wheel_delta)
{
    int rows_to_scroll = -(wheel_delta / WHEEL_DELTA) * 3;
    scroll_top_row += rows_to_scroll;
    if (scroll_top_row < 0) { scroll_top_row = 0; }
    int max_top = std::max(0, row_count - 1);
    if (scroll_top_row > max_top) { scroll_top_row = max_top; }
}

void Win32DiffView::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_panel);

    if (dynamic_brush == nullptr || text_format == nullptr)
    {
        return;
    }

    render_target->PushAxisAlignedClip(bg_rect, D2D1_ANTIALIAS_MODE_ALIASED);

    for (const DrawSpan& ds : draw_spans)
    {
        float y = static_cast<float>(origin_y) + (ds.row - static_cast<float>(scroll_top_row)) * line_height;
        if (y + line_height < static_cast<float>(origin_y))
        {
            continue;
        }
        if (y > static_cast<float>(origin_y + height_px))
        {
            break;
        }

        D2D1_COLOR_F color = D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z);
        if (ds.kind == DiffSpanKind::Removed)
        {
            color = D2D1::ColorF(tokens::status_error.x, tokens::status_error.y, tokens::status_error.z);
        }
        else if (ds.kind == DiffSpanKind::Added)
        {
            color = D2D1::ColorF(tokens::status_ok.x, tokens::status_ok.y, tokens::status_ok.z);
        }

        float x = static_cast<float>(origin_x) + ds.x;
        float text_width = static_cast<float>(ds.text.size()) * char_width;
        D2D1_RECT_F text_rect = D2D1::RectF(x, y, x + text_width + 4.0f, y + line_height);
        dynamic_brush->SetColor(color);
        render_target->DrawText(ds.text.c_str(), static_cast<UINT32>(ds.text.size()), text_format, text_rect, dynamic_brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

        if (ds.kind == DiffSpanKind::Removed)
        {
            float strike_y = y + line_height * 0.55f;
            render_target->DrawLine(D2D1::Point2F(x, strike_y), D2D1::Point2F(x + text_width, strike_y), dynamic_brush, 1.0f);
        }
    }

    render_target->PopAxisAlignedClip();
}
