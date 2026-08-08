#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>
#include <vector>

#include "glsl_numeric_literals.h"

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

// A left-side inspector, mirroring the golf inspector on the right, that
// turns every float literal in the current shader source into a
// draggable slider -- e.g. dragging the "0.5" in `vec3(0.5)` rewrites that
// exact substring in place as you drag. The panel owns a working copy of
// the source text and keeps every row's byte offset correct across edits
// (splicing one literal's replacement text shifts every later literal's
// offset by the length delta), so the caller can pull `current_source()`
// out at any point -- typically throttled during a drag, and always once
// more on mouse-up -- and push it back into the real editor/compiler.
class Win32ValueSlidersPanel
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int x, int y, int width, int height);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    bool contains(int client_x, int client_y) const;

    // Rescans `source` for float literals and rebuilds every row from
    // scratch, discarding any in-progress drag. Call whenever the source
    // text changes for a reason other than this panel's own edits (tab
    // switch onto Source/Viewport, document load/switch, Run golf,
    // profile apply, twigl import, etc.) -- otherwise the panel's stored
    // byte offsets would silently drift out of sync with the real text.
    void sync_from_source(const std::string& source);

    void on_mouse_wheel(int wheel_delta);
    bool on_mouse_down(int client_x, int client_y);
    // Always updates the dragged slider's live value/thumb position (for
    // smooth visual feedback); returns true when the *committed* text
    // (current_source()) actually changed this call, which the caller
    // should treat as a signal to push it into the editor/recompile
    // (typically throttled -- see main_win32.cpp's slider-drag handling).
    bool on_mouse_move(int client_x, int client_y);
    void on_mouse_up();
    bool is_dragging() const { return dragging_index >= 0; }

    const std::string& current_source() const { return source_text; }

    // Test-support accessors (same rationale as other panels' equivalents):
    // expose otherwise-private row state so tests can assert on it without
    // a D2D render target.
    int row_count_value() const { return static_cast<int>(rows.size()); }
    double row_value_at(int index) const;

private:
    struct Row
    {
        GlslNumericLiteral literal;
        SliderRange range;
        std::string context;
    };

    IDWriteTextFormat* text_format = nullptr;
    IDWriteTextFormat* value_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    std::string source_text;
    std::vector<Row> rows;
    int dragging_index = -1;
    int scroll_offset_rows = 0;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;

    static constexpr float kRowHeight = 52.0f;
    static constexpr float kContextHeight = 18.0f;
    static constexpr float kSliderPadding = 10.0f;

    int visible_row_count() const;
    RECT slider_rect_for_visible_row(int visible_row) const;
    int row_at_client_y(int client_y) const;
    void set_value_from_x(int index, int client_x);
    void apply_value_to_row(int index, double new_value);
};
