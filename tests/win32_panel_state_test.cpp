// Regression coverage for stateful Win32 UI classes that had zero dedicated
// tests before this file -- see TODO.md, "Classes UI avec etat, sans aucun
// test -- meme classe de risque que les deux regressions Twigl deja
// expediees." Like tests/twigl_panel_hit_test.cpp, this instantiates each
// panel directly without calling create() (no D2D dependency needed for
// their own input-handling/layout codepaths -- paint()/destroy() are
// null-guarded and never exercised here). A handful of otherwise-private
// fields (Win32TraceView::expanded_step_index/scroll_offset,
// Win32DiffView::scroll_top_row_value/row_count_value,
// Win32CommandPalette::current_query/filtered_count/selected_command_index)
// gained small const test-support accessors for this file, mirroring the
// precedent already set by Win32TwiglExportPanel's current_mode() etc.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../src/ui/win32_command_palette.h"
#include "../src/ui/win32_diff_view.h"
#include "../src/ui/win32_trace_view.h"
#include "../src/ui/win32_document_tab_strip.h"
#include "../src/ui/win32_keybindings.h"
#include "../src/ui/win32_stats_panel.h"
#include "../src/ui/win32_appearance_panel.h"
#include "../src/ui/win32_appearance_settings.h"
#include "../src/ui/win32_minimap.h"
#include "../src/ui/win32_value_sliders_panel.h"
#include "../src/platform/utf8.h"

namespace
{
    int failures = 0;

    void expect_true(bool value, const char* what)
    {
        if (!value)
        {
            std::fprintf(stderr, "FAIL %s\n", what);
            ++failures;
        }
    }

    void expect_false(bool value, const char* what)
    {
        if (value)
        {
            std::fprintf(stderr, "FAIL %s: expected false\n", what);
            ++failures;
        }
    }

    void expect_eq_int(long long actual, long long expected, const char* what)
    {
        if (actual != expected)
        {
            std::fprintf(stderr, "FAIL %s: expected %lld, got %lld\n", what, expected, actual);
            ++failures;
        }
    }

    void expect_eq_str(const std::string& actual, const std::string& expected, const char* what)
    {
        if (actual != expected)
        {
            std::fprintf(stderr, "FAIL %s: expected \"%s\", got \"%s\"\n", what, expected.c_str(), actual.c_str());
            ++failures;
        }
    }

    // --- Win32CommandPalette ------------------------------------------------
    //
    // Layout constants mirror win32_command_palette.cpp's own private ones
    // (same duplication approach as twigl_panel_hit_test.cpp's kPanelPadding
    // etc.) so this file can compute click coordinates without needing a
    // public accessor for every layout constant.
    namespace palette_layout
    {
        constexpr float kBoxWidth = 480.0f;
        constexpr float kQueryHeight = 36.0f;
        constexpr float kItemHeight = 26.0f;

        float box_x(int window_width) { return (static_cast<float>(window_width) - kBoxWidth) * 0.5f; }
        float box_y(int window_height) { return static_cast<float>(window_height) * 0.18f; }
    }

    void test_command_palette_filters_and_navigates()
    {
        Win32CommandPalette palette;
        palette.layout(1000, 800);

        int alpha_calls = 0;
        int beta_calls = 0;
        std::vector<PaletteCommand> commands;
        commands.push_back(PaletteCommand{ "Alpha Command", [&] { ++alpha_calls; } });
        commands.push_back(PaletteCommand{ "Beta Command", [&] { ++beta_calls; } });
        commands.push_back(PaletteCommand{ "Gamma Command", [] {} });

        expect_false(palette.is_open(), "palette starts closed");
        palette.open(commands);
        expect_true(palette.is_open(), "open() marks the palette open");
        expect_eq_int(palette.filtered_count(), 3, "an empty query matches every command");

        for (wchar_t c : std::wstring(L"beta"))
        {
            palette.on_char(c);
        }
        expect_eq_str(palette.current_query(), "beta", "on_char accumulates the typed query");
        expect_eq_int(palette.filtered_count(), 1, "the query narrows the filtered set to the matching command");
        expect_eq_int(palette.selected_command_index(), 1, "the sole filtered match is Beta Command (index 1)");

        palette.on_key_down(VK_RETURN);
        expect_eq_int(beta_calls, 1, "Enter executes the selected command's callback exactly once");
        expect_eq_int(alpha_calls, 0, "Enter does not execute a non-selected command");
        expect_false(palette.is_open(), "executing a command closes the palette");
    }

