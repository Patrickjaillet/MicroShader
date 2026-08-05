#pragma once

#include "win32_animation.h"

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteTextFormat;
struct IDWriteFactory;
struct ThemeBrushes;

class TabStrip
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int origin_x, int origin_y, int window_width);
    int hit_test(int x, int y) const;
    void set_hover(int index);
    void set_focused(bool focused_value);
    void switch_to(int index);
    int active_index() const { return active; }
    void tick();
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

    static constexpr float kHeight = 32.0f;
    static constexpr int kTabCount = 10;
    int origin_y() const { return top; }

    // Looks up a tab's index by its name (matching the label passed to
    // "Switch to tab: ..." command-palette actions, e.g. "Twigl"), so
    // callers that need a specific tab's index don't have to hardcode a
    // number that has to stay in sync with this class's own kTabLabels/
    // kTabNames array purely by convention (see roadmap.md P2 point 7 --
    // this exact class of bug already shipped twice for the Twigl tab).
    // Returns -1 if no tab has that name.
    static int index_of(const char* name);

private:
    static constexpr float kTabWidth = 120.0f;
    static constexpr float kCornerRadius = 2.0f;

    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;
    int left = 0;
    int top = 0;
    int hovered_index = -1;
    int active = 0;
    bool focused = false;

    AnimatedColor hover_anim[kTabCount];
    AnimatedColor open_anim;
};
