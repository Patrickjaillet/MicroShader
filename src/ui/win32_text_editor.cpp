#include "win32_text_editor.h"

#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "glsl_token_rules.h"
#include "glsl_syntax_colors.h"
#include "theme_tokens.h"
#include "../platform/utf8.h"

namespace
{
    bool is_word_char(wchar_t c)
    {
        return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'_';
    }
}

bool Win32TextEditor::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory, bool read_only_editor)
{
    is_read_only = read_only_editor;

    HRESULT hr = dwrite_factory->CreateTextFormat(
        L"Consolas", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(14.0f), L"en-us", &text_format);
    if (FAILED(hr))
    {
        return false;
    }

    text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    IDWriteTextLayout* probe = nullptr;
    if (SUCCEEDED(dwrite_factory->CreateTextLayout(L"M", 1, text_format, 1000.0f, 1000.0f, &probe)))
    {
        DWRITE_TEXT_METRICS metrics{};
        probe->GetMetrics(&metrics);
        if (metrics.width > 0.0f) { char_width = metrics.width; }
        if (metrics.height > 0.0f) { line_height = metrics.height; }
        probe->Release();
    }

    if (FAILED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush)))
    {
        return false;
    }

    last_blink_toggle = std::chrono::steady_clock::now();
    retokenize_all();
    return true;
}

void Win32TextEditor::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32TextEditor::set_text_utf8(const std::string& utf8_text)
{
    std::wstring wide = utf8_to_wide(utf8_text);
    lines.clear();
    size_t start = 0;
    for (size_t i = 0; i <= wide.size(); ++i)
    {
        if (i == wide.size() || wide[i] == L'\n')
        {
            std::wstring line = wide.substr(start, i - start);
            if (!line.empty() && line.back() == L'\r')
            {
                line.pop_back();
            }
            lines.push_back(line);
            start = i + 1;
        }
    }
    if (lines.empty())
    {
        lines.push_back(std::wstring());
    }
    caret = Caret{ 0, 0 };
    selection_anchor = caret;
    scroll_top_line = 0;
    scroll_x = 0.0f;
    retokenize_all();
}

std::string Win32TextEditor::text_utf8() const
{
    std::wstring combined;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        combined += lines[i];
        if (i + 1 < lines.size())
        {
            combined += L'\n';
        }
    }
    return wide_to_utf8(combined.c_str());
}

void Win32TextEditor::retokenize_all()
{
    starts_in_comment.assign(lines.size(), false);
    bool state = false;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        starts_in_comment[i] = state;
        tokenize_glsl_line(lines[i], state);
    }
}

void Win32TextEditor::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
}

bool Win32TextEditor::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

int Win32TextEditor::line_visible_count() const
{
    return std::max(1, static_cast<int>(static_cast<float>(height_px) / line_height));
}

bool Win32TextEditor::has_selection() const
{
    return !(caret.line == selection_anchor.line && caret.col == selection_anchor.col);
}

void Win32TextEditor::get_selection_range(Caret& start, Caret& end) const
{
    if (selection_anchor.line < caret.line || (selection_anchor.line == caret.line && selection_anchor.col <= caret.col))
    {
        start = selection_anchor;
        end = caret;
    }
    else
    {
        start = caret;
        end = selection_anchor;
    }
}

void Win32TextEditor::clamp_caret(Caret& c) const
{
    if (c.line < 0) { c.line = 0; }
    if (c.line >= static_cast<int>(lines.size())) { c.line = static_cast<int>(lines.size()) - 1; }
    int len = static_cast<int>(lines[c.line].size());
    if (c.col < 0) { c.col = 0; }
    if (c.col > len) { c.col = len; }
}

void Win32TextEditor::delete_range(Caret start, Caret end)
{
    if (start.line == end.line)
    {
        lines[start.line].erase(static_cast<size_t>(start.col), static_cast<size_t>(end.col - start.col));
    }
    else
    {
        std::wstring merged = lines[start.line].substr(0, start.col) + lines[end.line].substr(end.col);
        lines.erase(lines.begin() + start.line + 1, lines.begin() + end.line + 1);
        lines[start.line] = merged;
    }
    retokenize_all();
}

