#pragma once

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;

struct ThemeBrushes
{
    ID2D1SolidColorBrush* bg_app = nullptr;
    ID2D1SolidColorBrush* bg_panel = nullptr;
    ID2D1SolidColorBrush* bg_panel_raised = nullptr;
    ID2D1SolidColorBrush* bg_hover = nullptr;
    ID2D1SolidColorBrush* bg_active = nullptr;
    ID2D1SolidColorBrush* border_subtle = nullptr;
    ID2D1SolidColorBrush* accent = nullptr;
    ID2D1SolidColorBrush* text_primary = nullptr;
    ID2D1SolidColorBrush* text_secondary = nullptr;
    ID2D1SolidColorBrush* status_ok = nullptr;

    bool create(ID2D1RenderTarget* render_target);
    void release();
};
