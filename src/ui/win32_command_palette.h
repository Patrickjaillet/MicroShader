#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <functional>
#include <string>
#include <vector>

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

struct PaletteCommand
{
    std::string label;
    std::function<void()> execute;
};

class Win32CommandPalette
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void open(const std::vector<PaletteCommand>& commands);
    void close();
    bool is_open() const { return open_flag; }

    void layout(int window_width, int window_height);
    bool on_char(wchar_t character);
    bool on_key_down(WPARAM key);
    void on_mouse_move(int client_x, int client_y);
    bool on_mouse_down(int client_x, int client_y);
    void tick();
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

private:
    IDWriteTextFormat* query_format = nullptr;
    IDWriteTextFormat* item_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    std::vector<PaletteCommand> commands;
    std::vector<int> filtered_indices;
    std::string query;
    int selected = 0;
    int hovered = -1;
    bool open_flag = false;
    bool caret_blink_on = true;
    LONGLONG last_blink_ms = 0;

    float box_x = 0.0f;
    float box_y = 0.0f;
    float box_width = 480.0f;
    int window_width_px = 0;
    int window_height_px = 0;
    int scroll_offset = 0;
    static constexpr float kQueryHeight = 36.0f;
    static constexpr float kItemHeight = 26.0f;
    static constexpr int kMaxVisibleItems = 9;

    void refilter();
    void ensure_selected_visible();
    int filtered_index_at(float y, float x) const;
};
