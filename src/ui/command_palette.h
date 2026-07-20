#pragma once

#include <functional>
#include <string>
#include <vector>

struct PaletteCommand
{
    std::string label;
    std::function<void()> execute;
};

void render_command_palette(bool& open, const std::vector<PaletteCommand>& commands);
