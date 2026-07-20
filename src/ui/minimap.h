#pragma once

#include <string>

#include <TextEditor.h>

struct MinimapSettings
{
    bool enabled = false;
    int line_count_threshold = 50;
    float width = 96.0f;
};

bool minimap_should_render(int line_count, const MinimapSettings& settings);
void render_minimap(const char* id, const std::string& source, const TextEditor::Palette& palette, float height, float width);
