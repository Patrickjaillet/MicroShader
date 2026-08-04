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

// ROADMAP.md/roadmap_twigl.md Phase 43.3 -- the comma-separated list of
// twigl snippet names (see Win32TwiglExportPanel's kSnippetNames), for
// callers that need to always reserve them from the golfer's generated-name
// space so a snippet inserted later can never collide with an
// already-golfed identifier of the same short name. Deliberately scoped to
// just the snippet names (all 2+ characters, so the cost to the golfer's
// available short-name space is negligible) rather than also reserving
// twigl's single-character uniform names (r/m/t/f/b/s/o) -- doing that too
// would measurably shrink the 1-character name space for every shader,
// including the majority that never touch the Twigl export feature at all,
// which is a worse trade-off than the collision risk it would close. See
// roadmap_twigl.md Phase 43.3 for the full reasoning.
std::string twigl_reserved_snippet_names_csv();

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

    // ROADMAP.md/roadmap_twigl.md Phase 43.2 -- when the "Import twigl
    // shader" button is clicked, returns the un-rewritten (Shadertoy-style)
    // reconstruction of whatever is currently in this panel's preview text,
    // for the caller to paste into the Source editor. Mirrors
    // take_pending_snippet_insert's own poll-once pattern.
    bool take_pending_import(std::string& out_source);

    // ROADMAP.md/roadmap_twigl.md Phase 43.1 -- when the "Copy for
    // twigl.app" button is clicked, returns the exact export text
    // (mode/ES300/MRT/backbuffer/sound, via compute_export_text()) for the
    // caller to write to the clipboard and confirm with a toast.
    bool take_pending_copy_request(std::string& out_text);

    // ROADMAP.md Phase 38.3 -- read by the "Copy as twigl (mode: ...)"
    // export preset action so it matches whatever mode is currently
    // selected in this panel's segmented control.
    int32_t current_mode() const { return mode; }
    bool current_es300() const { return es300; }
    // ROADMAP.md/roadmap_twigl.md Phase 42.4 -- these three were previously
    // unreadable from outside the panel, which is exactly why
    // copy_as_twigl_action() in main_win32.cpp could silently diverge from
    // what the panel's own preview showed. Read by
    // compute_export_text() below and by the clipboard action.
    uint8_t current_mrt_targets() const { return mrt_targets; }
    bool current_has_backbuffer() const { return has_backbuffer; }
    bool current_has_sound() const { return has_sound; }

    // ROADMAP.md/roadmap_twigl.md Phase 44.3 -- restores this panel's
    // toggle state (e.g. from a WorkspaceDocument/EditorDocument on
    // document switch or session restore) and recomputes the preview so
    // the on-screen state matches immediately, exactly as if the user had
    // clicked each toggle themselves.
    void restore_state(int32_t restored_mode, bool restored_es300, uint8_t restored_mrt_targets,
        bool restored_has_backbuffer, bool restored_has_sound);

    // Recomputes the full twigl export text for arbitrary source text using
    // this panel's *current* toggle state (mode/es300/MRT/backbuffer/sound),
    // via the single ushader_twigl_rewrite_full FFI entry point. The live
    // preview and the "Copy for twigl.app" clipboard action both go through
    // this one function so they are byte-identical by construction --
    // closes ROADMAP.md/roadmap_twigl.md Phase 42.3 and 42.4 together.
    std::string compute_export_text(const std::string& golfed_source) const;

    // ROADMAP.md/roadmap_twigl.md Phase 43.2 -- the current preview text,
    // for the command-palette "Import twigl shader" action
    // (main_win32.cpp), which needs the same source the panel's own
    // "Import" button reads from but is reachable without clicking into
    // the panel first.
    std::string preview_text_for_import() const;

private:
    static constexpr int kModeCount = 4;
    static constexpr int kSnippetCount = 10;

    Win32ToolButton mode_buttons[kModeCount];
    Win32ToolButton es300_button;
    Win32ToolButton mrt_button;
    Win32ToolButton backbuffer_button;
    Win32ToolButton sound_button;
    // ROADMAP.md/roadmap_twigl.md Phase 43.1/43.2 -- the guaranteed-correct
    // "Copy for twigl.app" action and the round-trip "Import twigl shader"
    // action, both also reachable from the command palette (main_win32.cpp),
    // routed through this panel's own compute_export_text()/
    // ushader_twigl_unrewrite so every entry point stays byte-identical.
    Win32ToolButton copy_button;
    Win32ToolButton import_button;
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

    bool has_pending_import = false;
    std::string pending_import_source;

    bool has_pending_copy_request = false;

    void recompute_preview();
};