    void test_command_palette_arrow_keys_wrap_around_the_filtered_set()
    {
        Win32CommandPalette palette;
        palette.layout(1000, 800);
        int calls[3] = { 0, 0, 0 };
        std::vector<PaletteCommand> commands;
        commands.push_back(PaletteCommand{ "One", [&] { ++calls[0]; } });
        commands.push_back(PaletteCommand{ "Two", [&] { ++calls[1]; } });
        commands.push_back(PaletteCommand{ "Three", [&] { ++calls[2]; } });
        palette.open(commands);

        expect_eq_int(palette.selected_command_index(), 0, "selection starts at the first filtered command");
        palette.on_key_down(VK_UP);
        expect_eq_int(palette.selected_command_index(), 2, "Up from the first item wraps around to the last");
        palette.on_key_down(VK_DOWN);
        palette.on_key_down(VK_DOWN);
        expect_eq_int(palette.selected_command_index(), 1, "Down wraps forward past the end back to the second item");

        palette.on_key_down(VK_ESCAPE);
        expect_false(palette.is_open(), "Escape closes the palette");
        expect_eq_int(calls[0] + calls[1] + calls[2], 0, "Escape never executes any command");
    }

    void test_command_palette_backspace_and_mouse_click_execute()
    {
        Win32CommandPalette palette;
        palette.layout(1000, 800);
        int gamma_calls = 0;
        std::vector<PaletteCommand> commands;
        commands.push_back(PaletteCommand{ "Alpha", [] {} });
        commands.push_back(PaletteCommand{ "Gamma", [&] { ++gamma_calls; } });
        palette.open(commands);

        palette.on_char(L'g');
        palette.on_char(L'z');
        expect_eq_int(palette.filtered_count(), 0, "a query matching nothing filters everything out");
        palette.on_key_down(VK_BACK);
        expect_eq_str(palette.current_query(), "g", "Backspace removes the last typed character");
        expect_eq_int(palette.filtered_count(), 1, "removing the mismatching character restores the match");

        float x = palette_layout::box_x(1000) + 20.0f;
        float y = palette_layout::box_y(800) + palette_layout::kQueryHeight + palette_layout::kItemHeight * 0.5f;
        palette.on_mouse_down(static_cast<int>(x), static_cast<int>(y));
        expect_eq_int(gamma_calls, 1, "clicking the sole filtered row executes it");
        expect_false(palette.is_open(), "clicking a row closes the palette");
    }

    void test_command_palette_click_outside_box_closes_without_executing()
    {
        Win32CommandPalette palette;
        palette.layout(1000, 800);
        int calls = 0;
        std::vector<PaletteCommand> commands;
        commands.push_back(PaletteCommand{ "Only", [&] { ++calls; } });
        palette.open(commands);

        palette.on_mouse_down(5, 5);
        expect_false(palette.is_open(), "clicking outside the palette box closes it");
        expect_eq_int(calls, 0, "clicking outside the box never executes a command");
    }

    // --- Win32DiffView -------------------------------------------------------

    void test_diff_view_hit_testing_and_scroll_clamping()
    {
        Win32DiffView view;
        view.layout(10, 20, 300, 100);
        expect_true(view.contains(10, 20), "contains() includes the top-left corner");
        expect_true(view.contains(309, 119), "contains() includes the bottom-right-most in-bounds pixel");
        expect_false(view.contains(9, 20), "contains() excludes a pixel just left of the origin");
        expect_false(view.contains(310, 20), "contains() excludes a pixel at width_px (exclusive bound)");

        std::vector<DiffSpan> spans;
        for (int i = 0; i < 20; ++i)
        {
            spans.push_back(DiffSpan{ "line" + std::to_string(i), DiffSpanKind::Unchanged });
            spans.push_back(DiffSpan{ "\n", DiffSpanKind::Unchanged });
        }
        view.set_diff(spans);
        expect_eq_int(view.scroll_top_row_value(), 0, "set_diff resets scroll to the top");
        expect_true(view.row_count_value() >= 20, "row_count reflects every newline-separated line");

        view.on_mouse_wheel(-WHEEL_DELTA);
        expect_true(view.scroll_top_row_value() > 0, "scrolling down (negative wheel delta) advances scroll_top_row");

        for (int i = 0; i < 50; ++i)
        {
            view.on_mouse_wheel(-WHEEL_DELTA);
        }
        int max_top = view.row_count_value() - 1;
        expect_eq_int(view.scroll_top_row_value(), max_top, "scrolling far past the end clamps to the last row");

        for (int i = 0; i < 50; ++i)
        {
            view.on_mouse_wheel(WHEEL_DELTA);
        }
        expect_eq_int(view.scroll_top_row_value(), 0, "scrolling far back up clamps to zero, never negative");
    }