void Win32TextEditor::insert_text(const std::wstring& text)
{
    if (has_selection())
    {
        Caret start, end;
        get_selection_range(start, end);
        delete_range(start, end);
        caret = start;
        selection_anchor = start;
    }

    std::vector<std::wstring> parts;
    size_t seg_start = 0;
    for (size_t i = 0; i <= text.size(); ++i)
    {
        if (i == text.size() || text[i] == L'\n')
        {
            parts.push_back(text.substr(seg_start, i - seg_start));
            seg_start = i + 1;
        }
    }

    std::wstring tail = lines[caret.line].substr(caret.col);
    std::wstring prefix = lines[caret.line].substr(0, caret.col);

    if (parts.size() == 1)
    {
        lines[caret.line] = prefix + parts[0] + tail;
        caret.col = static_cast<int>(prefix.size() + parts[0].size());
    }
    else
    {
        lines[caret.line] = prefix + parts[0];
        for (size_t i = 1; i < parts.size(); ++i)
        {
            std::wstring new_line = (i + 1 == parts.size()) ? (parts[i] + tail) : parts[i];
            lines.insert(lines.begin() + caret.line + static_cast<int>(i), new_line);
        }
        caret.line += static_cast<int>(parts.size()) - 1;
        caret.col = static_cast<int>(parts.back().size());
    }

    selection_anchor = caret;
    retokenize_all();
    desired_col = caret.col;
    ensure_caret_visible();
}

void Win32TextEditor::ensure_caret_visible()
{
    int visible = line_visible_count();
    if (caret.line < scroll_top_line)
    {
        scroll_top_line = caret.line;
    }
    else if (caret.line >= scroll_top_line + visible)
    {
        scroll_top_line = caret.line - visible + 1;
    }
    if (scroll_top_line < 0)
    {
        scroll_top_line = 0;
    }

    float caret_x = static_cast<float>(caret.col) * char_width;
    float visible_width = static_cast<float>(width_px) - 8.0f;
    if (caret_x < scroll_x)
    {
        scroll_x = caret_x;
    }
    else if (caret_x > scroll_x + visible_width)
    {
        scroll_x = caret_x - visible_width;
    }
    if (scroll_x < 0.0f)
    {
        scroll_x = 0.0f;
    }
}

void Win32TextEditor::move_word(bool forward, bool extend_selection)
{
    std::wstring& line = lines[caret.line];
    int col = caret.col;
    if (forward)
    {
        int n = static_cast<int>(line.size());
        while (col < n && !is_word_char(line[col])) { ++col; }
        while (col < n && is_word_char(line[col])) { ++col; }
    }
    else
    {
        while (col > 0 && !is_word_char(line[col - 1])) { --col; }
        while (col > 0 && is_word_char(line[col - 1])) { --col; }
    }
    caret.col = col;
    if (!extend_selection)
    {
        selection_anchor = caret;
    }
}

bool Win32TextEditor::on_char(wchar_t character)
{
    if (is_read_only)
    {
        return false;
    }
    if (character == L'\r')
    {
        insert_text(L"\n");
        return true;
    }
    if (character < 32 && character != L'\t')
    {
        return false;
    }
    insert_text(std::wstring(1, character));
    return true;
}

