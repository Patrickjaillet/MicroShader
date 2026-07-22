#include "win32_command_palette.h"

#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "fuzzy_match.h"
#include "theme_tokens.h"
#include "../platform/utf8.h"

bool Win32CommandPalette::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(16.0f), L"en-us", &query_format)))
    {
        return false;
    }
    query_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &item_format)))
    {
        return false;
    }
    item_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    return SUCCEEDED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush));
}

void Win32CommandPalette::destroy()
{
    if (query_format != nullptr) { query_format->Release(); query_format = nullptr; }
    if (item_format != nullptr) { item_format->Release(); item_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32CommandPalette::open(const std::vector<PaletteCommand>& new_commands)
{
    commands = new_commands;
    query.clear();
    selected = 0;
    scroll_offset = 0;
    hovered = -1;
    open_flag = true;
    caret_blink_on = true;
    refilter();
}

void Win32CommandPalette::close()
{
    open_flag = false;
}

void Win32CommandPalette::layout(int window_width, int window_height)
{
    window_width_px = window_width;
    window_height_px = window_height;
    box_x = (static_cast<float>(window_width) - box_width) * 0.5f;
    box_y = static_cast<float>(window_height) * 0.18f;
}

void Win32CommandPalette::refilter()
{
    filtered_indices.clear();
    for (size_t i = 0; i < commands.size(); ++i)
    {
        if (fuzzy_match(query, commands[i].label))
        {
            filtered_indices.push_back(static_cast<int>(i));
        }
    }
    if (selected >= static_cast<int>(filtered_indices.size()))
    {
        selected = std::max(0, static_cast<int>(filtered_indices.size()) - 1);
    }
    scroll_offset = 0;
    ensure_selected_visible();
}

void Win32CommandPalette::ensure_selected_visible()
{
    if (selected < scroll_offset)
    {
        scroll_offset = selected;
    }
    else if (selected >= scroll_offset + kMaxVisibleItems)
    {
        scroll_offset = selected - kMaxVisibleItems + 1;
    }
    if (scroll_offset < 0)
    {
        scroll_offset = 0;
    }
}

bool Win32CommandPalette::on_char(wchar_t character)
{
    if (!open_flag)
    {
        return false;
    }
    if (character >= 32)
    {
        query.push_back(character < 128 ? static_cast<char>(character) : '?');
        selected = 0;
        refilter();
    }
    return true;
}

bool Win32CommandPalette::on_key_down(WPARAM key)
{
    if (!open_flag)
    {
        return false;
    }

    switch (key)
    {
    case VK_ESCAPE:
        close();
        return true;
    case VK_BACK:
        if (!query.empty())
        {
            query.pop_back();
            selected = 0;
            refilter();
        }
        return true;
    case VK_UP:
        if (!filtered_indices.empty())
        {
            selected = (selected - 1 + static_cast<int>(filtered_indices.size())) % static_cast<int>(filtered_indices.size());
            ensure_selected_visible();
        }
        return true;
    case VK_DOWN:
        if (!filtered_indices.empty())
        {
            selected = (selected + 1) % static_cast<int>(filtered_indices.size());
            ensure_selected_visible();
        }
        return true;
    case VK_RETURN:
        if (selected >= 0 && selected < static_cast<int>(filtered_indices.size()))
        {
            std::function<void()> execute = commands[static_cast<size_t>(filtered_indices[static_cast<size_t>(selected)])].execute;
            close();
            if (execute)
            {
                execute();
            }
        }
        return true;
    default:
        return true;
    }
}

int Win32CommandPalette::filtered_index_at(float y, float x) const
{
    if (x < box_x || x > box_x + box_width)
    {
        return -1;
    }
    float list_top = box_y + kQueryHeight;
    if (y < list_top)
    {
        return -1;
    }
    int visible_row = static_cast<int>((y - list_top) / kItemHeight);
    if (visible_row < 0 || visible_row >= kMaxVisibleItems)
    {
        return -1;
    }
    int index = visible_row + scroll_offset;
    if (index >= static_cast<int>(filtered_indices.size()))
    {
        return -1;
    }
    return index;
}

void Win32CommandPalette::on_mouse_move(int client_x, int client_y)
{
    if (!open_flag)
    {
        return;
    }
    hovered = filtered_index_at(static_cast<float>(client_y), static_cast<float>(client_x));
}

bool Win32CommandPalette::on_mouse_down(int client_x, int client_y)
{
    if (!open_flag)
    {
        return false;
    }

    int index = filtered_index_at(static_cast<float>(client_y), static_cast<float>(client_x));
    if (index >= 0)
    {
        std::function<void()> execute = commands[static_cast<size_t>(filtered_indices[static_cast<size_t>(index)])].execute;
        close();
        if (execute)
        {
            execute();
        }
        return true;
    }

    float list_height = static_cast<float>(std::min(static_cast<int>(filtered_indices.size()), kMaxVisibleItems)) * kItemHeight;
    float box_bottom = box_y + kQueryHeight + list_height;
    bool inside_box = client_x >= box_x && client_x <= box_x + box_width && client_y >= box_y && client_y <= box_bottom;
    if (!inside_box)
    {
        close();
    }
    return true;
}

void Win32CommandPalette::tick()
{
    LONGLONG now_ms = static_cast<LONGLONG>(GetTickCount64());
    if (now_ms - last_blink_ms > 530)
    {
        caret_blink_on = !caret_blink_on;
        last_blink_ms = now_ms;
    }
}

void Win32CommandPalette::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    if (!open_flag || dynamic_brush == nullptr)
    {
        return;
    }

    D2D1_RECT_F scrim_rect = D2D1::RectF(0.0f, 0.0f, static_cast<float>(window_width_px), static_cast<float>(window_height_px));
    dynamic_brush->SetColor(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.45f));
    render_target->FillRectangle(scrim_rect, dynamic_brush);

    float list_height = static_cast<float>(std::min(static_cast<int>(filtered_indices.size()), kMaxVisibleItems)) * kItemHeight;
    D2D1_RECT_F box_rect = D2D1::RectF(box_x, box_y, box_x + box_width, box_y + kQueryHeight + list_height);
    D2D1_ROUNDED_RECT rounded_box = D2D1::RoundedRect(box_rect, 2.0f, 2.0f);
    render_target->FillRoundedRectangle(rounded_box, brushes.bg_panel_raised);
    render_target->DrawRoundedRectangle(rounded_box, brushes.accent, 1.0f);

    D2D1_RECT_F query_rect = D2D1::RectF(box_x + 10.0f, box_y, box_x + box_width - 10.0f, box_y + kQueryHeight);
    std::wstring display_text = query.empty() ? L"Type a command..." : utf8_to_wide(query);
    dynamic_brush->SetColor(query.empty()
        ? D2D1::ColorF(tokens::text_disabled.x, tokens::text_disabled.y, tokens::text_disabled.z)
        : D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
    render_target->DrawText(display_text.c_str(), static_cast<UINT32>(display_text.size()), query_format, query_rect, dynamic_brush);

    if (caret_blink_on)
    {
        float caret_x = query_rect.left + static_cast<float>(query.size()) * 8.5f;
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
        render_target->FillRectangle(
            D2D1::RectF(caret_x + 2.0f, box_y + 8.0f, caret_x + 3.5f, box_y + kQueryHeight - 8.0f), dynamic_brush);
    }

    render_target->DrawLine(
        D2D1::Point2F(box_x + 1.0f, box_y + kQueryHeight), D2D1::Point2F(box_x + box_width - 1.0f, box_y + kQueryHeight),
        brushes.border_subtle, 1.0f);

    int visible_count = std::min(static_cast<int>(filtered_indices.size()) - scroll_offset, kMaxVisibleItems);
    for (int row = 0; row < visible_count; ++row)
    {
        int filtered_pos = row + scroll_offset;
        int command_index = filtered_indices[static_cast<size_t>(filtered_pos)];

        float row_top = box_y + kQueryHeight + static_cast<float>(row) * kItemHeight;
        D2D1_RECT_F row_rect = D2D1::RectF(box_x, row_top, box_x + box_width, row_top + kItemHeight);

        if (filtered_pos == selected)
        {
            dynamic_brush->SetColor(D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z, 0.35f));
            render_target->FillRectangle(row_rect, dynamic_brush);
        }
        else if (filtered_pos == hovered)
        {
            dynamic_brush->SetColor(D2D1::ColorF(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 1.0f));
            render_target->FillRectangle(row_rect, dynamic_brush);
        }

        D2D1_RECT_F label_rect = D2D1::RectF(row_rect.left + 12.0f, row_rect.top, row_rect.right - 12.0f, row_rect.bottom);
        std::wstring label = utf8_to_wide(commands[static_cast<size_t>(command_index)].label);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
        render_target->DrawText(label.c_str(), static_cast<UINT32>(label.size()), item_format, label_rect, dynamic_brush);
    }

    if (filtered_indices.empty())
    {
        D2D1_RECT_F empty_rect = D2D1::RectF(box_x + 12.0f, box_y + kQueryHeight, box_x + box_width - 12.0f, box_y + kQueryHeight + kItemHeight);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z));
        const wchar_t* no_match = L"No matching commands";
        render_target->DrawText(no_match, static_cast<UINT32>(wcslen(no_match)), item_format, empty_rect, dynamic_brush);
    }
}
