#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

#include "golf_controls.h"

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

class Win32GolfControls
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int x, int y, int width, int height);
    bool on_mouse_down(int client_x, int client_y);
    bool on_char(wchar_t character);
    bool on_key_down(WPARAM key);
    void set_field_focus(bool focused);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    bool contains(int client_x, int client_y) const;

    const GolfPassToggles& toggles() const { return golf_toggles; }
    const std::string& protected_names() const { return protected_names_text; }
    int budget_preset_index() const { return budget_index; }

private:
    struct CheckboxRow
    {
        bool* value = nullptr;
        const wchar_t* label = nullptr;
    };

    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    GolfPassToggles golf_toggles;
    std::string protected_names_text;
    int budget_index = -1;
    bool field_focused = false;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;

    static constexpr float kRowHeight = 24.0f;
    static constexpr float kCheckboxSize = 14.0f;

    RECT aggressive_rect() const;
    RECT preset_rect() const;
    RECT field_rect() const;
    RECT checkbox_hit_rect(int index) const;
};
