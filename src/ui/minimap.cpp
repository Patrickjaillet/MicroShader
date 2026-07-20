#include "minimap.h"

#include <algorithm>
#include <cctype>
#include <vector>

#include <imgui.h>

#include "glsl_language.h"
#include "theme_tokens.h"

namespace
{
    bool is_word_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
    }

    std::vector<std::string> split_source_lines(const std::string& source)
    {
        std::vector<std::string> lines;
        std::string current;
        for (char c : source)
        {
            if (c == '\n')
            {
                lines.push_back(current);
                current.clear();
            }
            else
            {
                current += c;
            }
        }
        lines.push_back(current);
        return lines;
    }
}

bool minimap_should_render(int line_count, const MinimapSettings& settings)
{
    return settings.enabled && line_count >= settings.line_count_threshold;
}

void render_minimap(const char* id, const std::string& source, const TextEditor::Palette& palette, float height, float width)
{
    std::vector<std::string> lines = split_source_lines(source);

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, ImVec2(width, height));
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), ImGui::GetColorU32(tokens::bg_field_sunken));

    if (lines.empty())
    {
        return;
    }

    float row_height = height / static_cast<float>(lines.size());
    row_height = std::clamp(row_height, 1.0f, 3.0f);

    for (std::size_t line_index = 0; line_index < lines.size(); ++line_index)
    {
        float y = origin.y + static_cast<float>(line_index) * row_height;
        if (y > origin.y + height)
        {
            break;
        }

        const std::string& line = lines[line_index];
        std::size_t i = 0;
        float x = origin.x + 2.0f;
        float row_top = y + 0.5f;
        float row_bottom = y + std::max(1.0f, row_height - 0.5f);

        while (i < line.size() && x < origin.x + width - 2.0f)
        {
            char c = line[i];
            if (std::isspace(static_cast<unsigned char>(c)) != 0)
            {
                ++i;
                x += 1.0f;
                continue;
            }

            std::string token;
            if (is_word_char(c))
            {
                while (i < line.size() && is_word_char(line[i]))
                {
                    token += line[i];
                    ++i;
                }
            }
            else
            {
                token += c;
                ++i;
            }

            TextEditor::PaletteIndex palette_index = classify_glsl_token(token);
            ImU32 color = palette[static_cast<int>(palette_index)];
            float token_width = std::max(1.0f, static_cast<float>(token.size()));
            draw_list->AddRectFilled(ImVec2(x, row_top), ImVec2(x + token_width, row_bottom), color);
            x += token_width + 0.5f;
        }
    }
}
