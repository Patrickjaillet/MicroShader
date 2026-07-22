#pragma once

#include "win32_animation.h"

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteTextFormat;
struct IDWriteFactory;
struct ThemeBrushes;

class TabStrip
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int origin_x, int origin_y, int window_width);
    int hit_test(int x, int y) const;
    void set_hover(int index);
    void set_focused(bool focused_value);
    void switch_to(int index);
    int active_index() const { return active; }
    void tick();
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    static constexpr float kHeight = 32.0f;
    static constexpr int kTabCount = 9;
    int origin_y() const { return top; }

private:
    static constexpr float kTabWidth = 120.0f;
    static constexpr float kCornerRadius = 2.0f;

    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;
    int left = 0;
    int top = 0;
    int hovered_index = -1;
    int active = 0;
    bool focused = false;

    AnimatedColor hover_anim[kTabCount];
    AnimatedColor open_anim;
};