bool Win32TextEditor::on_key_down(WPARAM key, bool ctrl_held, bool shift_held)
{
    switch (key)
    {
    case VK_LEFT:
        if (!shift_held && has_selection())
        {
            Caret start, end;
            get_selection_range(start, end);
            caret = start;
            selection_anchor = start;
        }
        else if (ctrl_held)
        {
            move_word(false, shift_held);
        }
        else
        {
            if (caret.col > 0) { caret.col--; }
            else if (caret.line > 0) { caret.line--; caret.col = static_cast<int>(lines[caret.line].size()); }
            if (!shift_held) { selection_anchor = caret; }
        }
        desired_col = caret.col;
        ensure_caret_visible();
        return true;
    case VK_RIGHT:
        if (!shift_held && has_selection())
        {
            Caret start, end;
            get_selection_range(start, end);
            caret = end;
            selection_anchor = end;
        }
        else if (ctrl_held)
        {
            move_word(true, shift_held);
        }
        else
        {
            if (caret.col < static_cast<int>(lines[caret.line].size())) { caret.col++; }
            else if (caret.line + 1 < static_cast<int>(lines.size())) { caret.line++; caret.col = 0; }
            if (!shift_held) { selection_anchor = caret; }
        }
        desired_col = caret.col;
        ensure_caret_visible();
        return true;
    case VK_UP:
        if (caret.line > 0) { caret.line--; caret.col = std::min(desired_col, static_cast<int>(lines[caret.line].size())); }
        else { caret.col = 0; }
        if (!shift_held) { selection_anchor = caret; }
        ensure_caret_visible();
        return true;
    case VK_DOWN:
        if (caret.line + 1 < static_cast<int>(lines.size())) { caret.line++; caret.col = std::min(desired_col, static_cast<int>(lines[caret.line].size())); }
        else { caret.col = static_cast<int>(lines[caret.line].size()); }
        if (!shift_held) { selection_anchor = caret; }
        ensure_caret_visible();
        return true;
    case VK_HOME:
        caret.col = 0;
        if (!shift_held) { selection_anchor = caret; }
        desired_col = 0;
        ensure_caret_visible();
        return true;
    case VK_END:
        caret.col = static_cast<int>(lines[caret.line].size());
        if (!shift_held) { selection_anchor = caret; }
        desired_col = caret.col;
        ensure_caret_visible();
        return true;
    case VK_PRIOR:
        caret.line = std::max(0, caret.line - line_visible_count());
        clamp_caret(caret);
        if (!shift_held) { selection_anchor = caret; }
        ensure_caret_visible();
        return true;
    case VK_NEXT:
        caret.line = std::min(static_cast<int>(lines.size()) - 1, caret.line + line_visible_count());
        clamp_caret(caret);
        if (!shift_held) { selection_anchor = caret; }
        ensure_caret_visible();
        return true;
    case VK_BACK:
        if (is_read_only) { return true; }
        if (has_selection())
        {
            Caret start, end;
            get_selection_range(start, end);
            delete_range(start, end);
            caret = start;
            selection_anchor = start;
        }
        else if (caret.col > 0)
        {
            Caret start{ caret.line, caret.col - 1 };
            delete_range(start, caret);
            caret = start;
            selection_anchor = start;
        }
        else if (caret.line > 0)
        {
            Caret start{ caret.line - 1, static_cast<int>(lines[caret.line - 1].size()) };
            delete_range(start, caret);
            caret = start;
            selection_anchor = start;
        }
        ensure_caret_visible();
        return true;
    case VK_DELETE:
        if (is_read_only) { return true; }
        if (has_selection())
        {
            Caret start, end;
            get_selection_range(start, end);
            delete_range(start, end);
            caret = start;
            selection_anchor = start;
        }
        else if (caret.col < static_cast<int>(lines[caret.line].size()))
        {
            Caret end{ caret.line, caret.col + 1 };
            delete_range(caret, end);
        }
        else if (caret.line + 1 < static_cast<int>(lines.size()))
        {
            Caret end{ caret.line + 1, 0 };
            delete_range(caret, end);
        }
        ensure_caret_visible();
        return true;
    case L'A':
        if (ctrl_held)
        {
            selection_anchor = Caret{ 0, 0 };
            caret = Caret{ static_cast<int>(lines.size()) - 1, static_cast<int>(lines.back().size()) };
            ensure_caret_visible();
            return true;
        }
        return false;
    case L'C':
        if (ctrl_held) { copy_to_clipboard(false); return true; }
        return false;
    case L'X':
        if (ctrl_held) { if (!is_read_only) { copy_to_clipboard(true); } return true; }
        return false;
    case L'V':
        if (ctrl_held) { if (!is_read_only) { paste_from_clipboard(); } return true; }
        return false;
    default:
        return false;
    }
}

