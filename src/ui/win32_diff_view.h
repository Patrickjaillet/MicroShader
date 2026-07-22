#pragma once

#include <string>
#include <vector>

#include "unified_diff.h"

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

class Win32DiffView
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void set_diff(const std::vector<DiffSpan>& spans);
    void layout(int x, int y, int width, int height);
    void on_mouse_wheel(int wheel_delta);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    bool contains(int client_x, int client_y) const;

private:
    struct DrawSpan
    {
        float x = 0.0f;
        float row = 0.0f;
        std::wstring text;
        DiffSpanKind kind = DiffSpanKind::Unchanged;
    };

    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    std::vector<DiffSpan> spans;
    std::vector<DrawSpan> draw_spans;
    int row_count = 0;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;
    float char_width = 8.0f;
    float line_height = 16.0f;
    int scroll_top_row = 0;

    void rebuild_layout();
};
