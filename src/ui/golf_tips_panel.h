#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

// golf.md Phase 33.1 / ROADMAP.md Phase 35-36 -- a read-only, searchable
// reference panel of well-known manual golfing idioms (short
// rotation-matrix constants, cheap trigonometric/geometric identity
// substitutions, compact hash/noise one-liners, SDF primitives/operators,
// cosine palettes, tonemap/gamma one-liners), each tagged with its source
// lineage (Fabrice Neyret / Inigo Quilez / Xor) so the existing search
// field also acts as a lineage filter (ROADMAP.md Phase 36.5). Per the
// Phase 11 invariant this whole document is subordinate to, nothing here
// is ever applied to the editor *without* an explicit per-entry click:
// "Copy snippet" copies to the clipboard, "Insert" inserts at the caret
// in the Source editor (ROADMAP.md Phase 36.1, mirroring the Phase 34.4
// Twigl snippet-insertion UX) -- never automatically, never on load.
class Win32GolfTipsPanel
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int x, int y, int width, int height);
    bool on_mouse_down(int client_x, int client_y);
    void on_mouse_move(int client_x, int client_y);
    void on_mouse_wheel(int wheel_delta);
    bool on_char(wchar_t character);
    bool on_key_down(WPARAM key);
    void set_field_focus(bool focused);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    bool contains(int client_x, int client_y) const;

    // Focuses the search field and pre-fills it, used by the Phase 33.2
    // "Explain Golf" trace-view cross-reference hint so the user lands on
    // a relevant subset of entries instead of the full catalogue.
    void focus_search_with_query(const std::string& query);

    // ROADMAP.md Phase 36.1: consumed once by main_win32.cpp after
    // on_mouse_down, exactly mirroring
    // Win32TwiglExportPanel::take_pending_snippet_insert.
    bool take_pending_snippet_insert(std::string& out_source);

private:
    IDWriteTextFormat* text_format = nullptr;
    IDWriteTextFormat* header_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    std::string search_text;
    bool field_focused = false;
    int scroll_top_row = 0;
    int hovered_copy_row = -1;
    int hovered_insert_row = -1;
    bool has_pending_snippet_insert = false;
    std::string pending_snippet_insert_source;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;

    static constexpr float kFieldHeight = 26.0f;
    static constexpr float kRowHeight = 58.0f;
    static constexpr float kCopyButtonWidth = 64.0f;
    static constexpr float kCopyButtonHeight = 22.0f;
    static constexpr float kInsertButtonWidth = 64.0f;

    RECT field_rect() const;
    RECT list_rect() const;
    RECT row_rect(int visible_index) const;
    RECT copy_button_rect(int visible_index) const;
    RECT insert_button_rect(int visible_index) const;

    struct VisibleEntries
    {
        int indices[48];
        int count = 0;
    };
    VisibleEntries filtered_entries() const;

    void copy_entry_to_clipboard(int catalogue_index) const;
};