int Win32TextEditor::column_from_x(int line_index, int pixel_x) const
{
    float rel = static_cast<float>(pixel_x - origin_x) - kLeftPadding + scroll_x;
    int col = static_cast<int>(rel / char_width + 0.5f);
    if (col < 0) { col = 0; }
    int len = static_cast<int>(lines[line_index].size());
    if (col > len) { col = len; }
    return col;
}

void Win32TextEditor::on_mouse_down(int client_x, int client_y, bool shift_held)
{
    int line = scroll_top_line + static_cast<int>(static_cast<float>(client_y - origin_y) / line_height);
    if (line < 0) { line = 0; }
    if (line >= static_cast<int>(lines.size())) { line = static_cast<int>(lines.size()) - 1; }
    int col = column_from_x(line, client_x);
    caret = Caret{ line, col };
    if (!shift_held)
    {
        selection_anchor = caret;
    }
    desired_col = col;
}

void Win32TextEditor::on_mouse_move(int client_x, int client_y, bool dragging)
{
    if (!dragging)
    {
        return;
    }
    int line = scroll_top_line + static_cast<int>(static_cast<float>(client_y - origin_y) / line_height);
    if (line < 0) { line = 0; }
    if (line >= static_cast<int>(lines.size())) { line = static_cast<int>(lines.size()) - 1; }
    int col = column_from_x(line, client_x);
    caret = Caret{ line, col };
    desired_col = col;
    ensure_caret_visible();
}

void Win32TextEditor::on_mouse_wheel(int wheel_delta)
{
    int lines_to_scroll = -(wheel_delta / WHEEL_DELTA) * 3;
    scroll_top_line += lines_to_scroll;
    if (scroll_top_line < 0) { scroll_top_line = 0; }
    int max_top = std::max(0, static_cast<int>(lines.size()) - 1);
    if (scroll_top_line > max_top) { scroll_top_line = max_top; }
}

void Win32TextEditor::tick()
{
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<float>(now - last_blink_toggle).count() > 0.53f)
    {
        caret_blink_on = !caret_blink_on;
        last_blink_toggle = now;
    }
}

void Win32TextEditor::set_focus(bool focused)
{
    has_focus_flag = focused;
    if (focused)
    {
        caret_blink_on = true;
        last_blink_toggle = std::chrono::steady_clock::now();
    }
}

void Win32TextEditor::set_error_line(int one_based_line, const std::wstring& message)
{
    error_line = one_based_line;
    error_message = message;
}

void Win32TextEditor::clear_error_line()
{
    error_line = -1;
    error_message.clear();
}

void Win32TextEditor::copy_to_clipboard(bool cut)
{
    Caret start, end;
    if (has_selection())
    {
        get_selection_range(start, end);
    }
    else
    {
        start = Caret{ caret.line, 0 };
        end = Caret{ caret.line, static_cast<int>(lines[caret.line].size()) };
    }

    std::wstring text;
    if (start.line == end.line)
    {
        text = lines[start.line].substr(start.col, end.col - start.col);
    }
    else
    {
        text = lines[start.line].substr(start.col);
        for (int i = start.line + 1; i < end.line; ++i)
        {
            text += L'\n';
            text += lines[i];
        }
        text += L'\n';
        text += lines[end.line].substr(0, end.col);
    }

    if (OpenClipboard(nullptr))
    {
        EmptyClipboard();
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
        if (mem != nullptr)
        {
            void* dest = GlobalLock(mem);
            if (dest != nullptr)
            {
                memcpy(dest, text.c_str(), (text.size() + 1) * sizeof(wchar_t));
                GlobalUnlock(mem);
                SetClipboardData(CF_UNICODETEXT, mem);
            }
        }
        CloseClipboard();
    }

    if (cut && !is_read_only)
    {
        delete_range(start, end);
        caret = start;
        selection_anchor = start;
    }
}

