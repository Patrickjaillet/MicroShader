#include "win32_value_sliders_panel.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>
#include <cstdio>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "../platform/utf8.h"
#include "../platform/accessibility_core.h"

bool Win32ValueSlidersPanel::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(11.0f), L"en-us", &text_format)))
    {
        return false;
    }
    text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Consolas", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(12.0f), L"en-us", &value_format)))
    {
        return false;
    }
    value_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    value_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    return SUCCEEDED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush));
}

void Win32ValueSlidersPanel::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (value_format != nullptr) { value_format->Release(); value_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32ValueSlidersPanel::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
}

bool Win32ValueSlidersPanel::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

void Win32ValueSlidersPanel::sync_from_source(const std::string& source)
{
    source_text = source;
    rows.clear();
    dragging_index = -1;
    scroll_offset_rows = 0;

    for (const GlslNumericLiteral& literal : find_glsl_float_literals(source))
    {
        Row row;
        row.literal = literal;
        row.range = compute_slider_range(literal.value);
        row.context = literal_context_snippet(source, literal.byte_offset, literal.byte_length, 30);
        rows.push_back(std::move(row));
    }
}

int Win32ValueSlidersPanel::visible_row_count() const
{
    return std::max(1, static_cast<int>(static_cast<float>(height_px) / kRowHeight));
}

RECT Win32ValueSlidersPanel::slider_rect_for_visible_row(int visible_row) const
{
    LONG row_top = origin_y + static_cast<LONG>(static_cast<float>(visible_row) * kRowHeight);
    LONG slider_top = row_top + static_cast<LONG>(kContextHeight) + 8;
    return RECT{
        origin_x + static_cast<LONG>(kSliderPadding), slider_top,
        origin_x + width_px - static_cast<LONG>(kSliderPadding), slider_top + 8
    };
}

int Win32ValueSlidersPanel::row_at_client_y(int client_y) const
{
    if (rows.empty())
    {
        return -1;
    }
    int visible_row = static_cast<int>(static_cast<float>(client_y - origin_y) / kRowHeight);
    if (visible_row < 0 || visible_row >= visible_row_count())
    {
        return -1;
    }
    int absolute_row = visible_row + scroll_offset_rows;
    if (absolute_row < 0 || absolute_row >= static_cast<int>(rows.size()))
    {
        return -1;
    }
    return absolute_row;
}

void Win32ValueSlidersPanel::apply_value_to_row(int index, double new_value)
{
    Row& row = rows[static_cast<size_t>(index)];
    new_value = std::clamp(new_value, row.range.min_value, row.range.max_value);
    std::string new_text = format_glsl_float_literal(new_value);
    std::string old_text = source_text.substr(row.literal.byte_offset, row.literal.byte_length);
    if (new_text == old_text)
    {
        return;
    }

    source_text = splice_source(source_text, row.literal.byte_offset, row.literal.byte_length, new_text);
    long long delta = static_cast<long long>(new_text.size()) - static_cast<long long>(row.literal.byte_length);
    row.literal.byte_length = new_text.size();
    row.literal.value = new_value;
    for (size_t i = static_cast<size_t>(index) + 1; i < rows.size(); ++i)
    {
        rows[i].literal.byte_offset = static_cast<std::size_t>(
            static_cast<long long>(rows[i].literal.byte_offset) + delta);
    }
}

void Win32ValueSlidersPanel::set_value_from_x(int index, int client_x)
{
    int visible_row = index - scroll_offset_rows;
    RECT slider = slider_rect_for_visible_row(visible_row);
    float t = static_cast<float>(client_x - slider.left) / static_cast<float>(slider.right - slider.left);
    t = std::clamp(t, 0.0f, 1.0f);
    const SliderRange& range = rows[static_cast<size_t>(index)].range;
    double value = range.min_value + static_cast<double>(t) * (range.max_value - range.min_value);
    apply_value_to_row(index, value);
}

bool Win32ValueSlidersPanel::on_mouse_down(int client_x, int client_y)
{
    int index = row_at_client_y(client_y);
    if (index < 0)
    {
        return false;
    }
    RECT slider = slider_rect_for_visible_row(index - scroll_offset_rows);
    RECT hit{ slider.left - 6, slider.top - 12, slider.right + 6, slider.bottom + 12 };
    POINT pt{ client_x, client_y };
    if (!PtInRect(&hit, pt))
    {
        return false;
    }
    dragging_index = index;
    set_value_from_x(index, client_x);
    return true;
}

bool Win32ValueSlidersPanel::on_mouse_move(int client_x, int)
{
    if (dragging_index < 0)
    {
        return false;
    }
    std::string before = source_text;
    set_value_from_x(dragging_index, client_x);
    return source_text != before;
}