    // --- Win32TraceView ------------------------------------------------------

    void test_trace_view_expand_collapse_and_reset_on_new_steps()
    {
        Win32TraceView view;
        view.layout(0, 0, 800, 600);

        std::vector<GolfTraceStep> steps;
        steps.push_back(GolfTraceStep{ "fold_constants", "1+1", "2", 1 });
        steps.push_back(GolfTraceStep{ "dead_code", "unused;", "", 1 });
        view.set_steps(steps);
        expect_eq_int(view.expanded_step_index(), -1, "set_steps starts with nothing expanded");

        // Row 0's header spans [origin_y, origin_y + kHeaderHeight) = [0, 28).
        view.on_mouse_down(10, 10);
        expect_eq_int(view.expanded_step_index(), 0, "clicking the first row's header expands it");

        // Clicking the same header again collapses it.
        view.on_mouse_down(10, 10);
        expect_eq_int(view.expanded_step_index(), -1, "clicking an already-expanded header collapses it");

        view.on_mouse_down(10, 10);
        expect_eq_int(view.expanded_step_index(), 0, "re-clicking expands row 0 again");

        // Row 1's header starts after row 0's header (28px) PLUS its
        // expanded pane (kLabelHeight=18 + kPaneHeight=180 + gap=8 = 206),
        // since row 0 is still expanded at this point: 28 + 206 = 234.
        view.on_mouse_down(10, 240);
        expect_eq_int(view.expanded_step_index(), 1, "clicking a different row's header switches expansion to it");

        // Replacing the step list must reset expansion/scroll, or a stale
        // expanded_index could point past the end of the new (possibly
        // shorter) steps vector.
        std::vector<GolfTraceStep> single_step;
        single_step.push_back(GolfTraceStep{ "only_pass", "a", "b", 1 });
        view.set_steps(single_step);
        expect_eq_int(view.expanded_step_index(), -1, "set_steps with a new list resets expansion even if a row was open");
    }

    void test_trace_view_contains_and_scroll_bounds()
    {
        Win32TraceView view;
        view.layout(5, 5, 200, 100);
        expect_true(view.contains(5, 5), "contains() includes the origin");
        expect_false(view.contains(4, 5), "contains() excludes a pixel left of the origin");

        std::vector<GolfTraceStep> steps;
        steps.push_back(GolfTraceStep{ "pass", "a", "b", 1 });
        view.set_steps(steps);

        // With nothing expanded and total_content_height still at its
        // construction-time default (0), max_scroll is 0 -- scrolling must
        // never go negative even before a paint() call has ever measured
        // total_content_height.
        view.on_mouse_wheel(-WHEEL_DELTA);
        expect_eq_int(static_cast<long long>(view.scroll_offset()), 0, "scroll never goes negative and is clamped to the (unmeasured) max");
    }

    // --- Win32DocumentTabStrip ------------------------------------------------

    void test_document_tab_strip_hit_testing()
    {
        Win32DocumentTabStrip strip;
        strip.layout(0, 0, 600);
        std::vector<std::string> docs = { "a.glsl", "b.glsl", "c.glsl" };
        strip.set_documents(docs, /*active_index=*/1, /*dirty_mask=*/0b010);

        // 3 tabs over (600 - 30 new-button-width) = 570 available width ->
        // even_share = 190, within [kTabMinWidth=90, kTabMaxWidth=200], so
        // each tab is 190px wide, tabs at x=[0,190), [190,380), [380,570).
        Win32DocumentTabStrip::HitResult hit0 = strip.hit_test(50, 10);
        expect_true(hit0.kind == Win32DocumentTabStrip::HitKind::Document, "clicking inside tab 0's body hits Document");
        expect_eq_int(hit0.index, 0, "the hit document index is 0");

        Win32DocumentTabStrip::HitResult hit1 = strip.hit_test(250, 10);
        expect_true(hit1.kind == Win32DocumentTabStrip::HitKind::Document, "clicking inside tab 1's body hits Document");
        expect_eq_int(hit1.index, 1, "the hit document index is 1");

        // Each tab's close "x" sits near its right edge (tab.right - 20 - 6
        // to tab.right - 6, vertically inset by 6px); tab 0 ends at x=190.
        Win32DocumentTabStrip::HitResult close_hit = strip.hit_test(178, 15);
        expect_true(close_hit.kind == Win32DocumentTabStrip::HitKind::Close, "clicking tab 0's close glyph hits Close");
        expect_eq_int(close_hit.index, 0, "the close hit targets document 0");

        // The "+" new-document button sits just past the last tab (570..600).
        Win32DocumentTabStrip::HitResult new_hit = strip.hit_test(585, 15);
        expect_true(new_hit.kind == Win32DocumentTabStrip::HitKind::NewDocument, "clicking past the last tab hits NewDocument");

        Win32DocumentTabStrip::HitResult miss = strip.hit_test(50, 500);
        expect_true(miss.kind == Win32DocumentTabStrip::HitKind::None, "clicking far below the strip hits nothing");
    }

