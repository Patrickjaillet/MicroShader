#pragma once

#include <string>

struct RawKeyChord
{
    std::string key_name;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};

std::string keybindings_file_path();
std::string find_string_field(const std::string& text, const std::string& key);
bool find_bool_field(const std::string& text, const std::string& key, bool default_value);
RawKeyChord find_raw_chord(const std::string& text, const std::string& prefix, const RawKeyChord& fallback);
void append_raw_chord_field(std::string& out, const char* prefix, const RawKeyChord& chord, bool trailing_comma);
