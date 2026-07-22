#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

class Win32AppearancePanel
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int x, int y, int width, int height);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    bool contains(int client_x, int client_y) const;
    bool on_mouse_down(int client_x, int client_y);
    void on_mouse_move(int client_x, int client_y);
    bool on_mouse_up();

    bool font_size_changed_and_clear();
    bool is_dragging() const { return dragging_slider; }

private:
    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;

    bool dragging_slider = false;
    bool pending_font_change = false;

    static constexpr float kRowHeight = 24.0f;
    static constexpr float kCheckboxSize = 14.0f;
    static constexpr float kSliderWidth = 260.0f;

    RECT slider_rect() const;
    RECT checkbox_rect() const;
    void set_font_size_from_x(int client_x);
};