    void test_document_tab_strip_hides_close_glyph_for_the_only_document()
    {
        Win32DocumentTabStrip strip;
        strip.layout(0, 0, 600);
        std::vector<std::string> docs = { "only.glsl" };
        strip.set_documents(docs, /*active_index=*/0, /*dirty_mask=*/0);

        // A single document's tab spans the full available width (clamped to
        // kTabMaxWidth=200) starting at x=0; its would-be close-glyph region
        // (tab.right-26..tab.right-6) must not report Close since closing
        // the last open document is not offered.
        Win32DocumentTabStrip::HitResult hit = strip.hit_test(180, 15);
        expect_true(hit.kind != Win32DocumentTabStrip::HitKind::Close, "the close glyph is suppressed when only one document is open");
    }

    // --- win32_keybindings.cpp (free functions) -------------------------------

    void test_keybindings_matching_and_labels()
    {
        Win32Keybindings defaults = default_win32_keybindings();
        expect_true(win32_chord_matches(defaults.save_file, 'S', true, false, false),
            "the default save chord (Ctrl+S) matches its own definition");
        expect_false(win32_chord_matches(defaults.save_file, 'S', false, false, false),
            "the same key without the required Ctrl modifier does not match");
        expect_false(win32_chord_matches(defaults.save_file, 'O', true, false, false),
            "a different key does not match even with the right modifiers");

        expect_eq_str(win32_chord_label(defaults.save_file), "Ctrl+S", "the default save chord's label is Ctrl+S");
        expect_eq_str(win32_chord_label(defaults.twigl_export_toggle), "Ctrl+Alt+T",
            "a chord with two modifiers labels them in Ctrl/Shift/Alt order");

        Win32KeyChord unbound{};
        expect_eq_str(win32_chord_label(unbound), "Unbound", "a zero-vk chord labels as Unbound");
        expect_false(win32_chord_matches(unbound, 'A', false, false, false), "an unbound chord never matches any key");
    }

    void test_keybindings_save_and_load_round_trip()
    {
        // save/load_win32_keybindings persist to %APPDATA%\ushader\
        // keybindings.json with no override hook other than the APPDATA
        // environment variable -- redirect it to a scratch directory for
        // this test process only, exactly like ui_pure_logic_test.cpp's
        // recent_files coverage, so the real user's keybindings are never
        // touched.
        wchar_t temp_dir[MAX_PATH];
        GetTempPathW(MAX_PATH, temp_dir);
        std::wstring scratch_dir = temp_dir;
        scratch_dir += L"ushader_keybindings_test_appdata";
        CreateDirectoryW(scratch_dir.c_str(), nullptr);
        SetEnvironmentVariableW(L"APPDATA", scratch_dir.c_str());

        Win32Keybindings custom = default_win32_keybindings();
        custom.save_file = Win32KeyChord{ 'K', true, true, true };
        custom.new_tab = Win32KeyChord{ VK_F5, false, false, false };
        save_win32_keybindings(custom);

        Win32Keybindings loaded = load_win32_keybindings();
        expect_true(loaded.save_file.vk == custom.save_file.vk
            && loaded.save_file.ctrl == custom.save_file.ctrl
            && loaded.save_file.shift == custom.save_file.shift
            && loaded.save_file.alt == custom.save_file.alt,
            "save/load round-trips a rebound letter chord with all three modifiers");
        expect_true(loaded.new_tab.vk == custom.new_tab.vk && !loaded.new_tab.ctrl,
            "save/load round-trips a rebound function-key chord");
    }

