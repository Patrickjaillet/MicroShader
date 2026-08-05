// Regression test for the golfer/Twigl uniform-name collision bug: the
// golfer used to be free to rename an unrelated local identifier to a
// single-character name (r/m/t/f/b/s/o) that the subsequent Twigl uniform
// rewrite (iResolution->r, iTime->t, ...) would then collide with, silently
// corrupting the exported/previewed Twigl code in Geek/Geeker/Geekest mode.
//
// Win32TwiglExportPanel::reserved_uniform_names_csv() is the fix: it reports
// exactly which single-character names the panel's *current* mode/MRT/
// backbuffer/sound state will substitute into the export text, so
// main_win32.cpp's effective_protected_names_for_golf() can fold them into
// the golfer's protected-names set before compiling. The golfer's own
// protected-names mechanism is already covered by
// rust-core/src/golfer.rs's protected_names_are_never_renamed test; this
// test locks down the C++ side that computes *which* names to protect.
//
// See roadmap.md, section "Correction déjà appliquée (2026-08-05)".
#include <cstdio>
#include <string>

#include "../src/ui/win32_twigl_export_panel.h"

namespace
{
    int failures = 0;

    void expect_eq(const std::string& actual, const std::string& expected, const char* what)
    {
        if (actual != expected)
        {
            std::fprintf(stderr, "FAIL %s: expected \"%s\", got \"%s\"\n", what, expected.c_str(), actual.c_str());
            ++failures;
        }
    }
}

int main()
{
    Win32TwiglExportPanel panel;

    // Classic mode uses long names (resolution/mouse/time/...) that the
    // golfer's shortest-name-first allocator would never pick anyway.
    panel.restore_state(/*mode=*/0, /*es300=*/false, /*mrt=*/1, /*backbuffer=*/false, /*sound=*/false);
    expect_eq(panel.reserved_uniform_names_csv(), "", "Classic mode reserves nothing");

    // Geek mode, single target, no backbuffer/sound: r/m/t/f (inputs) + o (output).
    panel.restore_state(/*mode=*/1, /*es300=*/false, /*mrt=*/1, /*backbuffer=*/false, /*sound=*/false);
    expect_eq(panel.reserved_uniform_names_csv(), "r,m,t,f,o", "Geek mode, no backbuffer/sound");

    // Geeker mode with backbuffer + sound also reserves b and s.
    panel.restore_state(/*mode=*/2, /*es300=*/false, /*mrt=*/1, /*backbuffer=*/true, /*sound=*/true);
    expect_eq(panel.reserved_uniform_names_csv(), "r,m,t,f,o,b,s", "Geeker mode with backbuffer+sound");

    // Geekest MRT reserves o0/o1 instead of a bare o, plus FC (gl_FragCoord's
    // Geekest-only shorthand).
    panel.restore_state(/*mode=*/3, /*es300=*/true, /*mrt=*/2, /*backbuffer=*/false, /*sound=*/false);
    expect_eq(panel.reserved_uniform_names_csv(), "r,m,t,f,o0,o1,FC", "Geekest MRT reserves o0/o1 and FC");

    // MRT with backbuffer also doubles the backbuffer name (b0/b1).
    panel.restore_state(/*mode=*/1, /*es300=*/true, /*mrt=*/2, /*backbuffer=*/true, /*sound=*/false);
    expect_eq(panel.reserved_uniform_names_csv(), "r,m,t,f,o0,o1,b0,b1", "Geek MRT with backbuffer doubles b0/b1");

    if (failures == 0)
    {
        std::printf("twigl_golf_collision_test: all checks passed\n");
        return 0;
    }
    std::fprintf(stderr, "twigl_golf_collision_test: %d failure(s)\n", failures);
    return 1;
}
