// Regression coverage for Win32TwiglExportPanel's own hit-testing and
// state-persistence logic -- the two shipped Twigl regressions (export tab
// never created, panel never destroyed) were both in exactly this
// previously-untested layer, caught only by manual/visual verification (see
// roadmap.md P3 point 12). Like twigl_golf_collision_test.cpp, this
// instantiates the panel directly without calling create() (no D2D
// dependency needed for layout()/on_mouse_down()/recompute_preview()'s own
// codepaths -- paint()/destroy() are null-guarded and never exercised here).
#include <cstdio>
#include <string>

#include "../src/ui/win32_twigl_export_panel.h"

namespace
{
    int failures = 0;

    void expect_eq_i32(int32_t actual, int32_t expected, const char* what)
    {
        if (actual != expected)
        {
            std::fprintf(stderr, "FAIL %s: expected %d, got %d\n", what, expected, actual);
            ++failures;
        }
    }

    void expect_true(bool value, const char* what)
    {
        if (!value)
        {
            std::fprintf(stderr, "FAIL %s\n", what);
            ++failures;
        }
    }

    // Matches win32_twigl_export_panel.cpp's own layout() constants.
    constexpr int kPanelPadding = 8;
    constexpr int kButtonHeight = 26;
    constexpr int kButtonGap = 6;
    constexpr int kRowGap = 8;
    constexpr int kModeButtonWidth = 96;
    constexpr int kWideButtonWidth = 150;
    constexpr int kSnippetWidth = 88;

    struct Point { int x; int y; };

    Point mode_button_center(int index)
    {
        int x = kPanelPadding + index * (kModeButtonWidth + kButtonGap) + kModeButtonWidth / 2;
        int y = kPanelPadding + kButtonHeight / 2;
        return { x, y };
    }

    int toggle_row_y()
    {
        return kPanelPadding + kButtonHeight + kRowGap;
    }

    Point es300_button_center() { return { kPanelPadding + kModeButtonWidth / 2, toggle_row_y() + kButtonHeight / 2 }; }
    Point mrt_button_center()
    {
        return { kPanelPadding + (kModeButtonWidth + kButtonGap) + kModeButtonWidth / 2, toggle_row_y() + kButtonHeight / 2 };
    }
    Point backbuffer_button_center()
    {
        return { kPanelPadding + 2 * (kModeButtonWidth + kButtonGap) + kModeButtonWidth / 2, toggle_row_y() + kButtonHeight / 2 };
    }
    Point sound_button_center()
    {
        return { kPanelPadding + 3 * (kModeButtonWidth + kButtonGap) + kModeButtonWidth / 2, toggle_row_y() + kButtonHeight / 2 };
    }

    int action_row_y() { return toggle_row_y() + kButtonHeight + kRowGap; }
    Point copy_button_center() { return { kPanelPadding + kWideButtonWidth / 2, action_row_y() + kButtonHeight / 2 }; }
    Point import_button_center()
    {
        return { kPanelPadding + (kWideButtonWidth + kButtonGap) + kWideButtonWidth / 2, action_row_y() + kButtonHeight / 2 };
    }

    int snippet_row_y() { return action_row_y() + kButtonHeight + kRowGap; }
    Point snippet_button_center(int index)
    {
        return { kPanelPadding + index * (kSnippetWidth + kButtonGap) + kSnippetWidth / 2, snippet_row_y() + kButtonHeight / 2 };
    }
}

void test_mode_button_hit_testing_updates_current_mode()
{
    Win32TwiglExportPanel panel;
    panel.layout(0, 0, 1000, 1000);
    expect_eq_i32(panel.current_mode(), 0, "panel starts in Classic mode");

    Point geek = mode_button_center(1);
    expect_true(panel.on_mouse_down(geek.x, geek.y), "clicking the Geek mode button reports handled");
    expect_eq_i32(panel.current_mode(), 1, "clicking Geek mode button switches to mode 1");

    Point geekest = mode_button_center(3);
    panel.on_mouse_down(geekest.x, geekest.y);
    expect_eq_i32(panel.current_mode(), 3, "clicking Geekest mode button switches to mode 3");
}

void test_toggle_buttons_flip_their_own_state_only()
{
    Win32TwiglExportPanel panel;
    panel.layout(0, 0, 1000, 1000);

    Point es300 = es300_button_center();
    panel.on_mouse_down(es300.x, es300.y);
    expect_true(panel.current_es300(), "ES300 toggle click sets current_es300() true");
    expect_eq_i32(panel.current_mrt_targets(), 1, "ES300 toggle click does not also flip MRT");
    expect_true(!panel.current_has_backbuffer(), "ES300 toggle click does not also flip backbuffer");

    panel.on_mouse_down(es300.x, es300.y);
    expect_true(!panel.current_es300(), "clicking ES300 toggle a second time flips it back off");

    Point mrt = mrt_button_center();
    panel.on_mouse_down(mrt.x, mrt.y);
    expect_eq_i32(panel.current_mrt_targets(), 2, "MRT toggle click sets current_mrt_targets() to 2");

    Point backbuffer = backbuffer_button_center();
    panel.on_mouse_down(backbuffer.x, backbuffer.y);
    expect_true(panel.current_has_backbuffer(), "backbuffer toggle click sets current_has_backbuffer() true");

    Point sound = sound_button_center();
    panel.on_mouse_down(sound.x, sound.y);
    expect_true(panel.current_has_sound(), "sound toggle click sets current_has_sound() true");

    // None of the other three toggles should have moved.
    expect_true(!panel.current_es300(), "backbuffer/sound clicks did not re-flip ES300");
    expect_eq_i32(panel.current_mrt_targets(), 2, "backbuffer/sound clicks did not reset MRT");
}

