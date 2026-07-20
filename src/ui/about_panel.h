#pragma once

struct Texture;
struct Keybindings;

void render_about_popup(bool& show_about, const Texture& logo, Keybindings& bindings, float& ui_font_size);
