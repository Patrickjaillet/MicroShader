#include "keybindings.h"

#include <cstddef>

#include <imgui.h>

#include "theme_tokens.h"
#include "../platform/paths.h"

namespace
{
    std::string keybindings_file_path()
    {
        std::string dir = app_data_dir();
        if (dir.empty())
        {
            return std::string();
        }
        return dir + "keybindings.json";
    }

    ImGuiKey key_from_name(const std::string& name)
    {
        if (name.empty())
        {
            return ImGuiKey_None;
        }
        for (int key_index = ImGuiKey_NamedKey_BEGIN; key_index < ImGuiKey_NamedKey_END; ++key_index)
        {
            ImGuiKey key = static_cast<ImGuiKey>(key_index);
            const char* key_name = ImGui::GetKeyName(key);
            if (key_name != nullptr && name == key_name)
            {
                return key;
            }
        }
        return ImGuiKey_None;
    }

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

    KeyChord find_chord(const std::string& text, const std::string& prefix, const KeyChord& fallback)
    {
        std::string key_name = find_string_field(text, prefix + "_key");
        KeyChord chord = fallback;
        ImGuiKey parsed_key = key_from_name(key_name);
        if (parsed_key != ImGuiKey_None)
        {
            chord.key = parsed_key;
            chord.ctrl = find_bool_field(text, prefix + "_ctrl", fallback.ctrl);
            chord.shift = find_bool_field(text, prefix + "_shift", fallback.shift);
            chord.alt = find_bool_field(text, prefix + "_alt", fallback.alt);
        }
        return chord;
    }

    void append_chord_field(std::string& out, const char* prefix, const KeyChord& chord, bool trailing_comma)
    {
        const char* key_name = ImGui::GetKeyName(chord.key);
        out += "  \"";
        out += prefix;
        out += "_key\": \"";
        out += key_name != nullptr ? key_name : "";
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
}

Keybindings default_keybindings()
{
    Keybindings bindings;
    bindings.command_palette = {ImGuiKey_P, true, true, false};
    bindings.new_tab = {ImGuiKey_N, true, false, false};
    bindings.open_file = {ImGuiKey_O, true, false, false};
    bindings.save_file = {ImGuiKey_S, true, false, false};
    bindings.close_tab = {ImGuiKey_W, true, false, false};
    return bindings;
}

Keybindings load_keybindings()
{
    Keybindings fallback = default_keybindings();
    std::string file_path = keybindings_file_path();
    if (file_path.empty())
    {
        return fallback;
    }
    std::string text = read_utf8_file(file_path);
    if (text.find('{') == std::string::npos)
    {
        return fallback;
    }

    Keybindings bindings;
    bindings.command_palette = find_chord(text, "command_palette", fallback.command_palette);
    bindings.new_tab = find_chord(text, "new_tab", fallback.new_tab);
    bindings.open_file = find_chord(text, "open_file", fallback.open_file);
    bindings.save_file = find_chord(text, "save_file", fallback.save_file);
    bindings.close_tab = find_chord(text, "close_tab", fallback.close_tab);
    return bindings;
}

void save_keybindings(const Keybindings& bindings)
{
    std::string file_path = keybindings_file_path();
    if (file_path.empty())
    {
        return;
    }

    std::string out = "{\n";
    append_chord_field(out, "command_palette", bindings.command_palette, true);
    append_chord_field(out, "new_tab", bindings.new_tab, true);
    append_chord_field(out, "open_file", bindings.open_file, true);
    append_chord_field(out, "save_file", bindings.save_file, true);
    append_chord_field(out, "close_tab", bindings.close_tab, false);
    out += "}\n";
    write_utf8_file(file_path, out);
}

bool chord_pressed(const ImGuiIO& io, const KeyChord& chord)
{
    if (chord.key == ImGuiKey_None)
    {
        return false;
    }
    if (io.KeyCtrl != chord.ctrl || io.KeyShift != chord.shift || io.KeyAlt != chord.alt)
    {
        return false;
    }
    return ImGui::IsKeyPressed(chord.key);
}

std::string chord_label(const KeyChord& chord)
{
    if (chord.key == ImGuiKey_None)
    {
        return "Unbound";
    }
    std::string label;
    if (chord.ctrl)
    {
        label += "Ctrl+";
    }
    if (chord.shift)
    {
        label += "Shift+";
    }
    if (chord.alt)
    {
        label += "Alt+";
    }
    const char* key_name = ImGui::GetKeyName(chord.key);
    label += key_name != nullptr ? key_name : "?";
    return label;
}

namespace
{
    struct BindableAction
    {
        const char* label;
        KeyChord* chord;
    };
}

void render_keybindings_panel(Keybindings& bindings)
{
    static int listening_index = -1;

    BindableAction actions[] = {
        {"Command palette", &bindings.command_palette},
        {"New tab", &bindings.new_tab},
        {"Open file", &bindings.open_file},
        {"Save", &bindings.save_file},
        {"Close tab", &bindings.close_tab},
    };

    ImGuiIO& io = ImGui::GetIO();

    for (int i = 0; i < static_cast<int>(sizeof(actions) / sizeof(actions[0])); ++i)
    {
        ImGui::TextColored(tokens::text_primary, "%s", actions[i].label);
        ImGui::SameLine(200.0f);

        if (listening_index == i)
        {
            ImGui::TextColored(tokens::accent, "Press a key...");
            for (int key_index = ImGuiKey_NamedKey_BEGIN; key_index < ImGuiKey_NamedKey_END; ++key_index)
            {
                ImGuiKey key = static_cast<ImGuiKey>(key_index);
                if (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl ||
                    key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift ||
                    key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt ||
                    key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper ||
                    key == ImGuiMod_Ctrl || key == ImGuiMod_Shift ||
                    key == ImGuiMod_Alt || key == ImGuiMod_Super)
                {
                    continue;
                }
                if (ImGui::IsKeyPressed(key))
                {
                    actions[i].chord->key = key;
                    actions[i].chord->ctrl = io.KeyCtrl;
                    actions[i].chord->shift = io.KeyShift;
                    actions[i].chord->alt = io.KeyAlt;
                    save_keybindings(bindings);
                    listening_index = -1;
                    break;
                }
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Cancel"))
            {
                listening_index = -1;
            }
        }
        else
        {
            ImGui::TextColored(tokens::text_secondary, "%s", chord_label(*actions[i].chord).c_str());
            ImGui::SameLine();
            std::string button_id = std::string("Rebind##") + actions[i].label;
            if (ImGui::SmallButton(button_id.c_str()))
            {
                listening_index = i;
            }
        }
    }

    ImGui::Spacing();
    if (ImGui::Button("Reset to defaults"))
    {
        bindings = default_keybindings();
        save_keybindings(bindings);
        listening_index = -1;
    }
}