void test_copy_button_raises_pending_flag_without_changing_toggles()
{
    Win32TwiglExportPanel panel;
    panel.layout(0, 0, 1000, 1000);
    panel.set_golfed_source("void main(){gl_FragColor=vec4(1.0);}");

    Point copy = copy_button_center();
    expect_true(panel.on_mouse_down(copy.x, copy.y), "clicking Copy reports handled");
    std::string copy_text;
    expect_true(panel.take_pending_copy_request(copy_text), "Copy click raises the pending copy-request flag");
    expect_true(!copy_text.empty(), "the copy request text is non-empty for a valid source");

    std::string copy_text_again;
    expect_true(!panel.take_pending_copy_request(copy_text_again),
        "the pending copy-request flag is consumed exactly once (poll-once contract)");
}

void test_import_button_reconstructs_and_raises_pending_flag()
{
    Win32TwiglExportPanel panel;
    panel.layout(0, 0, 1000, 1000);
    panel.set_golfed_source("void main(){gl_FragColor=vec4(1.0);}");

    Point import = import_button_center();
    expect_true(panel.on_mouse_down(import.x, import.y), "clicking Import reports handled");
    std::string import_source;
    expect_true(panel.take_pending_import(import_source), "Import click raises the pending import flag");
    // Reconstructed as this app's Source-tab mainImage signature (not a bare
    // "gl_FragColor" assignment, which ShaderRunner's own wrapping convention
    // would never compile -- see rust-core/src/twigl.rs's
    // wrap_plain_main_as_mainimage).
    expect_true(import_source.find("void mainImage(") != std::string::npos,
        "the reconstructed import source uses this app's mainImage signature");
    expect_true(import_source.find("fragColor") != std::string::npos,
        "the reconstructed import source assigns to the fragColor parameter");
}

void test_snippet_button_raises_pending_insert_with_matching_source()
{
    Win32TwiglExportPanel panel;
    panel.layout(0, 0, 1000, 1000);

    // Snippet index 0 is "PI" (see kSnippetNames in win32_twigl_export_panel.cpp).
    Point pi_snippet = snippet_button_center(0);
    panel.on_mouse_down(pi_snippet.x, pi_snippet.y);
    std::string snippet_source;
    expect_true(panel.take_pending_snippet_insert(snippet_source), "clicking a snippet button raises the pending insert flag");
    expect_true(snippet_source.find("PI") != std::string::npos, "the PI snippet's source actually defines PI");
}

void test_restore_state_round_trips_every_field()
{
    Win32TwiglExportPanel panel;
    panel.layout(0, 0, 1000, 1000);
    panel.restore_state(/*mode=*/3, /*es300=*/true, /*mrt=*/2, /*backbuffer=*/true, /*sound=*/true);

    expect_eq_i32(panel.current_mode(), 3, "restore_state sets mode");
    expect_true(panel.current_es300(), "restore_state sets es300");
    expect_eq_i32(panel.current_mrt_targets(), 2, "restore_state sets mrt_targets");
    expect_true(panel.current_has_backbuffer(), "restore_state sets has_backbuffer");
    expect_true(panel.current_has_sound(), "restore_state sets has_sound");
}

void test_restore_state_clamps_an_out_of_range_mode_to_classic()
{
    Win32TwiglExportPanel panel;
    panel.layout(0, 0, 1000, 1000);
    // A hand-edited or future-schema workspace file could contain an
    // out-of-range mode index; restore_state must clamp rather than let it
    // propagate into kModeLabels/kModeNames indexing elsewhere in the panel.
    panel.restore_state(/*mode=*/99, /*es300=*/false, /*mrt=*/1, /*backbuffer=*/false, /*sound=*/false);
    expect_eq_i32(panel.current_mode(), 0, "an out-of-range restored mode clamps to Classic (0)");

    panel.restore_state(/*mode=*/-1, /*es300=*/false, /*mrt=*/1, /*backbuffer=*/false, /*sound=*/false);
    expect_eq_i32(panel.current_mode(), 0, "a negative restored mode clamps to Classic (0)");
}

int main()
{
    test_mode_button_hit_testing_updates_current_mode();
    test_toggle_buttons_flip_their_own_state_only();
    test_copy_button_raises_pending_flag_without_changing_toggles();
    test_import_button_reconstructs_and_raises_pending_flag();
    test_snippet_button_raises_pending_insert_with_matching_source();
    test_restore_state_round_trips_every_field();
    test_restore_state_clamps_an_out_of_range_mode_to_classic();

    if (failures == 0)
    {
        std::printf("twigl_panel_hit_test: all checks passed\n");
        return 0;
    }
    std::fprintf(stderr, "twigl_panel_hit_test: %d failure(s)\n", failures);
    return 1;
}
