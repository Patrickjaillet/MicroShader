#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "win32_animation.h"

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteTextFormat;
struct IDWriteFactory;
struct ThemeBrushes;
class IconSet;

enum class TitleBarHit
{
    None,
    Drag,
    Minimize,
    Maximize,
    Close,
};

class TitleBar
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int window_width);
    TitleBarHit hit_test(int x, int y) const;
    void set_hover(TitleBarHit hit);
    void tick();
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes, const IconSet& icons, const wchar_t* title) const;

    static constexpr float kHeight = 32.0f;

private:
    static constexpr float kButtonWidth = 46.0f;

    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;
    int width = 0;
    TitleBarHit hovered = TitleBarHit::None;

    AnimatedColor minimize_anim;
    AnimatedColor maximize_anim;
    AnimatedColor close_anim;

    RECT minimize_rect() const;
    RECT maximize_rect() const;
    RECT close_rect() const;
};
