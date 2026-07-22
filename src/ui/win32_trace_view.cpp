#include "win32_trace_view.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "../platform/utf8.h"

bool Win32TraceView::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    HRESULT hr = dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &text_format);
    if (FAILED(hr))
    {
        return false;
    }
    text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    if (FAILED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush)))
    {
        return false;
    }

    if (!before_editor.create(render_target, dwrite_factory, true))
    {
        return false;
    }
    if (!after_editor.create(render_target, dwrite_factory, true))
    {
        return false;
    }
    return true;
}

void Win32TraceView::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
    before_editor.destroy();
    after_editor.destroy();
}

void Win32TraceView::set_steps(const std::vector<GolfTraceStep>& new_steps)
{
    steps = new_steps;
    expanded_index = -1;
    scroll_y = 0.0f;
}

void Win32TraceView::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
}

bool Win32TraceView::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

std::vector<Win32TraceView::RowLayout> Win32TraceView::compute_rows() const
{
    std::vector<RowLayout> rows;
    float y = static_cast<float>(origin_y) - scroll_y;
    float pane_gap = 8.0f;
    float half_width = (static_cast<float>(width_px) - pane_gap - 8.0f) * 0.5f;

    for (size_t i = 0; i < steps.size(); ++i)
    {
        RowLayout row;
        row.step_index = static_cast<int>(i);
        row.header_top = y;
        row.header_bottom = y + kHeaderHeight;
        y += kHeaderHeight;

        if (expanded_index == static_cast<int>(i))
        {
            row.pane_top = y + kLabelHeight;
            row.before_x = static_cast<float>(origin_x) + 4.0f;
            row.after_x = row.before_x + half_width + pane_gap;
            row.pane_width = half_width;
            y += kLabelHeight + kPaneHeight + pane_gap;
        }

        rows.push_back(row);
    }

    return rows;
}

void Win32TraceView::on_mouse_down(int client_x, int client_y)
{
    std::vector<RowLayout> rows = compute_rows();
    for (const RowLayout& row : rows)
    {
        if (expanded_index == row.step_index)
        {
            float pane_bottom = row.pane_top + kPaneHeight;
            if (client_y >= row.pane_top && client_y < pane_bottom)
            {
                if (client_x >= row.before_x && client_x < row.before_x + row.pane_width)
                {
                    before_editor.layout(static_cast<int>(row.before_x), static_cast<int>(row.pane_top), static_cast<int>(row.pane_width), static_cast<int>(kPaneHeight));
                    before_editor.on_mouse_down(client_x, client_y, false);
                    return;
                }
                if (client_x >= row.after_x && client_x < row.after_x + row.pane_width)
                {
                    after_editor.layout(static_cast<int>(row.after_x), static_cast<int>(row.pane_top), static_cast<int>(row.pane_width), static_cast<int>(kPaneHeight));
                    after_editor.on_mouse_down(client_x, client_y, false);
                    return;
                }
            }
        }

        if (client_y >= row.header_top && client_y < row.header_bottom)
        {
            expanded_index = (expanded_index == row.step_index) ? -1 : row.step_index;
            if (expanded_index >= 0)
            {
                before_editor.set_text_utf8(steps[static_cast<size_t>(expanded_index)].before);
                after_editor.set_text_utf8(steps[static_cast<size_t>(expanded_index)].after);
            }
            return;
        }
    }
}

void Win32TraceView::on_mouse_wheel(int wheel_delta)
{
    float rows_to_scroll = -static_cast<float>(wheel_delta / WHEEL_DELTA) * 3.0f * kHeaderHeight;
    scroll_y += rows_to_scroll;
    if (scroll_y < 0.0f) { scroll_y = 0.0f; }
    float max_scroll = std::max(0.0f, total_content_height - static_cast<float>(height_px));
    if (scroll_y > max_scroll) { scroll_y = max_scroll; }
}

void Win32TraceView::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_app);

    if (steps.empty())
    {
        if (text_format != nullptr && dynamic_brush != nullptr)
        {
            dynamic_brush->SetColor(D2D1::ColorF(tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z));
            D2D1_RECT_F msg_rect = D2D1::RectF(bg_rect.left + 8.0f, bg_rect.top, bg_rect.right, bg_rect.top + 24.0f);
            const wchar_t* message = L"Run golf to see the pass-by-pass trace.";
            render_target->DrawText(message, static_cast<UINT32>(wcslen(message)), text_format, msg_rect, dynamic_brush);
        }
        return;
    }

    render_target->PushAxisAlignedClip(bg_rect, D2D1_ANTIALIAS_MODE_ALIASED);

    std::vector<RowLayout> rows = compute_rows();
    for (const RowLayout& row : rows)
    {
        const GolfTraceStep& step = steps[static_cast<size_t>(row.step_index)];
        bool changed = step.count > 0;

        if (row.header_bottom >= static_cast<float>(origin_y) && row.header_top <= static_cast<float>(origin_y + height_px))
        {
            D2D1_RECT_F header_rect = D2D1::RectF(static_cast<float>(origin_x), row.header_top,
                static_cast<float>(origin_x + width_px), row.header_bottom);
            render_target->FillRectangle(header_rect, brushes.bg_panel);

            std::wstring label = utf8_to_wide(step.pass_name) + L" (" + std::to_wstring(step.count)
                + (step.count == 1 ? L" change)" : L" changes)");
            D2D1_RECT_F text_rect = D2D1::RectF(header_rect.left + 8.0f, header_rect.top, header_rect.right - 8.0f, header_rect.bottom);
            dynamic_brush->SetColor(changed
                ? D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z)
                : D2D1::ColorF(tokens::text_disabled.x, tokens::text_disabled.y, tokens::text_disabled.z));
            render_target->DrawText(label.c_str(), static_cast<UINT32>(label.size()), text_format, text_rect, dynamic_brush);
        }

        if (expanded_index == row.step_index)
        {
            dynamic_brush->SetColor(D2D1::ColorF(tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z));
            D2D1_RECT_F before_label = D2D1::RectF(row.before_x, row.pane_top - kLabelHeight, row.before_x + row.pane_width, row.pane_top);
            D2D1_RECT_F after_label = D2D1::RectF(row.after_x, row.pane_top - kLabelHeight, row.after_x + row.pane_width, row.pane_top);
            render_target->DrawText(L"Before", 6, text_format, before_label, dynamic_brush);
            render_target->DrawText(L"After", 5, text_format, after_label, dynamic_brush);

            before_editor.layout(static_cast<int>(row.before_x), static_cast<int>(row.pane_top), static_cast<int>(row.pane_width), static_cast<int>(kPaneHeight));
            after_editor.layout(static_cast<int>(row.after_x), static_cast<int>(row.pane_top), static_cast<int>(row.pane_width), static_cast<int>(kPaneHeight));
            before_editor.paint(render_target, brushes);
            after_editor.paint(render_target, brushes);
        }
    }

    if (!rows.empty())
    {
        const RowLayout& last = rows.back();
        float bottom = (expanded_index == last.step_index) ? (last.pane_top + kPaneHeight) : last.header_bottom;
        total_content_height = bottom - (static_cast<float>(origin_y) - scroll_y);
    }

    render_target->PopAxisAlignedClip();
}
