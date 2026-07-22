#pragma once

#include <string>
#include <vector>

#include "win32_text_editor.h"
#include "golf_trace.h"

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

class Win32TraceView
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void set_steps(const std::vector<GolfTraceStep>& new_steps);
    void layout(int x, int y, int width, int height);
    void on_mouse_down(int client_x, int client_y);
    void on_mouse_wheel(int wheel_delta);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;
    bool contains(int client_x, int client_y) const;

private:
    struct RowLayout
    {
        int step_index = 0;
        float header_top = 0.0f;
        float header_bottom = 0.0f;
        float before_x = 0.0f;
        float after_x = 0.0f;
        float pane_top = 0.0f;
        float pane_width = 0.0f;
    };

    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;
    mutable Win32TextEditor before_editor;
    mutable Win32TextEditor after_editor;

    std::vector<GolfTraceStep> steps;
    int expanded_index = -1;
    float scroll_y = 0.0f;
    mutable float total_content_height = 0.0f;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;

    static constexpr float kHeaderHeight = 28.0f;
    static constexpr float kLabelHeight = 18.0f;
    static constexpr float kPaneHeight = 180.0f;

    std::vector<RowLayout> compute_rows() const;
};
