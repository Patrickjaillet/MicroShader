#pragma once

#include <cstdint>
#include <string>

#include "win32_tool_button.h"
#include "win32_text_editor.h"
#include "ushader/golf_core.h"

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

class Win32TwiglExportPanel
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int x, int y, int width, int height);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;
    void tick();

    bool contains(int client_x, int client_y) const;
    bool on_mouse_down(int client_x, int client_y);
    void on_mouse_wheel(int wheel_delta);

    void set_golfed_source(const std::string& golfed_source);

    bool take_pending_snippet_insert(std::string& out_source);

private:
    static constexpr int kModeCount = 4;
    static constexpr int kSnippetCount = 10;

    Win32ToolButton mode_buttons[kModeCount];
    Win32ToolButton es300_button;
    Win32ToolButton mrt_button;
    Win32ToolButton backbuffer_button;
    Win32ToolButton sound_button;
    Win32ToolButton snippet_buttons[kSnippetCount];
    Win32TextEditor preview_editor;

    IDWriteTextFormat* label_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;

    int32_t mode = 0;
    bool es300 = false;
    uint8_t mrt_targets = 1;
    bool has_backbuffer = false;
    bool has_sound = false;

    std::string golfed_source_cache;
    UshaderBudgetResult last_budget{};

    bool has_pending_snippet_insert = false;
    std::string pending_snippet_insert_source;

    void recompute_preview();
};