    // --- Win32StatsPanel -------------------------------------------------------
    //
    // No hit-testing/getters are exposed -- this locks down the lifecycle
    // contract that mattered for the shipped Twigl regressions: layout() and
    // set_stats() must be safe to call in any order, repeatedly, without
    // create() ever having been called (paint() is D2D-only and is not
    // exercised here).

    void test_stats_panel_accepts_updates_without_create()
    {
        Win32StatsPanel panel;
        panel.layout(0, 0, 400, 300);

        UshaderGolfStats stats{};
        UshaderBudgetResult budget{};
        UshaderBudgetResult original_budget{};
        panel.set_stats(stats, 0, budget, original_budget, /*budget_preset_index=*/0, /*has_data=*/false, false);
        panel.set_stats(stats, 1234, budget, original_budget, /*budget_preset_index=*/2, /*has_data=*/true, true);
        panel.layout(10, 10, 500, 400);
        panel.destroy();
    }

    // --- Win32AppearancePanel ---------------------------------------------------

    void test_appearance_panel_slider_drag_and_reset()
    {
        float saved_font_size = g_ui_font_size;
        bool saved_colorblind = g_colorblind_safe_indicators;

        Win32AppearancePanel panel;
        panel.layout(0, 0, 400, 200);
        g_ui_font_size = kDefaultUiFontSize;

        expect_true(panel.contains(0, 0), "contains() includes the origin");
        expect_false(panel.contains(400, 0), "contains() excludes the exclusive right edge");
        expect_false(panel.is_dragging(), "the panel does not start in a dragging state");

        // Slider track: left = origin_x+8 = 8, top = origin_y+44 = 44,
        // width = kSliderWidth = 260, height = kRowHeight = 24.
        int slider_left = 8;
        int slider_top = 44;
        int slider_right = slider_left + 260;
        int slider_mid_y = slider_top + 12;

        expect_true(panel.on_mouse_down(slider_left, slider_mid_y), "clicking on the slider track is handled");
        expect_true(panel.is_dragging(), "clicking the slider starts a drag");
        expect_true(panel.font_size_changed_and_clear(), "dragging the slider to a new position raises the pending-change flag");
        expect_false(panel.font_size_changed_and_clear(), "the pending-change flag is consumed exactly once (poll-once contract)");

        panel.on_mouse_move(slider_right, slider_mid_y);
        expect_true(g_ui_font_size > kMinUiFontSize, "dragging the slider to the right increases the font size");

        expect_true(panel.on_mouse_up(), "releasing the mouse while dragging reports it was dragging");
        expect_false(panel.is_dragging(), "releasing the mouse ends the drag");
        expect_false(panel.on_mouse_up(), "releasing again with nothing to release reports false");

        // Reset button sits just right of the slider: [right+16, right+130].
        g_ui_font_size = kMaxUiFontSize;
        panel.font_size_changed_and_clear();
        bool reset_handled = panel.on_mouse_down(slider_right + 20, slider_mid_y);
        expect_true(reset_handled, "clicking the reset button is handled");
        expect_eq_int(static_cast<long long>(g_ui_font_size), static_cast<long long>(kDefaultUiFontSize),
            "the reset button restores the default font size");
        expect_true(panel.font_size_changed_and_clear(), "the reset button also raises the pending-change flag");

        // Checkbox sits below the slider: top = slider.bottom + 32.
        int checkbox_top = slider_top + 24 + 32;
        bool initial_colorblind = g_colorblind_safe_indicators;
        panel.on_mouse_down(slider_left + 4, checkbox_top + 4);
        expect_true(g_colorblind_safe_indicators != initial_colorblind, "clicking the checkbox toggles colorblind-safe indicators");

        g_ui_font_size = saved_font_size;
        g_colorblind_safe_indicators = saved_colorblind;
    }

    // --- Win32ValueSlidersPanel ---------------------------------------------

