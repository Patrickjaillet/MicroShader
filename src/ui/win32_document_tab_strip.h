#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>
#include <vector>

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

// ROADMAP.md Phase 38.2 -- an always-visible strip of open documents,
// rendered above the Author/Analyze/Export/Settings tab groups
// (win32_tab_strip.h). Purely a display+hit-testing widget: main_win32.cpp
// owns the actual document list (source/golfed text, file path, per-
// document golf profile) and decides what switching/closing/adding a
// document means; this class only knows display names and which index is
// active.
class Win32DocumentTabStrip
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int x, int y, int width);
    void set_documents(const std::vector<std::string>& display_names, int active_index, int dirty_mask);

    enum class HitKind
    {
        None,
        Document,
        Close,
        NewDocument
    };

    struct HitResult
    {
        HitKind kind = HitKind::None;
        int index = -1;
    };

    HitResult hit_test(int x, int y) const;
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    static constexpr float kHeight = 30.0f;

private:
    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    std::vector<std::string> names;
    int active = 0;
    // Bit i set means document i has unsaved changes (shown as a dot
    // instead of a close "x" is out of scope here -- kept as a leading
    // marker glyph in the label instead, simplest correctness-preserving
    // signal that still satisfies Phase 38.8's "state exposure" via the
    // accessibility name below).
    int dirty_mask = 0;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;

    static constexpr float kTabMinWidth = 90.0f;
    static constexpr float kTabMaxWidth = 200.0f;
    static constexpr float kCloseWidth = 20.0f;
    static constexpr float kNewButtonWidth = 30.0f;

    float tab_width() const;
    RECT tab_rect(int index) const;
    RECT close_rect(int index) const;
    RECT new_button_rect() const;
};
