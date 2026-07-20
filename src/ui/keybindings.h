#pragma once

#include <string>

#include <imgui.h>

struct KeyChord
{
    ImGuiKey key = ImGuiKey_None;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};

struct Keybindings
{
    KeyChord command_palette;
    KeyChord new_tab;
    KeyChord open_file;
    KeyChord save_file;
    KeyChord close_tab;
};

Keybindings default_keybindings();
Keybindings load_keybindings();
void save_keybindings(const Keybindings& bindings);
bool chord_pressed(const ImGuiIO& io, const KeyChord& chord);
std::string chord_label(const KeyChord& chord);
void render_keybindings_panel(Keybindings& bindings);
