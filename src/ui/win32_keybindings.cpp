#include "win32_keybindings.h"
#include "keybindings_storage.h"

#include "../platform/paths.h"

namespace
{
    struct VkNameEntry
    {
        WORD vk;
        const char* name;
    };

    const VkNameEntry kVkNames[] = {
        { VK_TAB, "Tab" }, { VK_LEFT, "LeftArrow" }, { VK_RIGHT, "RightArrow" },
        { VK_UP, "UpArrow" }, { VK_DOWN, "DownArrow" }, { VK_PRIOR, "PageUp" }, { VK_NEXT, "PageDown" },
        { VK_HOME, "Home" }, { VK_END, "End" }, { VK_INSERT, "Insert" }, { VK_DELETE, "Delete" },
        { VK_BACK, "Backspace" }, { VK_SPACE, "Space" }, { VK_RETURN, "Enter" }, { VK_ESCAPE, "Escape" },
        { VK_F1, "F1" }, { VK_F2, "F2" }, { VK_F3, "F3" }, { VK_F4, "F4" }, { VK_F5, "F5" }, { VK_F6, "F6" },
        { VK_F7, "F7" }, { VK_F8, "F8" }, { VK_F9, "F9" }, { VK_F10, "F10" }, { VK_F11, "F11" }, { VK_F12, "F12" },
    };

    std::string vk_to_name(WORD vk)
    {
        if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z'))
        {
            return std::string(1, static_cast<char>(vk));
        }
        for (const VkNameEntry& entry : kVkNames)
        {
            if (entry.vk == vk)
            {
                return entry.name;
            }
        }
        return std::string();
    }

    WORD name_to_vk(const std::string& name)
    {
        if (name.size() == 1)
        {
            char c = name[0];
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z'))
            {
                return static_cast<WORD>(c);
            }
        }
        for (const VkNameEntry& entry : kVkNames)
        {
            if (name == entry.name)
            {
                return entry.vk;
            }
        }
        return 0;
    }

    Win32KeyChord load_one(const std::string& text, const char* prefix, const Win32KeyChord& fallback)
    {
        RawKeyChord raw_fallback{ vk_to_name(fallback.vk), fallback.ctrl, fallback.shift, fallback.alt };
        RawKeyChord raw = find_raw_chord(text, prefix, raw_fallback);
        WORD vk = name_to_vk(raw.key_name);
        if (vk == 0)
        {
            return fallback;
        }
        return Win32KeyChord{ vk, raw.ctrl, raw.shift, raw.alt };
    }

    void save_one(std::string& out, const char* prefix, const Win32KeyChord& chord, bool trailing_comma)
    {
        RawKeyChord raw{ vk_to_name(chord.vk), chord.ctrl, chord.shift, chord.alt };
        append_raw_chord_field(out, prefix, raw, trailing_comma);
    }
}

Win32Keybindings default_win32_keybindings()
{
    Win32Keybindings bindings;
    bindings.command_palette = Win32KeyChord{ 'P', true, true, false };
    bindings.new_tab = Win32KeyChord{ 'N', true, false, false };
    bindings.open_file = Win32KeyChord{ 'O', true, false, false };
    bindings.save_file = Win32KeyChord{ 'S', true, false, false };
    bindings.close_tab = Win32KeyChord{ 'W', true, false, false };
    return bindings;
}

Win32Keybindings load_win32_keybindings()
{
    Win32Keybindings fallback = default_win32_keybindings();
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

    Win32Keybindings bindings;
    bindings.command_palette = load_one(text, "command_palette", fallback.command_palette);
    bindings.new_tab = load_one(text, "new_tab", fallback.new_tab);
    bindings.open_file = load_one(text, "open_file", fallback.open_file);
    bindings.save_file = load_one(text, "save_file", fallback.save_file);
    bindings.close_tab = load_one(text, "close_tab", fallback.close_tab);
    return bindings;
}

void save_win32_keybindings(const Win32Keybindings& bindings)
{
    std::string file_path = keybindings_file_path();
    if (file_path.empty())
    {
        return;
    }

    std::string out = "{\n";
    save_one(out, "command_palette", bindings.command_palette, true);
    save_one(out, "new_tab", bindings.new_tab, true);
    save_one(out, "open_file", bindings.open_file, true);
    save_one(out, "save_file", bindings.save_file, true);
    save_one(out, "close_tab", bindings.close_tab, false);
    out += "}\n";
    write_utf8_file(file_path, out);
}

bool win32_chord_matches(const Win32KeyChord& chord, WPARAM key, bool ctrl, bool shift, bool alt)
{
    if (chord.vk == 0)
    {
        return false;
    }
    return static_cast<WORD>(key) == chord.vk && ctrl == chord.ctrl && shift == chord.shift && alt == chord.alt;
}

std::string win32_chord_label(const Win32KeyChord& chord)
{
    if (chord.vk == 0)
    {
        return "Unbound";
    }
    std::string label;
    if (chord.ctrl) { label += "Ctrl+"; }
    if (chord.shift) { label += "Shift+"; }
    if (chord.alt) { label += "Alt+"; }
    std::string name = vk_to_name(chord.vk);
    label += name.empty() ? "?" : name;
    return label;
}
