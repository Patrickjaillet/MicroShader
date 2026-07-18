#pragma once

#include <string>

struct ImGuiIO;
struct ImFont;

extern ImFont* g_icon_font;
extern ImFont* g_mono_font;

void apply_theme();
void load_fonts(ImGuiIO& io, float base_size);
bool themed_checkbox(const char* label, bool* value);
