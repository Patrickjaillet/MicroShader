#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <chrono>
#include <string>
#include <vector>

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

class Win32TextEditor
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory, bool read_only_editor);
    void destroy();

    void set_text_utf8(const std::string& utf8_text);
    std::string text_utf8() const;

    void layout(int x, int y, int width, int height);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    bool on_char(wchar_t character);
    bool on_key_down(WPARAM key, bool ctrl_held, bool shift_held);
    void on_mouse_down(int client_x, int client_y, bool shift_held);
    void on_mouse_move(int client_x, int client_y, bool dragging);
    void on_mouse_wheel(int wheel_delta);
    void tick();

    void set_focus(bool focused);
    void set_error_line(int one_based_line, const std::wstring& message);
    void clear_error_line();

    bool contains(int client_x, int client_y) const;
    const std::vector<std::wstring>& all_lines() const { return lines; }
    int line_count() const { return static_cast<int>(lines.size()); }

private:
    struct Caret
    {
        int line = 0;
        int col = 0;
    };

    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    std::vector<std::wstring> lines{ std::wstring() };
    std::vector<bool> starts_in_comment{ false };

    Caret caret;
    Caret selection_anchor;
    int desired_col = 0;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;

    float char_width = 8.0f;
    float line_height = 16.0f;
    static constexpr float kLeftPadding = 6.0f;

    int scroll_top_line = 0;
    float scroll_x = 0.0f;

    bool is_read_only = false;
    bool has_focus_flag = false;
    bool caret_blink_on = true;
    std::chrono::steady_clock::time_point last_blink_toggle{};

    int error_line = -1;
    std::wstring error_message;

    void retokenize_all();
    void ensure_caret_visible();
    bool has_selection() const;
    void get_selection_range(Caret& start, Caret& end) const;
    void delete_range(Caret start, Caret end);
    void insert_text(const std::wstring& text);
    void clamp_caret(Caret& c) const;
    int line_visible_count() const;
    int column_from_x(int line_index, int pixel_x) const;
    void copy_to_clipboard(bool cut);
    void paste_from_clipboard();
    void move_word(bool forward, bool extend_selection);
};
