#include "keybindings_storage.h"

#include "../platform/paths.h"

namespace
{
    std::string find_field_slice(const std::string& text, const std::string& key)
    {
        std::string needle = "\"" + key + "\"";
        std::size_t key_pos = text.find(needle);
        if (key_pos == std::string::npos)
        {
            return std::string();
        }
        std::size_t colon_pos = text.find(':', key_pos + needle.size());
        if (colon_pos == std::string::npos)
        {
            return std::string();
        }
        std::size_t value_start = colon_pos + 1;
        while (value_start < text.size() && (text[value_start] == ' ' || text[value_start] == '\t'))
        {
            ++value_start;
        }
        std::size_t value_end = text.find_first_of(",}\n", value_start);
        if (value_end == std::string::npos)
        {
            value_end = text.size();
        }
        return text.substr(value_start, value_end - value_start);
    }
}

std::string keybindings_file_path()
{
    std::string dir = app_data_dir();
    if (dir.empty())
    {
        return std::string();
    }
    return dir + "keybindings.json";
}

bool find_bool_field(const std::string& text, const std::string& key, bool default_value)
{
    std::string slice = find_field_slice(text, key);
    if (slice.find("true") != std::string::npos)
    {
        return true;
    }
    if (slice.find("false") != std::string::npos)
    {
        return false;
    }
    return default_value;
}

std::string find_string_field(const std::string& text, const std::string& key)
{
    std::string needle = "\"" + key + "\"";
    std::size_t key_pos = text.find(needle);
    if (key_pos == std::string::npos)
    {
        return std::string();
    }
    std::size_t colon_pos = text.find(':', key_pos + needle.size());
    if (colon_pos == std::string::npos)
    {
        return std::string();
    }
    std::size_t quote_start = text.find('"', colon_pos + 1);
    if (quote_start == std::string::npos)
    {
        return std::string();
    }
    std::size_t quote_end = text.find('"', quote_start + 1);
    if (quote_end == std::string::npos)
    {
        return std::string();
    }
    return text.substr(quote_start + 1, quote_end - quote_start - 1);
}

RawKeyChord find_raw_chord(const std::string& text, const std::string& prefix, const RawKeyChord& fallback)
{
    std::string key_name = find_string_field(text, prefix + "_key");
    RawKeyChord chord = fallback;
    if (!key_name.empty())
    {
        chord.key_name = key_name;
        chord.ctrl = find_bool_field(text, prefix + "_ctrl", fallback.ctrl);
        chord.shift = find_bool_field(text, prefix + "_shift", fallback.shift);
        chord.alt = find_bool_field(text, prefix + "_alt", fallback.alt);
    }
    return chord;
}

void append_raw_chord_field(std::string& out, const char* prefix, const RawKeyChord& chord, bool trailing_comma)
{
    out += "  \"";
    out += prefix;
    out += "_key\": \"";
    out += chord.key_name;
    out += "\",\n";
    out += "  \"";
    out += prefix;
    out += "_ctrl\": ";
    out += chord.ctrl ? "true" : "false";
    out += ",\n";
    out += "  \"";
    out += prefix;
    out += "_shift\": ";
    out += chord.shift ? "true" : "false";
    out += ",\n";
    out += "  \"";
    out += prefix;
    out += "_alt\": ";
    out += chord.alt ? "true" : "false";
    out += trailing_comma ? ",\n" : "\n";
}
