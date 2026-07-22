#include "win32_theme_brushes.h"

#include <d2d1.h>

#include "theme_tokens.h"

namespace
{
    D2D1_COLOR_F to_d2d_color(const tokens::Color4& color)
    {
        return D2D1::ColorF(color.x, color.y, color.z, color.w);
    }

    ID2D1SolidColorBrush* make_brush(ID2D1RenderTarget* render_target, const tokens::Color4& color)
    {
        ID2D1SolidColorBrush* brush = nullptr;
        render_target->CreateSolidColorBrush(to_d2d_color(color), &brush);
        return brush;
    }
}

bool ThemeBrushes::create(ID2D1RenderTarget* render_target)
{
    bg_app = make_brush(render_target, tokens::bg_app);
    bg_panel = make_brush(render_target, tokens::bg_panel);
    bg_panel_raised = make_brush(render_target, tokens::bg_panel_raised);
    bg_hover = make_brush(render_target, tokens::bg_hover);
    bg_active = make_brush(render_target, tokens::bg_active);
    border_subtle = make_brush(render_target, tokens::border_subtle);
    accent = make_brush(render_target, tokens::accent);
    text_primary = make_brush(render_target, tokens::text_primary);
    text_secondary = make_brush(render_target, tokens::text_secondary);
    status_ok = make_brush(render_target, tokens::status_ok);

    return bg_app != nullptr && bg_panel != nullptr && bg_panel_raised != nullptr
        && bg_hover != nullptr && bg_active != nullptr && border_subtle != nullptr
        && accent != nullptr && text_primary != nullptr && text_secondary != nullptr
        && status_ok != nullptr;
}

void ThemeBrushes::release()
{
    if (bg_app != nullptr) { bg_app->Release(); bg_app = nullptr; }
    if (bg_panel != nullptr) { bg_panel->Release(); bg_panel = nullptr; }
    if (bg_panel_raised != nullptr) { bg_panel_raised->Release(); bg_panel_raised = nullptr; }
    if (bg_hover != nullptr) { bg_hover->Release(); bg_hover = nullptr; }
    if (bg_active != nullptr) { bg_active->Release(); bg_active = nullptr; }
    if (border_subtle != nullptr) { border_subtle->Release(); border_subtle = nullptr; }
    if (accent != nullptr) { accent->Release(); accent = nullptr; }
    if (text_primary != nullptr) { text_primary->Release(); text_primary = nullptr; }
    if (text_secondary != nullptr) { text_secondary->Release(); text_secondary = nullptr; }
    if (status_ok != nullptr) { status_ok->Release(); status_ok = nullptr; }
}