void Win32TextEditor::paste_from_clipboard()
{
    if (!OpenClipboard(nullptr))
    {
        return;
    }

    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data != nullptr)
    {
        const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(data));
        if (text != nullptr)
        {
            std::wstring clipboard_text(text);
            std::wstring normalized;
            normalized.reserve(clipboard_text.size());
            for (wchar_t c : clipboard_text)
            {
                if (c != L'\r')
                {
                    normalized.push_back(c);
                }
            }
            insert_text(normalized);
            GlobalUnlock(data);
        }
    }

    CloseClipboard();
}

void Win32TextEditor::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_panel);

    if (dynamic_brush == nullptr || text_format == nullptr)
    {
        return;
    }

    render_target->PushAxisAlignedClip(bg_rect, D2D1_ANTIALIAS_MODE_ALIASED);

    int first_line = scroll_top_line;
    int last_line = std::min(static_cast<int>(lines.size()) - 1, first_line + line_visible_count());

    Caret sel_start{}, sel_end{};
    bool selecting = has_selection();
    if (selecting)
    {
        get_selection_range(sel_start, sel_end);
    }

    for (int i = first_line; i <= last_line; ++i)
    {
        float y = static_cast<float>(origin_y) + static_cast<float>(i - first_line) * line_height;

        if (i == error_line - 1)
        {
            D2D1_RECT_F err_rect = D2D1::RectF(static_cast<float>(origin_x), y,
                static_cast<float>(origin_x + width_px), y + line_height);
            dynamic_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Red, 0.12f));
            render_target->FillRectangle(err_rect, dynamic_brush);
        }

        if (selecting && i >= sel_start.line && i <= sel_end.line)
        {
            int col_a = (i == sel_start.line) ? sel_start.col : 0;
            int col_b = (i == sel_end.line) ? sel_end.col : static_cast<int>(lines[i].size());
            float sx = static_cast<float>(origin_x) + kLeftPadding + static_cast<float>(col_a) * char_width - scroll_x;
            float ex = static_cast<float>(origin_x) + kLeftPadding + static_cast<float>(col_b) * char_width - scroll_x;
            D2D1_RECT_F sel_rect = D2D1::RectF(sx, y, ex, y + line_height);
            dynamic_brush->SetColor(D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z, 0.35f));
            render_target->FillRectangle(sel_rect, dynamic_brush);
        }

        bool comment_state = starts_in_comment[static_cast<size_t>(i)];
        std::vector<GlslTokenSpan> spans = tokenize_glsl_line(lines[static_cast<size_t>(i)], comment_state);
        for (const GlslTokenSpan& span : spans)
        {
            std::wstring token_text = lines[static_cast<size_t>(i)].substr(static_cast<size_t>(span.start), static_cast<size_t>(span.length));
            float x = static_cast<float>(origin_x) + kLeftPadding + static_cast<float>(span.start) * char_width - scroll_x;
            D2D1_RECT_F text_rect = D2D1::RectF(x, y, x + static_cast<float>(span.length) * char_width + 4.0f, y + line_height);
            dynamic_brush->SetColor(glsl_syntax_color(span.kind));
            render_target->DrawText(token_text.c_str(), static_cast<UINT32>(token_text.size()), text_format, text_rect, dynamic_brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        if (has_focus_flag && caret_blink_on && caret.line == i)
        {
            float cx = static_cast<float>(origin_x) + kLeftPadding + static_cast<float>(caret.col) * char_width - scroll_x;
            dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z, 1.0f));
            render_target->FillRectangle(D2D1::RectF(cx, y, cx + 1.5f, y + line_height), dynamic_brush);
        }
    }

    render_target->PopAxisAlignedClip();
    (void)brushes;
}
