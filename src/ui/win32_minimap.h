#pragma once

#include <string>
#include <vector>

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct ThemeBrushes;

struct MinimapSettings
{
    bool enabled = true;
    int line_count_threshold = 50;
    float width = 96.0f;
};

bool minimap_should_render(int line_count, const MinimapSettings& settings);

void paint_minimap(ID2D1RenderTarget* render_target, ID2D1SolidColorBrush* dynamic_brush, const ThemeBrushes& brushes,
    const std::vector<std::wstring>& lines, float origin_x, float origin_y, float width, float height);
