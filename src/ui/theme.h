#pragma once

#include <string>

struct ImGuiIO;
struct ImFont;

extern ImFont* g_icon_font;

void apply_theme();
void load_fonts(ImGuiIO& io, float base_size);
