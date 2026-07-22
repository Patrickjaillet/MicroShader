#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

struct Win32KeyChord
{
    WORD vk = 0;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};

struct Win32Keybindings
{
    Win32KeyChord command_palette;
    Win32KeyChord new_tab;
    Win32KeyChord open_file;
    Win32KeyChord save_file;
    Win32KeyChord close_tab;
};

Win32Keybindings default_win32_keybindings();
Win32Keybindings load_win32_keybindings();
void save_win32_keybindings(const Win32Keybindings& bindings);
bool win32_chord_matches(const Win32KeyChord& chord, WPARAM key, bool ctrl, bool shift, bool alt);
std::string win32_chord_label(const Win32KeyChord& chord);
