#include "win32_title_bar.h"

#include <d2d1.h>
#include <dwrite.h>

#include "win32_theme_brushes.h"
#include "win32_icon_set.h"
#include "theme_tokens.h"
#include "../platform/accessibility_core.h"

bool TitleBar::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    HRESULT hr = dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        13.0f, L"en-us", &text_format);
    if (FAILED(hr))
    {
        return false;
    }

    text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    if (FAILED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush)))
    {
        return false;
    }

    minimize_anim.snap_to(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 0.0f);
    maximize_anim.snap_to(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 0.0f);
    close_anim.snap_to(tokens::accent.x, tokens::accent.y, tokens::accent.z, 0.0f);
    return true;
}

void TitleBar::destroy()
{
    if (text_format != nullptr)
    {
        text_format->Release();
        text_format = nullptr;
    }
    if (dynamic_brush != nullptr)
    {
        dynamic_brush->Release();
        dynamic_brush = nullptr;
    }
}

void TitleBar::layout(int window_width)
{
    width = window_width;
}

RECT TitleBar::minimize_rect() const
{
    LONG right = static_cast<LONG>(width) - static_cast<LONG>(kButtonWidth) * 3;
    return RECT{ right, 0, right + static_cast<LONG>(kButtonWidth), static_cast<LONG>(kHeight) };
}

RECT TitleBar::maximize_rect() const
{
    LONG right = static_cast<LONG>(width) - static_cast<LONG>(kButtonWidth) * 2;
    return RECT{ right, 0, right + static_cast<LONG>(kButtonWidth), static_cast<LONG>(kHeight) };
}

RECT TitleBar::close_rect() const
{
    LONG right = static_cast<LONG>(width) - static_cast<LONG>(kButtonWidth);
    return RECT{ right, 0, right + static_cast<LONG>(kButtonWidth), static_cast<LONG>(kHeight) };
}

TitleBarHit TitleBar::hit_test(int x, int y) const
{
    POINT pt{ x, y };
    RECT minimize_r = minimize_rect();
    RECT maximize_r = maximize_rect();
    RECT close_r = close_rect();
    if (PtInRect(&minimize_r, pt) != 0) { return TitleBarHit::Minimize; }
    if (PtInRect(&maximize_r, pt) != 0) { return TitleBarHit::Maximize; }
    if (PtInRect(&close_r, pt) != 0) { return TitleBarHit::Close; }
    if (y >= 0 && y < static_cast<int>(kHeight)) { return TitleBarHit::Drag; }
    return TitleBarHit::None;
}

void TitleBar::set_hover(TitleBarHit hit)
{
    if (hit == hovered)
    {
        return;
    }

    if (hovered == TitleBarHit::Minimize) { minimize_anim.set_target(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 0.0f); }
    if (hovered == TitleBarHit::Maximize) { maximize_anim.set_target(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 0.0f); }
    if (hovered == TitleBarHit::Close) { close_anim.set_target(tokens::accent.x, tokens::accent.y, tokens::accent.z, 0.0f); }

    hovered = hit;

    if (hovered == TitleBarHit::Minimize) { minimize_anim.set_target(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 1.0f); }
    if (hovered == TitleBarHit::Maximize) { maximize_anim.set_target(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 1.0f); }
    if (hovered == TitleBarHit::Close) { close_anim.set_target(tokens::accent.x, tokens::accent.y, tokens::accent.z, 1.0f); }
}

void TitleBar::tick()
{
    minimize_anim.tick();
    maximize_anim.tick();
    close_anim.tick();
}

namespace
{
    D2D1_RECT_F to_d2d_rect(const RECT& r)
    {
        return D2D1::RectF(static_cast<float>(r.left), static_cast<float>(r.top),
            static_cast<float>(r.right), static_cast<float>(r.bottom));
    }

    void fill_animated(ID2D1RenderTarget* render_target, ID2D1SolidColorBrush* brush, const RECT& rect, const AnimatedColor& anim)
    {
        float r, g, b, a;
        anim.current(r, g, b, a);
        if (a <= 0.0f)
        {
            return;
        }
        brush->SetColor(D2D1::ColorF(r, g, b, a));
        render_target->FillRectangle(to_d2d_rect(rect), brush);
    }
}

void TitleBar::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes, const IconSet& icons, const wchar_t* title) const
{
    D2D1_RECT_F bar_rect = D2D1::RectF(0.0f, 0.0f, static_cast<float>(width), kHeight);
    render_target->FillRectangle(bar_rect, brushes.bg_panel);

    if (text_format != nullptr)
    {
        D2D1_RECT_F title_rect = D2D1::RectF(12.0f, 0.0f, static_cast<float>(width) - kButtonWidth * 3, kHeight);
        render_target->DrawText(title, static_cast<UINT32>(wcslen(title)), text_format, title_rect, brushes.text_primary);
    }

    if (dynamic_brush != nullptr)
    {
        fill_animated(render_target, dynamic_brush, minimize_rect(), minimize_anim);
        fill_animated(render_target, dynamic_brush, maximize_rect(), maximize_anim);
        fill_animated(render_target, dynamic_brush, close_rect(), close_anim);
    }

    auto icon_origin = [](RECT r, int icon_size)
    {
        float cx = (r.left + r.right) * 0.5f;
        float cy = (r.top + r.bottom) * 0.5f;
        return D2D1::Point2F(cx - static_cast<float>(icon_size) * 0.5f, cy - static_cast<float>(icon_size) * 0.5f);
    };

    const int icon_size = 16;

    auto draw_icon = [&](const char* name, RECT r, bool active_hover)
    {
        D2D1_POINT_2F origin = icon_origin(r, icon_size);
        if (active_hover)
        {
            icons.draw(render_target, name, icon_size, origin.x, origin.y,
                tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z, 1.0f);
        }
        else
        {
            icons.draw(render_target, name, icon_size, origin.x, origin.y,
                tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z, 1.0f);
        }
    };

    draw_icon("minus", minimize_rect(), hovered == TitleBarHit::Minimize);
    draw_icon("square", maximize_rect(), hovered == TitleBarHit::Maximize);
    draw_icon("x", close_rect(), hovered == TitleBarHit::Close);

    auto announce = [](const char* name, RECT r)
    {
        accessibility_register(name, AccessibleRole::Button,
            static_cast<float>(r.left), static_cast<float>(r.top),
            static_cast<float>(r.right - r.left), static_cast<float>(r.bottom - r.top), true);
    };
    announce("Minimize", minimize_rect());
    announce("Maximize", maximize_rect());
    announce("Close", close_rect());
}