void Win32ValueSlidersPanel::on_mouse_up()
{
    dragging_index = -1;
}

void Win32ValueSlidersPanel::on_mouse_wheel(int wheel_delta)
{
    int rows_to_scroll = -(wheel_delta / WHEEL_DELTA) * 3;
    scroll_offset_rows += rows_to_scroll;
    int max_top = std::max(0, static_cast<int>(rows.size()) - visible_row_count());
    scroll_offset_rows = std::clamp(scroll_offset_rows, 0, max_top);
}

double Win32ValueSlidersPanel::row_value_at(int index) const
{
    if (index < 0 || index >= static_cast<int>(rows.size()))
    {
        return 0.0;
    }
    return rows[static_cast<size_t>(index)].literal.value;
}

void Win32ValueSlidersPanel::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_panel);

    if (dynamic_brush == nullptr || text_format == nullptr || value_format == nullptr)
    {
        return;
    }

    if (rows.empty())
    {
        D2D1_RECT_F msg_rect = D2D1::RectF(bg_rect.left + 8.0f, bg_rect.top + 12.0f, bg_rect.right - 8.0f, bg_rect.top + 60.0f);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z));
        const wchar_t* message = L"No numeric values found in the source.";
        render_target->DrawText(message, static_cast<UINT32>(wcslen(message)), text_format, msg_rect, dynamic_brush);
        return;
    }

    render_target->PushAxisAlignedClip(bg_rect, D2D1_ANTIALIAS_MODE_ALIASED);

    int visible = visible_row_count();
    for (int visible_row = 0; visible_row < visible; ++visible_row)
    {
        int index = visible_row + scroll_offset_rows;
        if (index >= static_cast<int>(rows.size()))
        {
            break;
        }
        const Row& row = rows[static_cast<size_t>(index)];

        float row_top = static_cast<float>(origin_y) + static_cast<float>(visible_row) * kRowHeight;

        D2D1_RECT_F context_rect = D2D1::RectF(static_cast<float>(origin_x) + 8.0f, row_top + 2.0f,
            static_cast<float>(origin_x + width_px) - 72.0f, row_top + kContextHeight);
        std::wstring context_wide = utf8_to_wide(row.context);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z));
        render_target->DrawText(context_wide.c_str(), static_cast<UINT32>(context_wide.size()), text_format, context_rect, dynamic_brush);

        D2D1_RECT_F value_rect = D2D1::RectF(static_cast<float>(origin_x + width_px) - 68.0f, row_top,
            static_cast<float>(origin_x + width_px) - 8.0f, row_top + kContextHeight + 4.0f);
        std::string value_text = format_glsl_float_literal(row.literal.value);
        std::wstring value_wide = utf8_to_wide(value_text);
        dynamic_brush->SetColor(index == dragging_index
            ? D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z)
            : D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
        render_target->DrawText(value_wide.c_str(), static_cast<UINT32>(value_wide.size()), value_format, value_rect, dynamic_brush);

        RECT slider = slider_rect_for_visible_row(visible_row);
        D2D1_RECT_F track_rect = D2D1::RectF(static_cast<float>(slider.left), static_cast<float>(slider.top),
            static_cast<float>(slider.right), static_cast<float>(slider.bottom));
        render_target->FillRectangle(track_rect, brushes.bg_panel_raised);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
        render_target->DrawRectangle(track_rect, dynamic_brush, 1.0f);

        double t = (row.range.max_value > row.range.min_value)
            ? (row.literal.value - row.range.min_value) / (row.range.max_value - row.range.min_value)
            : 0.5;
        t = std::clamp(t, 0.0, 1.0);
        float thumb_x = static_cast<float>(slider.left) + static_cast<float>(t) * static_cast<float>(slider.right - slider.left);
        float thumb_y = (static_cast<float>(slider.top) + static_cast<float>(slider.bottom)) * 0.5f;
        D2D1_ELLIPSE thumb = D2D1::Ellipse(D2D1::Point2F(thumb_x, thumb_y), 6.0f, 6.0f);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z));
        render_target->FillEllipse(thumb, dynamic_brush);

        char accessible_name[192];
        std::snprintf(accessible_name, sizeof(accessible_name), "Value slider: %s = %s",
            row.context.c_str(), value_text.c_str());
        accessibility_register(accessible_name, AccessibleRole::Button,
            static_cast<float>(slider.left), row_top,
            static_cast<float>(slider.right - slider.left), kRowHeight, true);
    }

    render_target->PopAxisAlignedClip();
}