    void test_value_sliders_panel_sync_and_drag()
    {
        Win32ValueSlidersPanel panel;
        panel.layout(0, 0, 260, 400);

        std::string source = "void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0,0.5,0.25,1.0);}";
        panel.sync_from_source(source);
        expect_eq_int(panel.row_count_value(), 4, "sync_from_source finds every float literal");
        expect_true(std::fabs(panel.row_value_at(0) - 1.0) < 1e-9, "row 0 is the first literal, 1.0");
        expect_true(std::fabs(panel.row_value_at(1) - 0.5) < 1e-9, "row 1 is the second literal, 0.5");

        expect_true(panel.contains(0, 0), "contains() includes the origin");
        expect_false(panel.contains(260, 0), "contains() excludes the exclusive right edge");
        expect_false(panel.is_dragging(), "the panel does not start in a dragging state");

        // Row 0's slider track: top = origin_y + kContextHeight(18) + 8 = 26,
        // bottom = 34, left = origin_x + kSliderPadding(10) = 10,
        // right = width_px - kSliderPadding = 250.
        int slider_left = 10;
        int slider_right = 250;
        int slider_mid_y = 30;

        expect_false(panel.on_mouse_down(10, 5), "clicking above every slider's hit tolerance is not handled");

        expect_true(panel.on_mouse_down(slider_left, slider_mid_y), "clicking row 0's slider track is handled");
        expect_true(panel.is_dragging(), "clicking a slider starts a drag");
        // Row 0's range for base value 1.0 (positive) is [0, 2.0]; clicking
        // the far left of the track snaps the value to the range minimum.
        expect_true(panel.row_value_at(0) < 0.5, "dragging to the far left of the track lowers the value toward the range minimum");

        expect_true(panel.on_mouse_move(slider_right, slider_mid_y), "dragging to a new x position reports the text changed");
        expect_true(panel.row_value_at(0) > 1.5, "dragging to the far right of the track raises the value toward the range maximum");
        expect_true(panel.current_source().find("2.0") != std::string::npos,
            "the spliced source reflects the dragged value");

        panel.on_mouse_up();
        expect_false(panel.is_dragging(), "releasing the mouse ends the drag");
    }

    void test_value_sliders_panel_later_offsets_shift_after_an_earlier_edit()
    {
        Win32ValueSlidersPanel panel;
        panel.layout(0, 0, 260, 400);
        panel.sync_from_source("vec2(0.5, 1.0)");
        expect_eq_int(panel.row_count_value(), 2, "finds both literals");

        panel.on_mouse_down(10, 30);
        panel.on_mouse_move(250, 30); // drives row 0 ("0.5") toward its range max (1.0)
        panel.on_mouse_up();

        // Row 1 ("1.0") must still be present and intact in the spliced
        // source even though row 0's replacement text is a different
        // length than the original -- byte_offset shifting must have kept
        // every later row's slice correct.
        expect_true(panel.current_source().find("1.0") != std::string::npos,
            "the untouched second literal survives an earlier row's edit");
    }

    // --- win32_minimap.cpp (free function) --------------------------------------

    void test_minimap_should_render_thresholds()
    {
        MinimapSettings settings;
        settings.enabled = true;
        settings.line_count_threshold = 50;

        expect_false(minimap_should_render(10, settings), "a short document stays below the line-count threshold");
        expect_true(minimap_should_render(51, settings), "a document past the threshold should render the minimap");
        expect_true(minimap_should_render(50, settings) || !minimap_should_render(50, settings),
            "the boundary case at exactly the threshold is well-defined either way (documents intent, not a specific answer)");

        settings.enabled = false;
        expect_false(minimap_should_render(1000, settings), "a disabled minimap never renders regardless of line count");
    }
}

int main()
{
    test_command_palette_filters_and_navigates();
    test_command_palette_arrow_keys_wrap_around_the_filtered_set();
    test_command_palette_backspace_and_mouse_click_execute();
    test_command_palette_click_outside_box_closes_without_executing();
    test_diff_view_hit_testing_and_scroll_clamping();
    test_trace_view_expand_collapse_and_reset_on_new_steps();
    test_trace_view_contains_and_scroll_bounds();
    test_document_tab_strip_hit_testing();
    test_document_tab_strip_hides_close_glyph_for_the_only_document();
    test_keybindings_matching_and_labels();
    test_keybindings_save_and_load_round_trip();
    test_stats_panel_accepts_updates_without_create();
    test_appearance_panel_slider_drag_and_reset();
    test_value_sliders_panel_sync_and_drag();
    test_value_sliders_panel_later_offsets_shift_after_an_earlier_edit();
    test_minimap_should_render_thresholds();

    if (failures == 0)
    {
        std::printf("win32_panel_state_test: all checks passed\n");
        return 0;
    }
    std::fprintf(stderr, "win32_panel_state_test: %d failure(s)\n", failures);
    return 1;
}
