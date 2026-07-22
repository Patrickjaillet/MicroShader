#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

class Win32AboutPanel
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int x, int y, int width, int height);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    bool contains(int client_x, int client_y) const;
    void on_mouse_move(int client_x, int client_y);
    bool on_mouse_down(int client_x, int client_y);

    static constexpr int kLinkCount = 3;

private:
    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;

    int hovered_link = -1;

    static constexpr float kRowHeight = 22.0f;

    RECT row_rect(int row) const;
    RECT link_rect(int index) const;
};
