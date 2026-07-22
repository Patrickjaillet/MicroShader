#include "win32_minimap.h"

#define NOMINMAX
#include <d2d1.h>

#include <algorithm>

#include "win32_theme_brushes.h"
#include "glsl_token_rules.h"
#include "glsl_syntax_colors.h"

bool minimap_should_render(int line_count, const MinimapSettings& settings)
{
    return settings.enabled && line_count >= settings.line_count_threshold;
}

void paint_minimap(ID2D1RenderTarget* render_target, ID2D1SolidColorBrush* dynamic_brush, const ThemeBrushes& brushes,
    const std::vector<std::wstring>& lines, float origin_x, float origin_y, float width, float height)
{
    D2D1_RECT_F bg_rect = D2D1::RectF(origin_x, origin_y, origin_x + width, origin_y + height);
    render_target->FillRectangle(bg_rect, brushes.bg_panel);

    if (lines.empty() || dynamic_brush == nullptr)
    {
        return;
    }

    float row_height = height / static_cast<float>(lines.size());
    row_height = std::clamp(row_height, 1.0f, 3.0f);

    bool in_block_comment = false;
    for (size_t line_index = 0; line_index < lines.size(); ++line_index)
    {
        float y = origin_y + static_cast<float>(line_index) * row_height;
        if (y > origin_y + height)
        {
            break;
        }

        float row_top = y + 0.5f;
        float row_bottom = y + std::max(1.0f, row_height - 0.5f);

        std::vector<GlslTokenSpan> spans = tokenize_glsl_line(lines[line_index], in_block_comment);
        float x = origin_x + 2.0f;
        for (const GlslTokenSpan& span : spans)
        {
            if (x >= origin_x + width - 2.0f)
            {
                break;
            }
            float token_width = std::max(1.0f, static_cast<float>(span.length));
            dynamic_brush->SetColor(glsl_syntax_color(span.kind));
            render_target->FillRectangle(D2D1::RectF(x, row_top, x + token_width, row_bottom), dynamic_brush);
            x += token_width + 0.5f;
        }
    }
}
