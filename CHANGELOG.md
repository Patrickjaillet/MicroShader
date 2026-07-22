# Changelog

All notable changes to µShader are documented in this file.

## [Unreleased]

## [3.0.1] - 2026-07-22

UX feedback fix: the Controls panel becomes a persistent right-side
inspector instead of a full-screen tab, and two actions that were
previously keyboard/command-palette-only get visible buttons.

### Changed

- The "Controls" tab is removed; `Win32GolfControls` now renders as a
  fixed-width (300px) inspector docked to the right edge, visible on
  every tab except Viewport (which keeps the full window width,
  needed for Compare mode's split render). All other tab content
  (editors, Diff, Trace, Stats, Appearance, About) narrows to make
  room; the Source/Golfed minimap repositions to stay left of the
  inspector instead of the raw window edge.
- The pass-checkbox list switched from two columns to one: the
  two-column layout was sized for a full-width tab and truncated
  labels like "Common subexpressions" once squeezed into the 300px
  inspector. Single column uses the inspector's full available height
  instead, with no truncation.

### Added

- A "Golf" button at the top of the inspector runs the same
  recompile action as `F5` or the command palette's "Run golf" entry
  — previously keyboard/palette-only.
- A "Formatted view" toggle button in the top-right of the Golfed
  tab's content area, mirroring the existing `Ctrl+Shift+F` chord —
  previously also keyboard/palette-only.
- Both new buttons (`src/ui/win32_tool_button.h/.cpp`, a small
  reusable owner-drawn button) are registered for UI Automation like
  every other Phase 26 control, verified live with the same
  `System.Windows.Automation` client used throughout that phase.

## [3.0.0] - 2026-07-22

**Breaking change**: µShader's UI framework is now native Win32 +
Direct2D/DirectWrite/GDI+ + WGL-hosted OpenGL — Dear ImGui, SDL3, and
the ImGuiColorTextEdit widget are fully removed from the build.
`ushader.exe` is the Win32 shell built through Phases 22–26; the old
SDL3/ImGui app is gone, not just superseded. This closes the
Phase 22–27 migration arc.

This release ships a deliberately smaller, curated feature set, not
full parity with the retired app — see "Removed / not yet ported"
below and `ROADMAP.md` Phase 28 for the tracked follow-up plan.

### Changed

- `CMakeLists.txt`: removed the `SDL3`, `imgui`, and
  `imguicolortextedit` `FetchContent` dependencies and library
  targets; links `user32`, `gdi32`, `gdiplus`, `d2d1`, `dwrite`, and
  `opengl32` directly (all in-box on every Windows 10/11 edition,
  including LTSC 2019 — no separate runtime to install).
- The former `ushader_win32_shell` CMake target is renamed `ushader`,
  reclaiming the name from the deleted SDL3 app; its subsystem changed
  from `/SUBSYSTEM:CONSOLE` (a development-time console window) to
  `/SUBSYSTEM:WINDOWS` — verified live that no console window appears.
- `theme_tokens.h`'s color constants moved off `ImVec4` (the one
  remaining header-level ImGui dependency the Win32 shell had picked
  up) onto a local `tokens::Color4` struct with the same field layout.
- `installer/ushader.iss`, `README.md`, and `THIRD_PARTY_NOTICES.md`
  rewritten to describe the actual shipped shell and its real
  dependencies (previously: FFmpeg, `gif-h`, Inter/Lucide fonts — all
  removed, see below).

### Removed / not yet ported

Six features from the retired ImGui app did not make it into the
Win32 shell during Phase 26 and were deferred rather than bundled
into this release — tracked in `ROADMAP.md` Phase 28, not silently
dropped:

- Golfing profiles (`.ushaderprofile` save/load, `Maximum`/`Safe`/
  `None` built-ins) — the CLI's `--profile` flag and the published
  schema are unaffected, this is only the GUI-side save/load UI.
- Multi-document workspace (multiple open shaders as separate tabs)
  and the session-restore-on-launch behavior that depended on it.
- One-click "Copy as Shadertoy / Bonzomatic / bare main()" clipboard
  exports.
- Importing a Shader Minifier–style exclude-name list.
- Self-contained HTML session report export.
- Viewport capture/recording: PNG screenshots, and GIF/MP4/WebM
  recording (the latter bundled `ffmpeg.exe` and `gif-h`, both now
  removed along with the feature).

### Fixed

- `assets/fonts/` (`Inter.ttf`, `lucide.ttf`) removed — unused once
  the ImGui shell that loaded them was gone; this phase's own opening
  sentence explicitly named the Lucide font as something that must not
  survive it.
- Several latent build issues surfaced only once SDL3/ImGui's
  incidental include-path and link propagation went away:
  `exclude_list_import_test` needed a real source swap
  (`file_dialog_sdl.cpp` → `file_dialog.cpp`, and
  `import_exclude_list_action` changed from taking `SDL_Window*` to
  `HWND`), three test targets had a dead `SDL3::SDL3` link, and
  `ushaderprofile_schema_test` was missing an explicit `rust_core`
  link it had never needed before. `workspace.h`/`.cpp` and
  `golf_controls.h` had a transitive dependency on the deleted
  `theme.h`, decoupled with a local literal default and by dropping
  dead SDL-specific declarations.
- `scripts/check_contrast.py`'s token-parsing regex updated for the
  `ImVec4` → `Color4` rename — re-verified all four text/background
  pairs still pass WCAG AA (5.87:1 to 13.36:1).

### Known gap

- The Windows 10 LTSC 2019 hardware/VM acceptance pass from
  `ROADMAP.md` Phase 27 has not been performed — this development
  environment has no LTSC 2019 target. Everything verifiable here
  (clean build, all 8 test executables passing, `wgl_equivalence_test`
  still 5/5 bit-exact, live UI smoke test) is done.

## [2.21.0] - 2026-07-22

Phase 26 (Editor & panel feature parity) is now complete — this
closes its final checklist item, Windows UI Automation coverage for
`ushader_win32_shell`. Includes a real bug fix, found only through
live verification with an actual UI Automation client, not just a
build check.

### Added

- Extracted the SDL-independent core of `src/platform/accessibility.cpp`
  into `platform/accessibility_core.h/.cpp`, linked into both `ushader`
  and `ushader_win32_shell`; `accessibility.cpp` is now a thin shim
  that extracts an `HWND` from `SDL_Window*` and forwards to
  `accessibility_init_hwnd`.
- `main_win32.cpp` calls `accessibility_init_hwnd` directly and wraps
  each frame's paint pass in `accessibility_begin_frame`/`end_frame`.
- Registered every owner-drawn control introduced in Phases 24–26:
  the three title-bar buttons, all nine `TabStrip` tabs, the
  Aggressive-golf toggle plus all 16 pass checkboxes and the
  budget-preset button in the Controls tab, the font-size control and
  colorblind checkbox in Appearance, and the three links in About.

### Fixed

- Windows UI Automation clients (`System.Windows.Automation`, not
  legacy MSAA) query `WM_GETOBJECT` with `lParam = UiaRootObjectId`
  (`-25`), not `OBJID_CLIENT` (`-4`) — the only value this shim ever
  checked for. This meant the entire feature had silently never
  worked for any real UIA client since it was first written, only
  ever build-verified. Found by adding temporary logging and querying
  the running app with .NET's UI Automation client, which returned
  zero children from the top-level window in both apps. Fixed by
  checking for both values (`UiaRootObjectId` added to
  `uia_minimal.h`). Re-verified live afterward: the root element's
  name now correctly reads "uShader" (our provider's override), and
  `FindAll` returns the expected element count with correct
  bounding rectangles and live, correct `ToggleState` values on every
  tab tested (12 elements on Viewport, 30 on Controls, 15 each on
  Appearance and About).

### Known gap

- The old `ushader` (SDL3) target still shows zero UI Automation
  children after this fix, a separate pre-existing issue (likely
  `SDL_GL_CreateContext` recreating the window after
  `accessibility_init` runs) not chased further since that target is
  deleted wholesale in Phase 27.

## [2.20.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), eleventh sub-milestone: the
About panel — the last of the five panels in that checklist item,
which is now fully checked off.

### Added

- A new ninth "About" tab (`src/ui/win32_about_panel.h/.cpp`) hosting
  the version string, copyright line, three clickable links (email
  and two GitHub URLs, opened via `ShellExecuteW` with an
  accent/accent-hover color swap on hover), and the MIT license /
  third-party-notices lines — ported from the old app's About-modal
  "About" sub-tab. The logo image and "Keyboard Shortcuts" sub-tab
  were intentionally not ported: no `branding/logo.png` exists (a
  Phase 25 decision) and the old app's own guard already skips it too,
  and "Keyboard Shortcuts" was never an actual rendered panel in this
  shell (`Win32Keybindings` is config/persistence only, not UI).
- Text size wired through the existing `ui_font_pt()`/
  `rebuild_ui_fonts()` mechanism, consistent with every other Phase 26
  panel.

### Remaining in Phase 26

Windows UI Automation coverage for every owner-drawn control
introduced across Phases 24–26 — the other checklist item in this
phase, independent of the five-panels bullet closed by this release.

## [2.19.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), tenth sub-milestone: the
Appearance panel.

### Added

- A new eighth "Appearance" tab in the native Win32 shell, ported from
  the old app's About-modal Appearance tab: a UI text-size slider
  (13–28pt, default 18pt) with a "Reset to default" action, and a
  "Colorblind-safe status indicators" checkbox.
- The font-size slider is genuinely wired: a new `ui_font_pt()` helper
  (`src/ui/win32_appearance_settings.h/.cpp`) scales every
  DirectWrite-owning panel's font size (editors, diff view, trace
  view, command palette, golf controls, stats panel, tab strip), and
  releasing the slider triggers a new `rebuild_ui_fonts()` in
  `main_win32.cpp` that rebuilds every affected panel's D2D/DWrite
  resources at the new size without losing editor content, cursor
  position, or syntax highlighting.
- The colorblind checkbox is genuinely wired too: `StatusDot` (the
  title-bar compile-status indicator) now draws a filled square
  instead of a circle for the Error state when the toggle is on.
- Wired into the command palette ("Switch to tab: Appearance").
- Live-verified with `PostMessage`+`PrintWindow` screenshots: slider
  drag visibly shrinks tab-strip/title-bar/editor text in the same
  pass, "Reset to default" restores 18pt, and the checkbox toggles
  independently of the slider.

### Remaining in Phase 26

The About panel, plus Windows UI Automation coverage for every
owner-drawn control introduced across Phases 24–26.

## [2.18.0] - 2026-07-22

Fixes a build break in the old SDL3 `ushader` target discovered while
verifying the Compare panel sub-milestone caused no regressions —
unrelated to Compare itself, in the still-in-progress Windows UI
Automation provider (`src/platform/accessibility.cpp`).

### Fixed

- `ushader` failed to compile: `UIAutomationCore.h` in Windows SDK
  10.0.26100.0 self-conflicts for roughly 50 of its COM interfaces —
  MSVC reports the real interface body as a redefinition of that same
  interface's own forward declaration, purely from stock SDK content.
  Ruled out `NTDDI_VERSION`/`_WIN32_WINNT` pinning and MSVC's
  `/Zc:preprocessor-`/`/permissive`/`/permissive-` flags (no effect);
  confirmed pinning the whole project to the older 10.0.22621.0 SDK
  avoids the bug but isn't viable, since that SDK lacks `gameinput.h`
  needed by SDL3's GDK joystick backend. Fixed by adding
  `src/platform/uia_minimal.h`, which hand-declares only the handful
  of COM interfaces, enums, and `UIA_*` ID constants this project's
  accessibility provider actually uses (copied verbatim, GUIDs
  included, from the same SDK header's known-good section), so
  `accessibility.cpp` never includes the broken header at all.
  Rebuilt `ushader` clean and re-ran `wgl_equivalence_test` (still
  5/5 bit-exact) to confirm no other regressions.

## [2.17.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), ninth sub-milestone: the
Compare panel — including a real bug found and fixed during its own
verification.

### Added

- The Viewport tab now genuinely splits into independent Source
  (left) and Golfed (right) renders in Compare mode, toggled via the
  command palette's existing "Toggle Compare mode" entry or a new
  `Ctrl+Shift+C` chord. A second `ShaderRunner` (`g_golfed_runner`)
  compiles the golfed output alongside the existing source runner.
- Each half now renders into its own correctly-sized
  `OffscreenFramebuffer` and is composited onto the visible backbuffer
  with `glBlitFramebuffer`, added to `render/gl_functions.h` and both
  loaders. `OffscreenFramebuffer` gained a `framebuffer_id()`
  accessor alongside its existing `texture_id()`.

### Fixed

- Caught during this sub-milestone's own live verification: the first
  Compare-mode implementation split the two halves with plain
  `glViewport()` calls, but `gl_FragCoord` is window-absolute, not
  viewport-local — the right half's `uv` calculation continued from
  where the left half ended instead of restarting at 0, so the
  "split" view was actually one continuous gradient stretched across
  the full width. Found by pixel-sampling a screenshot across the
  expected seam and seeing none. Fixed with the offscreen-framebuffer
  approach above; re-ran `wgl_equivalence_test` after the GL-loader
  change (still 5/5 bit-exact) and re-screenshotted to confirm two
  independent renders with a visible seam.

### Remaining in Phase 26

Appearance and About panels, plus Windows UI Automation coverage for
every owner-drawn control introduced across Phases 24–26.

## [2.16.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), eighth sub-milestone: the
Stats panel.

### Added

- A seventh "Stats" tab hosting `ui/win32_stats_panel.h`/`.cpp`
  (`Win32StatsPanel`): char-count reduction, all 16 per-pass counters,
  the deflate size estimate, and color-coded budget badges — fed by
  the same `UshaderGolfStats` the Controls-driven
  `ushader_golf_traced()` call already produced (previously computed
  and discarded) plus a new `ushader_estimate_budget()` call. Same
  data and C ABI calls the old app's `render_stats_panel` uses, now
  painted in Direct2D.
- Discovered and adopted a safer live-verification technique this
  session: `PostMessage` (queues input without stealing OS foreground
  focus) plus `PrintWindow` (captures a window's real content even
  when occluded or not frontmost) — sidesteps the focus-stealing
  problem that blocked two earlier sub-milestones' screenshots,
  instead of needing the app window frontmost at all.
- Verified live with it: confirmed real reduction stats and all-zero
  per-pass counters (correct for a shader with nothing for those
  passes to remove), then selected a real budget preset, recompiled,
  and confirmed a correct color-coded "OK Shadertoy: 120 / 65536"
  badge — plus confirmed the preset's unset deflate limit correctly
  suppresses the second badge, matching the old app's own guard.

### Remaining in Phase 26

Compare, Appearance, and About panels, plus Windows UI Automation
coverage for every owner-drawn control introduced across Phases
24–26.

## [2.15.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), seventh sub-milestone: the
Golf Controls panel — the first of five panel bullets, and the one
that finally makes golfing options real in the native shell.

### Added

- A sixth "Controls" tab hosting `ui/win32_golf_controls.h`/`.cpp`
  (`Win32GolfControls`): the "Aggressive golf" toggle, all 16
  individual pass checkboxes (dimmed and inert when Aggressive is
  off), a protected-names text field, and a click-to-cycle budget-
  preset selector — all reading/writing the real `GolfPassToggles`
  struct.
- `recompile_from_editor()` now calls `to_golf_options()` on the
  panel's real toggles instead of an all-`false` placeholder, and
  passes the real protected-names string instead of `nullptr`. This
  is the first point since Phase 26 began where golfing in the native
  shell actually respects user choice rather than a hardcoded
  default.
- `to_golf_options()` — pure, previously bundled inside the ImGui/
  SDL-coupled `ui/golf_controls.cpp` — split into a neutral
  `ui/golf_options_convert.cpp` (same split pattern used repeatedly
  this phase); the old app links both files and is unaffected.
- Scope notes: profile save/load, the built-in Maximum/Safe/None
  profile combo, and exclude-list import are deliberately deferred —
  each is its own real file-dialog/serialization slice, not shoehorned
  in here.
- Verified live and decisively, not just visually: loaded
  `fixtures/dead_stores.glsl` (real dead-store chains), toggled
  Aggressive off and recompiled — the golfed output kept all three
  redundant writes (`a=1.;a=2.;a=3.;`); toggled it back on and
  recompiled — only the final write survived (`a;a=3.;`). Confirmed
  the checkbox genuinely drives the Rust golf engine, not just its own
  visual state.

### Remaining in Phase 26

Stats, Compare, Appearance, and About panels, plus Windows UI
Automation coverage for every owner-drawn control introduced across
Phases 24–26.

## [2.14.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), sixth sub-milestone:
rebindable keybindings, drag-and-drop, and Recent Files.

### Added

- `ui/win32_keybindings.h`/`.cpp` (`Win32Keybindings`): a VK_*-keyed
  keybindings set reading/writing the exact same
  `%APPDATA%\ushader\keybindings.json` the old `ImGuiKey`-keyed app
  uses. The shared string-parsing/writing primitives were extracted
  out of `ui/keybindings.cpp` into `ui/keybindings_storage.h`/`.cpp`;
  the old app now delegates to them and is byte-behavior-identical.
- `platform/file_dialog.cpp` and `platform/paths.cpp` each had their
  one SDL-coupled function split into a `_sdl.cpp` sibling
  (`file_dialog_sdl.cpp`, `paths_sdl.cpp`), so the native shell calls
  `GetOpenFileNameW`/`GetSaveFileNameW` directly by `HWND` without
  linking SDL3 — the same split pattern used for `glsl_language.cpp`/
  `diff_view.cpp`/`command_palette.cpp` earlier in this phase.
- `Ctrl+O` opens a real native file picker (GLSL filter) that loads
  the file into the Source tab, recompiles, and records it via the
  reused, unmodified `ui/recent_files.cpp`. `Ctrl+S` saves the current
  Source text. Dropping a `.glsl` file onto the window
  (`WM_DROPFILES`) does the same as Open. Recent files show up as live
  "Open recent: ..." entries in the command palette.
- Scope notes, not silent gaps: `new_tab`'s chord resets to the
  default shader (closest real analog without a multi-document
  workspace yet); `close_tab` is loaded but intentionally inert (no
  multi-document workspace to close a tab *of*); the interactive
  keybinding-rebind panel is deferred to the Appearance panel, which
  is the natural host for it.
- Verified live end-to-end: opened the real Windows file dialog with
  the correct filter, loaded `fixtures/fractal.glsl` (a genuine
  raymarching shader), watched it compile (green status dot),
  confirmed `recent_files.json` was written in the shared format, then
  confirmed a live "Open recent: ...fractal.glsl" entry in the command
  palette — which also retroactively live-verifies the palette itself,
  unverified in the previous sub-milestone due to a focus-stealing
  block that has since cleared.

### Remaining in Phase 26

The Golf controls/Stats/Compare/Appearance/About panels, and Windows
UI Automation coverage for all of the above.

## [2.13.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), fifth sub-milestone: the
command palette.

### Added

- `ui/win32_command_palette.h`/`.cpp` (`Win32CommandPalette`): a
  Direct2D-painted overlay (scrim + centered query box + filtered
  result list) opened via `Ctrl+Shift+P`, the same chord
  `keybindings.command_palette` already defaults to in the old app.
  Keyboard-driven: type to fuzzy-filter, arrows to move selection,
  Enter to run, Escape or a click outside the box to dismiss; mouse
  hover/click on results works too.
- `ui/fuzzy_match.h`/`.cpp`: the subsequence fuzzy-matcher extracted
  out of `ui/command_palette.cpp` (previously private to that file)
  into a shell-agnostic module. The old app's palette now includes it
  and is otherwise unchanged.
- Commands are scoped honestly to what exists in this shell today —
  no fabricated toggle-pass or export commands ahead of their panels:
  switch to each of the five tabs, run golf (recompile), reset to the
  default shader, toggle Formatted view.

### Verification note

This sub-milestone was **not** live-screenshotted like every other
Phase 26 piece this session. It builds clean in both
`ushader_win32_shell` and the old `ushader` target, but a live check
was blocked when Windows declined to grant the verification script
foreground focus (an active foreground window belonging to the user
at the time) — forcing it risked interrupting that work, so it was
skipped and flagged here rather than silently assumed. A live pass is
still owed before this is fully proven, same standard as every other
piece in this arc.

### Remaining in Phase 26

Keybindings/drag-and-drop/recent files, the Golf controls/Stats/
Compare/Appearance/About panels, and Windows UI Automation coverage
for all of the above.

## [2.12.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), fourth sub-milestone: the
Diff and Trace tabs, completing the editor's feature-parity checklist.

### Added

- A fourth "Diff" tab, hosting a new `ui/win32_diff_view.h`/`.cpp`
  (`Win32DiffView`): a scrollable, word-wrapped render of the real
  unified diff between Source and Golfed, removed tokens struck
  through in red, added tokens in green.
- `compute_unified_diff()` and its LCS-based token diff — previously
  bundled with the ImGui-dependent `render_diff_view()` in one file —
  split out into a new shell-agnostic `ui/unified_diff.h`/`.cpp`, so
  the native shell never links Dear ImGui just to compute a diff.
  `ui/diff_view.cpp` (the still-shipping SDL3 app's rendering half)
  now includes `unified_diff.h` and is otherwise byte-behavior-
  identical.
- A fifth "Trace" tab, hosting a new `ui/win32_trace_view.h`/`.cpp`
  (`Win32TraceView`): real per-pass trace data from
  `ushader_golf_traced()` (replacing the plain `ushader_golf()` call
  from the previous sub-milestone) and the existing
  `parse_golf_trace()`, both reused unchanged. Each of the 16
  aggressive-pass names is a clickable, click-to-expand header
  (dimmed at 0 changes) showing real Before/After source through two
  more read-only `Win32TextEditor` instances. Simplified from the
  ImGui version: single-expand accordion instead of independently
  collapsible headers — noted as a deliberate scope cut, not a silent
  gap.
- Verified live end-to-end on the built `.exe`: the Diff tab's real
  strikethrough/addition colors, the Trace tab's 16 real pass names
  (correctly empty by default, since no Golf Controls panel exists yet
  to enable aggressive passes — verified both the empty and populated
  states), and a header expanding to show real Before/After panes with
  the list reflowing around it.

### Fixed

- Caught and reported a test-automation misclick during this session:
  a `SetForegroundWindow` race let a screenshot-driven click land on
  an unrelated browser tab instead of the app window. Added an
  explicit foreground-window check before every subsequent synthetic
  click. No functional code was affected.

### Remaining in Phase 26

The command palette, keybindings/drag-and-drop/recent files, the Golf
controls/Stats/Compare/Appearance/About panels, and Windows UI
Automation coverage for all of the above.

## [2.11.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), third sub-milestone: the
minimap, continuing from `v2.10.0`'s Golfed view.

### Added

- `ui/win32_minimap.h`/`.cpp` (`paint_minimap()`): a whole-file
  overview of thin, token-colored bars — one row per source line,
  scaled to fit the available height — reproducing `ui/minimap.cpp`'s
  behavior (including the 50-line `MinimapSettings::line_count_threshold`
  before it appears at all) on top of the Phase 26 tokenizer instead of
  `TextEditor::Palette` lookups.
- `ui/glsl_syntax_colors.h`/`.cpp`: the token-to-color mapping pulled
  out of `win32_text_editor.cpp` into its own module so the editor and
  the minimap share one palette instead of two that could drift apart.
- `Win32TextEditor` gained `all_lines()`/`line_count()` accessors; the
  shell's `layout_chrome()` now reserves the minimap's width from the
  editor whenever it's shown, recomputed after every recompile and
  Formatted-view toggle (either can change the line count).
- Verified live: pasted a 61-line shader into the Source tab, watched
  the minimap appear (correctly absent below the 50-line threshold,
  matching the old app) with token colors visibly matching the
  editor's own syntax highlighting, and confirmed the editor's text
  area narrows to make room for it — screenshotted against the running
  `.exe`.

### Remaining in Phase 26

Inline unified diff view, Trace tab panes, the command palette,
keybindings/drag-and-drop/recent files, the Golf controls/Stats/
Compare/Appearance/About panels, and Windows UI Automation coverage
for all of the above.

## [2.10.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), second sub-milestone: the
read-only Golfed view and Formatted toggle, continuing from `v2.9.0`'s
editor control.

### Added

- A third "Golfed" tab in the native shell's tab strip, hosting a
  second, read-only `Win32TextEditor`. Every successful Source-tab
  recompile now also calls the real Rust core golf engine
  (`ushader_golf()`, via `rust_core` — newly linked into
  `ushader_win32_shell`, the same C ABI the SDL3 app uses) and shows
  the actual golfed output: renamed identifiers, shortened numeric
  literals, stripped whitespace, syntax-highlighted like the Source
  tab.
- `Ctrl+Shift+F` toggles the Golfed tab between raw output and a
  reformatted view via `ui/glsl_format.cpp`'s existing `format_glsl()`
  — a pure string function with no ImGui dependency, reused unchanged.
- `Win32TextEditor` gained real read-only enforcement (edits are
  rejected; caret navigation, selection, and copy still work) rather
  than just being a convention — used for the first time by the
  Golfed tab.
- Verified live end-to-end on the built `.exe`: switched to the Golfed
  tab and confirmed real golfed output, toggled formatted view and
  confirmed re-indentation, and confirmed typing into the read-only
  editor is rejected while clicking to move the caret still works —
  screenshotted at each step.

### Remaining in Phase 26

Minimap, inline unified diff view, Trace tab panes, the command
palette, keybindings/drag-and-drop/recent files, the Golf controls/
Stats/Compare/Appearance/About panels, and Windows UI Automation
coverage for all of the above.

## [2.9.0] - 2026-07-22

Phase 26 (Editor & panel feature parity), first sub-milestone: the
bespoke GLSL text-editor control, delivered as its own verified
increment given the phase's size (per Phase 22's risk register, the
largest phase in the arc) — the same incremental-delivery pattern
Phase 21 used, rather than one large unverified pass.

### Added

- `ui/win32_text_editor.h`/`.cpp` (`Win32TextEditor`): a Direct2D-
  rendered, monospace-font code-editor control with real caret
  movement (arrows, word-jump via Ctrl, Home/End, Page Up/Down), text
  selection (Shift+arrows and mouse drag), clipboard cut/copy/paste,
  insertion/deletion, scrolling (mouse wheel and keep-caret-visible),
  and per-line syntax highlighting recomputed on every edit — driven
  directly off `WM_CHAR`/`WM_KEYDOWN`/mouse messages, no `RichEdit`
  host.
- `ui/glsl_token_rules.h`/`.cpp`: the keyword/builtin-function/builtin-
  variable lists and the token-classification decision tree extracted
  out of `ui/glsl_language.cpp` into a shell-agnostic module (a
  `GlslTokenKind` enum instead of ImGuiColorTextEdit's
  `TextEditor::PaletteIndex`), plus a new `tokenize_glsl_line()` line
  tokenizer (with multi-line `/* */` block-comment state carried
  between lines) that `Win32TextEditor` uses for live highlighting.
  `ui/glsl_language.cpp` itself now delegates to this module and keeps
  byte-identical behavior for the still-shipping SDL3 app — verified
  by re-running the affected translation units through the build.
- The new shell's Viewport-only tab strip (Phase 24) grows a second
  "Source" tab hosting the editor, with real click-to-switch between
  Source and Viewport (the WGL child `HWND` is now shown/hidden to
  match). `F5` now compiles whatever is actually in the editor, not a
  hardcoded string; `Shift+F5` resets to the default shader and
  recompiles. Errors highlight the exact offending line in the editor,
  reusing the Phase 23 `ShaderErrorState`/`make_shader_error_state()`
  module rather than re-parsing compiler logs a second time.
- Verified live end-to-end on the built `.exe`: typed real (and
  deliberately broken) GLSL into the Source tab, watched syntax
  highlighting update as-typed, triggered and observed a real compile
  error highlight the exact broken line, recovered via reset, and
  switched tabs to confirm the Viewport keeps rendering independently
  — screenshotted at each step.

### Remaining in Phase 26

Read-only Golfed view + Formatted toggle, minimap, inline unified diff
view, Trace tab panes, the command palette, keybindings/drag-and-drop/
recent files, the Golf controls/Stats/Compare/Appearance/About panels,
and Windows UI Automation coverage for all of the above — tracked in
`ROADMAP.md`, continuing from this sub-milestone.

## [2.8.0] - 2026-07-22

Phase 25 (Iconography, PNG asset system & motion FX), scoped to the
new `ushader_win32_shell` native target (the still-shipping SDL3/Dear
ImGui `ushader` app and its `lucide.ttf` glyph-font icons are
untouched, per the Phase 22 removal-list note that Dear ImGui is
retired wholesale at Phase 27, not migrated incrementally).

### Added

- `scripts/generate_ui_icons.py`: a Pillow-based offline icon
  renderer producing the same 15 semantic icons `ui/icons.h` names for
  the old shell (play, circle-alert, code, image, folder-open, save,
  copy, download, camera, info, circle, circle-stop, minus, square, x)
  as multi-scale (16/20/24/32px) PNGs under `assets/icons/ui/`.
- `ui/win32_icon_set.h`/`.cpp`: loads those PNGs via
  `Gdiplus::Bitmap` and composites them tinted (a GDI+ `ColorMatrix`
  remap onto a `D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE` render
  target's interop HDC) via `Gdiplus::Graphics::DrawImage`. The Phase
  24 title bar's hand-drawn minimize/maximize/close glyphs are now
  real bitmap icons (`minus`/`square`/`x`) drawn through it.
- `ui/win32_animation.h`/`.cpp`: a generic `AnimatedColor` easing
  helper (~100ms, ease-out-cubic), ticked once per frame from the main
  loop. Replaces the instant hover/active color swap on the title-bar
  buttons and tab-strip fill with a real interpolated transition.
- `ui/win32_status_dot.h`/`.cpp`: a compile-status indicator — pulses
  (accent color, sine-wave alpha) while a real `ShaderRunner::compile()`
  is in flight, behind a minimum-visible-duration state machine so the
  instant hardcoded-shader compile doesn't just flicker; shakes
  (decaying sine offset) on a real compile error; rings with a
  fading success pulse on a real compile success. Wired to genuine
  `F5` (recompile the default shader) / `Shift+F5` (recompile a
  deliberately broken fixture, to exercise the error path) key events
  — there's no "Run golf" action yet in this shell (Phase 26), so
  recompile is the closest real analog available now.
- Viewport shimmer placeholder: a second `ShaderRunner` renders an
  animated diagonal-band gradient shader in place of the real one
  during startup (before the first successful compile) and for a
  short window after every `WM_SIZE` resize, instead of a blank frame.
- `scripts/generate_app_icon_tiles.py`: regenerates
  `assets/icons/app_source.png` into a real 16/32/48/256px PNG tile
  set (`assets/icons/app/`) and rebuilds `app.ico`/`installer.ico` as
  proper multi-resolution `.ico` files (previously single-size 256px
  only). `docs/logo.png` is explicitly *not* regenerated — doing so
  "at higher resolution" needs real source art or an image-generation
  tool, neither available here; upscaling the existing file would add
  no genuine detail, so it stays deferred (documented reason, unlike
  Phase 10.6's silent skip).
- Verified live end-to-end on the running `.exe`: screenshotted the
  animated hover fill + bitmap icon tint on the title bar's close
  button, the blue compiling pulse → red error dot (via `Shift+F5`),
  the recovery → green dot with success-pulse ring (via `F5`), and
  confirmed the icon PNGs are copied next to the built executable by a
  new `ushader_win32_shell` `POST_BUILD` asset-copy step.

## [2.7.0] - 2026-07-22

Phase 24 (Win32 application shell, layout & Direct2D chrome). New
`ushader_win32_shell` build target — a real, running Win32/Direct2D
window with only the Viewport tab wired up so far. The existing
SDL3/ImGui `ushader` app is untouched and remains the shipping build;
this target grows panel-by-panel through Phase 26 before Phase 27
retires the SDL3 shell in its favor.

### Added

- `src/main_win32.cpp`: `WinMain`-equivalent scaffolding —
  `WNDCLASSEX` registration, `CreateWindowExW`, a standard `WM_*`
  message pump, no SDL3/Dear ImGui dependency.
- Custom title bar (`ui/win32_title_bar.h`/`.cpp`): client area
  extended into the caption via `DwmExtendFrameIntoClientArea` +
  `WM_NCCALCSIZE` returning an empty non-client region (forced to take
  effect immediately with `SetWindowPos(..., SWP_FRAMECHANGED, ...)`,
  otherwise the first `WM_NCCALCSIZE` call — sent with `wParam ==
  FALSE` during window creation — falls through to the OS-drawn
  caption and never gets replaced), plus manual `WM_NCHITTEST`
  drag-region and resize-border hit-testing and `WM_NCACTIVATE`
  handling so Windows doesn't repaint the native caption on
  activation. Direct2D/DirectWrite-drawn title text and
  minimize/maximize/close glyphs, hover-highlighted via `WM_NCMOUSEMOVE`.
- Tab-strip control (`ui/win32_tab_strip.h`/`.cpp`): Direct2D-painted,
  `2px`-rounded tab rectangles with DirectWrite labels, hover
  highlight on `WM_MOUSEMOVE`, focus ring on `WM_SETFOCUS`/
  `WM_KILLFOCUS`. Currently drives a single "Viewport" tab; the
  Source/Golfed/Trace/Diff tabs are wired in as their panels port over
  Phase 26.
- `ui/win32_theme_brushes.h`/`.cpp`: converts `theme_tokens.h`'s
  existing named color constants directly into `ID2D1SolidColorBrush`
  instances at render-target creation — no markup/ResourceDictionary
  translation step. Re-ran `scripts/check_contrast.py` against the
  same unchanged token values to re-verify WCAG AA compliance (all
  pairs still pass, 5.87:1 to 13.36:1).
- The Phase 23 `WglViewportHost`/`ShaderRunner` now host the live
  Viewport tab inside this shell, unmodified from Phase 23.
- Verified end-to-end on a live Windows desktop: launched the built
  `ushader_win32_shell.exe`, confirmed a single correctly-drawn custom
  title bar (no native/custom caption double-draw), confirmed
  maximize respects the taskbar work area, and confirmed the window
  closes cleanly via the custom close button hit-test region.
- `docs/screenshot_phase24_shell.png`: a chrome-progress capture of
  the new bare shell. `docs/screenshot.png` (the `README.md` hero
  image, which still depicts the full five-panel shipping app) is
  intentionally left untouched until Phase 27 gives the native shell
  full panel parity.

## [2.6.0] - 2026-07-22

Phase 23 (Rendering pipeline: native WGL inside a child HWND). First
phase in the Win32-native arc that ships real code, alongside the
existing SDL3/ImGui app which is untouched and still the shipping
shell — Phase 24 is what switches the shell over.

### Added

- `render/WglViewportHost` (`wgl_viewport_host.h`/`.cpp`): creates a
  child `HWND`, sets its pixel format via `ChoosePixelFormat`/
  `SetPixelFormat`, and creates a WGL context via `wglCreateContext`/
  `wglMakeCurrent`, upgraded to an OpenGL 3.3 core profile through
  `wglCreateContextAttribsARB` where available. Its `WM_SIZE` handler
  drives a `glViewport` update, the WGL-hosted equivalent of the SDL3
  `SDL_EVENT_WINDOW_RESIZED` path. `ShaderRunner`'s own GL call sites
  (compile, link, uniform upload, the fullscreen-triangle draw) are
  untouched.
- `render/gl_functions_wgl.cpp`: a `wglGetProcAddress`-based OpenGL
  function loader (falling back to `GetProcAddress` on `opengl32.dll`
  for pre-1.2 entry points) alongside the existing SDL-based loader,
  so the WGL-hosted path never links SDL3. The extern function-pointer
  storage itself moved to a new SDL-free `gl_functions_globals.cpp`,
  shared by both loaders.
- `render/shader_error_state.h`/`.cpp`: compile/link error parsing
  (line-number extraction from NVIDIA/Mesa-style GLSL compiler logs,
  display-message formatting) extracted out of `main.cpp`'s ImGui
  rendering code into a shell-agnostic `ShaderErrorState` struct and
  `make_shader_error_state()` builder, so the Phase 24 Win32/Direct2D
  shell can read shader errors without any ImGui-era code left to
  port. `main.cpp` now calls this shared function at every error
  call site instead of keeping its own copy of the parsing logic, and
  no longer echoes shader errors to stdout — the status dot and the
  new shell-agnostic struct are the only surfacing path now.
- `wgl_viewport_spike` (new build target, `wgl_viewport_spike_main.cpp`):
  a bare Win32 window hosting one child-`HWND` `WglViewportHost`
  viewport rendering the existing hardcoded default shader, animated
  via `iTime` and resizable — the standalone proof this phase's
  architecture decision holds before Phase 24 commits to building the
  real shell on top of it. No SDL3, no ImGui.
- `wgl_equivalence_test` (new build target,
  `tests/wgl_equivalence_test.cpp`): re-runs the Phase 15 automated
  multi-frame equivalence safety net (`run_equivalence_check`,
  bit-exact by default) against a `WglViewportHost`-hosted context
  instead of the SDL3-hosted one, proving the context-hosting swap
  introduces no pixel drift before any Direct2D shell work begins on
  top of it. Passes 5/5 samples bit-exact.

### Changed

- `ROADMAP.md` versioning note: continuing the Phase 22 drift, this
  phase ships as `2.6.0` rather than the `2.4.x` originally predicted
  for it.

Phase 22 (Win32-native migration: architecture decision & feasibility
spike). Decision-and-spike phase only — no UI panel is rewritten and
no runtime behavior changes; this release locks the architecture so
Phases 23–27 don't thrash.

### Changed

- Architecture decision: the shell rewrite planned since Phase 9/10
  targets raw **Win32 API in C++20** (`WNDCLASSEX`, `CreateWindowExW`,
  a standard `WM_*` message pump) with **GDI+, Direct2D, and
  DirectWrite** for owner-drawn chrome, rather than WinUI 3/Fluent
  Design — Fluent's Mica/Acrylic materials and the Windows App SDK
  runtime are unreliable or unsupported on Windows 10 LTSC 2019,
  conflicting with the "Strict Windows 10/11 compatibility" and
  "Offline-First Isolation" conventions. No C++/WinRT, no XAML, no
  .NET, and nothing new to bundle in the installer.
- Rendering strategy decision: the viewport moves to a native **WGL**
  context on a child `HWND`, not ANGLE and not a GLSL→HLSL/Direct3D11
  rewrite — `render/shader_runner.cpp`'s GL call sites, the Shadertoy
  uniform contract, and the Rust golfing engine (`rust-core/`,
  `include/ushader/golf_core.h`) are unaffected.
- Packaging is unchanged: still an unpackaged, self-contained `.exe`
  plus the existing Inno Setup installer — no MSIX, no Store.
- `ROADMAP.md` section 3 versioning note updated: the `2.3.x`–`2.7.x`
  prediction for Phases 22–26 had already drifted before this phase
  started (Phase 21 alone consumed `2.2.0`–`2.4.0` across its
  incremental releases), so this phase ships as `2.5.0` instead;
  `3.0.x` for Phase 27 remains the only hard version target in the
  arc.

## [2.4.0] - 2026-07-20

Closes out Phase 21 (Offline interop with other golf/shader tools) and
the Phase 12–21 arc as a whole.

### Added

- Phase 21: a "Build system integration" section in `README.md`'s
  Batch pipeline documentation, with a concrete MSBuild `PreBuildEvent`
  target (`golf.exe --budget ... --report ...`, failing the build on a
  size regression) and a plain `.bat` watch script wrapping the CLI's
  existing `--watch` mode — documentation only, no bundled MSBuild
  target or watcher service shipped.
- Phase 21: `--watch`, `--diff-only`, and `--protect`, three `golf.rs`
  CLI flags that existed in the binary and its `--help` text since
  Phase 17 but were never added to `ROADMAP.md`, `CHANGELOG.md`, or
  `README.md`, found during this release's final-acceptance pass and
  documented in all three now.

### Fixed

- Phase 12–21 final acceptance: removed source comments that had crept
  into `ui/theme.h`/`.cpp`, `ui/workspace.h`/`.cpp`,
  `render/shader_runner.h`/`.cpp`, and `main.cpp` during Phases 15 and
  20, in violation of the section 2 "no comments in the source code"
  convention. No behavior change; comment text only.
- `README.md`'s screenshot caption updated from "Source, Golfed, and
  Viewport panels" to also name the Trace and Diff panels added in
  Phases 14 and 18, which the caption had never been updated to
  mention. `docs/screenshot.png` itself is unchanged in this release —
  re-rendering it needs a live build on the Windows/MSVC/SDL3/OpenGL
  toolchain, tracked as a follow-up rather than silently assumed done.

## [2.3.0] - 2026-07-20

Phase 21 (Offline interop with other golf/shader tools); 3 of 5 items
done.

### Added

- Phase 21: `docs/ushaderprofile.schema.json`, a published JSON
  Schema for the `.ushaderprofile` format, and
  `docs/ushaderprofile-schema.md`, the accompanying human-readable
  spec (field table, compatibility rules, schema-version history), so
  external tooling can generate or consume profiles without
  reverse-engineering `golf_profile.cpp`.
- Phase 21: `.ushaderprofile` files now carry an explicit
  `"schema_version": 1` field, making the format actually versioned
  rather than only documented. Older profile files without the field
  keep loading unchanged and are treated as schema version 1.

## [2.2.0] - 2026-07-20

Starts Phase 21 (Offline interop with other golf/shader tools); 2 of
the phase's 5 items land in this release.

### Added

- Phase 21: an "Import exclude list..." button next to the "Protected
  names" field reads a Shader Minifier–style exclude-name list
  (plain text, one identifier per line) and merges any new names into
  the existing protected-names field, so an existing exclude list
  doesn't need retyping. Blank lines and `//`/`#`-prefixed comment
  lines in the file are skipped; names already in the field are not
  duplicated.
- Phase 21: three one-click export wrappers for the golfed output —
  "Copy as Shadertoy", "Copy as Bonzomatic", and "Copy as bare
  main()" — in the Golfed panel toolbar and the command palette, each
  copying a fixed local string template straight to the clipboard.
  The Shadertoy and Bonzomatic variants are the golfed text as-is
  (both tools auto-supply the same uniform set and call `mainImage`
  themselves); the bare `void main()` variant rewrites the shader into
  a standalone fragment shader that declares its own uniforms and
  calls `mainImage` from a plain `void main()`, for pasting into raw
  GLSL sandboxes that don't understand the Shadertoy API. None of this
  talks to any of those tools over a network — every wrapper is a
  local string transform, per the Offline-First Isolation rule.

## [2.1.0] - 2026-07-20

Closes out Phase 20 (Display correctness & accessibility) in full.

### Added

- Phase 20: minimal Windows UI Automation support. Title-bar
  minimize/maximize/close buttons, themed checkboxes, and icon
  buttons now register a name, `Button`/`CheckBox` control type, and
  screen-space bounding rect with a new UIA provider bridge
  (`platform/accessibility.cpp`), so a screen reader such as Narrator
  can announce them. This is a "screen-reader-nameable" baseline, not
  full keyboard-driven accessibility: elements report
  `IsKeyboardFocusable = false` and don't support `Invoke`/`Toggle`
  activation, since Dear ImGui's immediate-mode click handling can't
  be driven from a UIA callback without a much larger restructuring.
- Phase 20: `scripts/check_contrast.py`, a WCAG AA relative-luminance
  contrast checker re-running the same method used in Phase 10.1/10.8
  (previously run ad hoc, not checked into the repo), now parsing
  `theme_tokens.h` directly. Re-confirms all four `text.primary`/
  `text.secondary` vs `bg.panel`/`bg.app` pairs pass AA (12.59:1,
  13.36:1, 5.87:1, 6.22:1). The colorblind-safe status indicators
  (Phase 20.3) reuse the same color tokens and only change shape, so
  there's no separate colorblind token set to check independently —
  the script documents that explicitly rather than silently skipping
  the second pass.
- Phase 20: colorblind-safe status indicators. A new "Colorblind-safe
  status indicators" checkbox in the Appearance tab (off by default)
  makes the equivalence-check and shader-compile status dots
  shape-differentiated — filled circle for Ok, triangle for Warning,
  square for Error — instead of the same circle for all three. The
  underlying `status.ok`/`status.warning`/`status.error` color tokens
  are unchanged; only the shape changes when the toggle is on.
- Phase 20: user-adjustable base UI font size (13–28pt), set from a new
  "Appearance" tab next to About/Keyboard Shortcuts. Combines
  multiplicatively with the per-monitor DPI scale (also Phase 20) into
  a single style/font scale factor, so panel paddings and line-heights
  grow proportionally with the text rather than the glyphs alone.
  Persisted app-wide (not per-tab) as `ui_font_size` in the existing
  `last_session.ushaderworkspace` file, applied immediately at startup
  ahead of the tab-restore confirmation.
- Phase 20: correct per-monitor DPI handling. The window is now created
  with `SDL_WINDOW_HIGH_PIXEL_DENSITY`; ImGui's style metrics and
  `FontGlobalScale` are recomputed from the per-monitor content scale
  at startup and again whenever the window is dragged to a display with
  a different scale factor (`SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED`),
  via a new `apply_dpi_scale()` in `ui/theme.cpp`.

### Changed

- Closed out the Phase 19 "Optional PDF variant" roadmap item: no
  embeddable, offline-capable HTML-to-PDF renderer could be vendored
  without either violating Offline-First Isolation (wkhtmltopdf's
  bundled, unmaintained QtWebKit) or being disproportionate to a single
  optional export format (a full Chromium/CEF embed; WeasyPrint's
  native dependency stack has no lightweight vendorable Windows
  build). Per the roadmap's own documented fallback, the bullet is
  dropped in favor of documenting "print the HTML report from any
  browser (`Ctrl+P` → Save as PDF)" in `README.md`. No code changes;
  the session-report feature (HTML only) is unchanged.

## [2.0.1] - 2026-07-20

### Added

- Drag-and-drop `.glsl` files onto the main window to open each as a
  new tab, reusing the existing Phase 16 tab strip.
- Recent Files list under the File menu, backed by the same
  `%APPDATA%\ushader\` store used for keybindings. Selecting an entry
  reopens it as a new tab; entries pointing at files that no longer
  exist on disk are pruned automatically. Includes a "Clear Recent
  Files" action.
- Opening or saving a file (via the Open/Save/Save as dialogs, the
  new drag-and-drop path, or a Recent Files entry) now records that
  path in the Recent Files list.

This completes the last outstanding Phase 18 item (v1.9.x); Phase 19
had already shipped as 2.0.0 before this item was finished, so this
lands as a 2.0.x patch rather than 1.9.2.

## [2.0.0] - 2026-07-19

### Added

- Local session reports (Phase 19): "Export report..." in the File
  menu and the command palette writes a single self-contained HTML
  file with no external references — it opens correctly with zero
  network access, including on an offline judge's machine. The report
  includes the source and golfed code (both syntax-highlighted with
  the same rules the editors use, golfed code shown formatted for
  readability), the size/budget summary and per-pass counters, and
  the automated equivalence-check result.
- Optional embedded viewport screenshot in the report, reusing the
  existing PNG capture path, inlined as a base64 image. Off by default
  via a new "Include screenshot in report" checkbox next to the
  Screenshot button, to keep the report file small when not wanted.

## [1.9.1] - 2026-07-19

### Added

- Diff panel: a "Diff" tab alongside Trace shows a token-level unified
  diff between Source and Golfed, with removed spans struck through in
  the error color and added spans in the ok color. Built directly on
  the Phase 14 trace data's overall before/after pair, with its own
  small LCS-based token diff (falling back to a line-level diff for
  very large inputs) rather than a new diff engine.
- "Show Diff panel" added to the command palette.

## [1.9.0] - 2026-07-19

### Added

- Command palette (`Ctrl+Shift+P`, Premiere/VS-Code convention): a
  fuzzy-searchable list dispatching to every existing action (run
  golf, toggle a pass, load/save profile, switch tab, toggle Compare
  mode, export), plus a new "Switch to tab: …" entry per open tab and
  a "Toggle Minimap" entry.
- Rebindable keyboard shortcuts: the command palette, new tab, open,
  save, and close-tab shortcuts can now be rebound from a new
  "Keyboard Shortcuts" tab next to "About", and are persisted to
  `%APPDATA%\ushader\keybindings.json`. Hardcoded defaults are used
  whenever the file is absent or malformed, so the app is never left
  without shortcuts.
- Minimap for the Source and Golfed editors: a compact colored strip
  reusing the same GLSL keyword/identifier tables and syntax palette
  as the editors themselves, off by default and only shown once a tab
  toggles it on and the file is at or above a configurable line-count
  threshold.

## [1.8.0] - 2026-07-19

### Added

- Batch pipeline mode for the `golf` CLI (Phase 17), for embedding
  µShader in an offline asset pipeline. Passing a directory or a glob
  (containing `*` or `?`) golfs every `.glsl` file found — reusing the
  exact `golf_with_protected_names` entry point the GUI calls, so CLI
  and GUI output can never diverge — and writes each result next to its
  input as `<name>.min.glsl` (already-golfed `.min.glsl` files are
  skipped so re-runs are idempotent). Single-file and stdin usage is
  unchanged.
- `--profile <path.ushaderprofile>`: load the exact pass toggles,
  protected-names list and budget preset an artist last saved in the
  GUI as the base configuration; explicit CLI flags still override it.
- `--budget <preset>`: consumes the Phase 12 budget presets and exits
  with a non-zero status when any file's estimate exceeds the
  threshold — the mechanism a CI job needs to fail a build on a size
  regression.
- `--report <path.json|path.csv>`: machine-readable per-file report
  (input path, raw/golfed/compressed byte counts, per-pass
  `AggressiveStats` counters, and pass/fail against the budget) for
  pipeline dashboards.
- `--diff`: prints a unified source/golfed diff per file to stdout
  instead of writing golfed output, for a pre-commit dry-run.
- `--pretty`: opts into colored, human-oriented console output. By
  default the CLI emits no ANSI color or progress spinners so its
  output stays clean when redirected into CI logs.

## [1.7.0] - 2026-07-19

### Added

- Multi-document workspace (Phase 16). A tab strip above the
  Source/Golfed/Viewport dock holds one tab per open `.glsl` file,
  reusing the flat-rectangle tab language with an accent-underlined
  active tab. Each tab owns its own editor buffer, golf result,
  compile error, pass toggles, protected-names list, budget preset,
  golf/budget stats, pass trace, and equivalence result. Switching
  tabs recompiles only the newly-active tab's GL programs and never
  re-runs the golf engine on the tab you left. A trailing `+` tab
  button and a new `File` menu (`New tab` Ctrl+N, `Open` Ctrl+O,
  `Save` Ctrl+S, `Save as...`, `Close tab` Ctrl+W, `Exit`) manage the
  open set.
- Session persistence (Phase 16). On clean exit the workspace is
  auto-saved to
  `%APPDATA%\ushader\last_session.ushaderworkspace` — a JSON superset
  of the `.ushaderprofile` format storing every open file path, the
  active tab index, and each tab's pass toggles, protected names and
  budget preset. The Dear ImGui panel layout is persisted alongside it
  to `%APPDATA%\ushader\layout.ini` and referenced by name. On next
  launch a "Restore last session?" prompt offers to reopen those files
  (or start fresh); files are never silently reopened.
- Unsaved-changes indicators (Phase 16). A dot marks any tab whose
  source differs from its saved contents, closing a dirty tab asks to
  save/discard/cancel, and exiting with unsaved shaders opens a
  confirmation dialog listing every dirty tab before quitting.

## [1.6.0] - 2026-07-19

### Added

- Automated multi-frame equivalence safety net (Phase 15, in progress),
  internal to `render/shader_runner` for now. `EquivalenceSampleConfig`
  holds a configurable, user-editable list of `iTime` sample points
  (defaulting to `0.0, 0.5, 1.0, 2.5, 5.0`) and a fixed sample
  resolution; `run_equivalence_samples()` renders the already-compiled
  source and golfed programs Compare mode uses at every sample `iTime`
  into a caller-owned scratch framebuffer pair, invoking a callback
  with the rendered pair per sample. Every uniform besides `iTime` —
  `iResolution`, `iMouse`, and also `iFrame`/`iFrameRate`/`iDate` — is
  pinned to a fixed value on every sample and every run, so repeated
  equivalence checks of the same program pair always compare identical
  inputs. No pixel readback/diffing or UI surface yet — this lands the
  offscreen dual-render foundation the upcoming per-sample
  `glReadPixels` diff and pass/fail indicator will build on.

## [1.5.0] - 2026-07-19

### Added

- Pass-by-pass golf trace (Phase 14, in progress), internal to
  `rust-core` for now. The golfing engine can now optionally record a
  before/after source snapshot and a change count for every
  transformation pass it runs, in fixpoint-iteration order, without
  any cost to the normal (untraced) golfing path used everywhere in
  the app today. No user-visible UI or CLI surface yet — this lands
  the `rust-core` foundation the upcoming "Trace" tab and
  `--diff`-style tooling will build on.
- `ushader_golf_traced`, a new C ABI entry point built on the above:
  golfs a shader exactly like `ushader_golf`, but also fills an
  out-parameter with a serialized JSON trace of every enabled pass
  considered, in order, each with its before/after source and change
  count. Still no user-visible UI or CLI surface — this is the C ABI
  bridge the upcoming "Trace" tab will call into.
- A new "Trace" tab, docked alongside Source/Golfed/Viewport: a
  collapsible list of every golfing pass considered on the last run,
  with its change count. Expanding a pass shows a side-by-side
  before/after view of that pass's own delta only, syntax-highlighted
  the same way as the Source and Golfed editors. Passes that made no
  change are still listed, grayed out, so the trace is always a
  complete record of what the golfer considered.
- `fixtures/golf_trace.glsl`: a regression fixture for the Phase 14
  trace, plus a new `trace_pass_order_and_counts_match_fixture_regression`
  Rust unit test in `golfer.rs` that asserts its exact pass-order and
  per-pass-count sequence (two fixpoint iterations, 32 steps total).
  Guards against the fixpoint loop in `golf_with_protected_names_impl`
  silently reordering, adding, or dropping a pass — a regression none
  of the existing per-pass unit tests would catch on their own.

## [1.4.1] - 2026-07-19

### Added

- `fixtures/sample.ushaderprofile`: a checked-in sample profile (a
  mixed, `Custom` set of pass toggles, a protected-names list, and a
  named budget preset) plus a new
  `tests/golf_profile_roundtrip_test.cpp` executable that saves,
  reloads, and re-checks it — confirming every `AggressiveOptions`
  pass toggle, the protected-names list, and the selected budget
  preset survive a `.ushaderprofile` save/reload cycle unchanged, and
  that the reloaded options still golf correctly through the real
  Rust `rust-core` engine.

## [1.4.0] - 2026-07-19

### Added

- Golfing profiles (Phase 13, in progress). The golf controls panel
  gained "Save profile…" / "Load profile…" buttons that write/read a
  `.ushaderprofile` JSON file capturing every pass toggle, the
  protected-names list, and the selected Phase 12 budget preset, so a
  competitive-golf configuration can be reused across sessions instead
  of re-toggling every checkbox. Uses the existing native file-dialog
  integration; no new dialog code path.
- A new "Profile" combo alongside the pass toggles offers three
  built-in, read-only presets — `Maximum` (every pass on), `Safe`
  (dead-code elimination only, no algebraic/CSE rewrites), and `None`
  (aggressive mode off) — and reflects `Custom` whenever the current
  toggles (including ones loaded from a file) don't match any of the
  three.
- The last `.ushaderprofile` path used with `Save profile…`/`Load
  profile…` is now remembered across restarts (stored in
  `%APPDATA%\ushader\last_profile_path.txt`) and pre-fills both
  dialogs on the next launch. This remembers the path only — it does
  not auto-load the profile's contents at startup, so no golf setting
  changes without an explicit `Load profile…` click. A fixtures
  round-trip test is tracked separately in `ROADMAP.md` and not yet
  implemented.

## [1.3.1] - 2026-07-19

### Changed

- The application now launches maximized (`SDL_MaximizeWindow` called
  right after window creation) instead of opening at the fixed
  1280x720 default size. The existing title-bar maximize/restore
  toggle is unchanged; the window can still be restored to its
  windowed size the same way as before.

## [1.3.0] - 2026-07-19

### Added

- Compression-aware size budgets (Phase 12). The stats panel no
  longer measures golf size only in raw characters/bytes: every golf
  run now also estimates the DEFLATE-compressed size (fixed-Huffman,
  RFC 1951, computed entirely in `rust-core`, no external compression
  library and no network access) since that is what competitive
  code-golf and demoscene size limits actually judge.
- New "Budget preset" combo in the golf controls panel: `Shadertoy`,
  `X/Twitter shader`, `JS13K-style 13KB`, `4KB intro`, `8KB intro`,
  `64KB intro`. The stats panel shows a raw-byte badge and/or a
  compressed-byte badge against the selected preset's threshold,
  colored green/amber/red as the estimate sits under, near, or over
  budget.
- New C ABI entry point `ushader_estimate_budget` (declared in the
  regenerated `include/ushader/golf_core.h`) returning both the raw
  and DEFLATE-estimated byte counts for a golfed shader.

## [1.2.5] - 2026-07-19

### Added

- New golf pass: common subexpression elimination. When two
  declarations in a straight-line run initialize with a
  token-identical pure expression, the later ones are rewritten to
  reference the first variable instead of recomputing, e.g.
  `float a=dot(p,p),b=dot(p,p);` becomes `float a=dot(p,p),b=a;`.
  New "Common subexpressions" toggle in the Passes panel and matching
  per-pass counter. Deliberately narrow in scope for safety: only
  whole declaration initializers (never a sub-expression), only
  built from a fixed whitelist of pure builtins (never a user
  function), and the candidate cache is cleared on every block
  boundary and on any statement that isn't itself a clean matching
  declaration.
- Found and fixed two real correctness bugs in this new pass during
  development (both now covered by regression tests): a variable-name
  lookup that read the pre-rename identifier instead of the actual
  rendered name, and a cache-invalidation check that missed the
  common case of a brace following `)` or `else` rather than `;`.

## [1.2.4] - 2026-07-18

### Added

- New golf pass: algebraic identity simplification. Removes
  multiplicative/additive identities (`x*1`, `1*x`, `x/1`, `x+0`,
  `0+x`, `x-0`) on bare identifiers, and rewrites `pow(x,2.)` to
  `x*x`, with its own toggle ("Algebraic identities") in the Passes
  panel. Restricted to single-identifier operands so it can never
  duplicate or drop an expression with a side effect; numeric-literal
  operands are deliberately left to the existing constant-folding
  passes, which already handle negative-zero edge cases correctly.

## [1.2.3] - 2026-07-18

### Changed

- Checkboxes (all 16 of them, across the golf-pass toggles, "Aggressive
  golf", "Formatted view", and "Compare") are now a bespoke flat 12px
  square matching the Phase 10 dark theme spec, instead of the default
  ImGui circular-ish check — `border.subtle` outline idle, `accent`
  outline on hover, solid `accent` fill with a hand-drawn white check
  mark when active.
- The "Protected names" text field now shows a 1px accent focus ring
  while being edited.

## [1.2.2] - 2026-07-18

### Changed

- "Run golf" is now the one accent-filled primary button in the UI
  (solid `accent` background, hover/active accent shades), matching
  Phase 10's design intent that it be the single clearly-primary
  surface — every other button now reads as visually secondary by
  comparison.

## [1.2.1] - 2026-07-18

### Changed

- Full dark UI/UX overhaul modeled on Adobe Premiere Pro's editing
  workspace, replacing the Phase 3 white theme entirely (no
  light/dark toggle — one theme, deliberately, as before):
  - New design-token table (`src/ui/theme_tokens.h`) backing every
    color used anywhere in the UI.
  - Compact chrome: 2px corner radius everywhere, thin flat
    scrollbars, hover-only resize splitters, flat rectangular tabs
    with a 2px accent underline on the active tab.
  - Custom borderless window chrome: SDL3 borderless window with a
    hand-rolled hit-test callback for dragging/resizing, plus a
    custom title bar (app icon, title, minimize/maximize/close).
  - Dark `TextEditor` palette for the Source/Golfed panels.
  - Viewport restyled as a Program Monitor: black letterboxing,
    compile-status dot (green/red) replacing the old text banner, and
    a timecode readout.
  - Icons now shift color between idle/hover/active states.
  - About popup rebuilt as a centered dark card.
  - New design references under `docs/design/` (color palette, full
    UI mockup, icon states) and a WCAG AA contrast pass on the
    text/background token pairs.

### Fixed

- The application could crash (`abort()`, debug CRT error) when
  minimized: the custom title bar and dock-host windows were sized
  from the OS-reported viewport size every frame, which can go to
  zero while minimized, feeding a negative height into Dear ImGui's
  docking system. UI building is now skipped entirely for frames
  where the window is minimized or has a degenerate size.

## [1.2.0] - 2026-07-18

### Changed

- MP4/WebM viewport recording no longer depends on a system-installed
  `ffmpeg` on `PATH`. A static `ffmpeg.exe` (GPL build from
  `BtbN/FFmpeg-Builds`) is now fetched at build time, copied next to
  `ushader.exe`, and included in the installer, so MP4/WebM work out
  of the box on a fresh install. The Record UI falls back to
  disabling MP4/WebM (with a tooltip) only if the bundled binary is
  somehow missing.
- Added `THIRD_PARTY_NOTICES.md` documenting the bundled FFmpeg (GPL)
  binary, referenced from the About popup.

## [1.1.0] - 2026-07-18

### Added

- Viewport recording: capture the running shader directly to an
  animated GIF, or to MP4/WebM when `ffmpeg` is available on `PATH`.
  A Record/Stop control and format selector sit in the Viewport
  panel toolbar; MP4/WebM are grayed out with an explanatory tooltip
  when `ffmpeg` isn't found.

### Fixed

- `Open`/`Save` for `.glsl` files used narrow-path `std::ifstream`/
  `std::ofstream`, which silently fail on Windows for paths containing
  non-ASCII characters (e.g. this project's own `µShader` folder).
  Replaced with UTF-8-aware helpers built on `_wfopen_s`; verified
  with a round-trip write/read through a `µ`-containing path.
- The application launched with a console window behind the main
  window. Switched the executable to the Windows subsystem so only
  the GUI window appears.
- Each docked panel (Source, Golfed, Viewport) showed a small
  dropdown arrow exposing a "Hide tab bar" menu that could leave a
  panel without a visible tab. Disabled that menu button on all
  panel dock nodes.

## [1.0.0] - 2026-07-18

First stable release.

### Added

- Inno Setup installer (`installer/ushader.iss`), producing
  `uShader-Setup-<version>.exe` from the stamped `VERSION` file and
  the Phase 7 icons. Unsigned — no code-signing certificate is
  available in the build environment used for this release.
- Verified end to end on Windows 10 (LTSC 2019, build 17763): built,
  installed via the Inno Setup installer, launched from the installed
  location, and exercised across every panel. Not independently
  verified on Windows 11.

## [0.8.0] - 2026-07-18

### Added

- "About" popup (menu bar -> About): logo, version, copyright, and
  clickable email/website/repository links.
- Application and installer icons (`assets/icons/app.ico`,
  `assets/icons/installer.ico`), embedded into the executable via a
  Windows resource script.

### Fixed

- File paths containing non-ASCII characters (e.g. the project's own
  `µShader` folder) were silently failing to load or save through
  `stb_image`/`stb_image_write`, which default to the ANSI code page
  on Windows instead of UTF-8. This affected the About logo and could
  have affected PNG screenshot export depending on where a user saved
  it. Fixed by enabling `STBI_WINDOWS_UTF8`/`STBIW_WINDOWS_UTF8`.

## [0.7.0] - 2026-07-18

### Added

- Open/Save buttons in the Source panel using native Windows file
  dialogs, for loading and saving `.glsl` shader source.
- Copy button in the Golfed panel: copies the golfed output (always
  the raw minified text, regardless of "Formatted view") to the
  clipboard.
- Export (Shadertoy) button: saves the golfed output to a `.glsl`
  file — Shadertoy's format is just the `mainImage` function body, so
  no extra wrapping is needed.
- Screenshot button in the Viewport panel: captures the golfed
  shader's current frame via `glReadPixels` and writes it to a PNG
  file with `stb_image_write`.

## [0.6.0] - 2026-07-18

### Added

- Golfing controls in the Source panel: an "Aggressive golf" master
  toggle, individual checkboxes for each of the 14 aggressive
  transformation passes, and a protected-names field (comma-separated
  identifiers the engine will never rename).
- Reduction stats panel in the Golfed panel: input/output char counts
  and percentage reduction, golfed byte size, renamed/numbers-shortened
  counts, a per-pass counter breakdown, and size-budget badges (280,
  512, 1024 bytes).
- "Compare" mode in the Viewport panel: renders the Source and Golfed
  shaders side by side, each in its own offscreen framebuffer, to
  visually confirm golfing didn't change the output.
- The Rust `capi` now returns full golf statistics (`UshaderGolfStats`)
  alongside the golfed code, not just the code string.

## [0.5.0] - 2026-07-18

### Added

- A real text-editor widget (ImGuiColorTextEdit) replaces the plain
  text boxes in the Source and Golfed panels, with a custom GLSL
  language definition covering real keywords, types, qualifiers,
  built-in functions/variables, and preprocessor directives (the
  library's own bundled "GLSL" definition was actually generic C
  vocabulary, so it was not used).
- "Formatted view" toggle on the Golfed panel: re-indents the golfed
  one-liner across multiple lines for readability. Display only — the
  code that gets golfed, compiled, and (later) copied stays the
  minified version.
- Error-line highlighting in the Source editor: on a compile failure,
  the offending line is parsed out of the driver's error log and
  highlighted directly in the editor, correctly mapped back from the
  wrapped fragment shader's line numbering to the user's source lines.

## [0.4.0] - 2026-07-18

### Added

- Dear ImGui docking UI shell integrated into the SDL3/OpenGL window,
  with a custom white theme: an embedded Inter font, rounded corners,
  generous padding, and a restrained accent color palette.
- A three-panel Source / Golfed / Viewport dockable layout that
  automatically collapses into tabs under a narrow window.
- The "Run golf" action calls into the Phase 1 Rust engine through
  the C ABI bridge and displays the golfed output.
- The viewport renders the golfed shader into an offscreen framebuffer
  and displays it live via `ImGui::Image`; shader compile/link errors
  are surfaced in the UI with the offending source line parsed out of
  the driver's error log where possible.
- A small vector icon set (Lucide, `assets/fonts/lucide.ttf`) rendered
  through ImGui's font atlas.

## [0.3.0] - 2026-07-18

### Added

- OpenGL 3.3 core context on the SDL3 window, with a minimal hand-rolled
  function loader for the core-profile entry points not statically
  exported on Windows.
- `ShaderRunner`: a fullscreen-triangle Shadertoy-style single-pass
  renderer (`src/render/`) — compiles a user `mainImage` fragment body
  wrapped with the standard uniform set (`iTime`, `iResolution`,
  `iMouse`, `iDate`, `iFrame`, `iFrameRate`) and draws it with no
  vertex buffers (`gl_VertexID`-driven fullscreen triangle).
- A hardcoded default shader renders and animates on startup; window
  resizing updates the GL viewport.
- Shader compile/link errors are reported to stdout.

## [0.2.0] - 2026-07-18

### Added

- `rust-core/`: a tokenizer-based GLSL minification engine (identifier
  renaming, numeric literal shortening, whitespace stripping, and
  aggressive passes: dead-code elimination, constant folding,
  declaration merging, and more).
- `capi` feature (`rust-core/src/capi.rs`) exposing the engine through
  a small `extern "C"` surface (`ushader_golf`, `ushader_free_string`).
- `include/ushader/golf_core.h`, generated with `cbindgen`.
- `fixtures/*.glsl`: a regression corpus covering each transformation
  pass, used to verify golf output.
- `tests/rust_core_smoke_test.cpp`: a console test target exercising
  the C ABI bridge end to end.
- CMake now builds `rust-core` via Cargo and links it into both the
  smoke test and the main application.

## [0.1.0] - 2026-07-18

### Added

- Repository structure bootstrapped: `LICENSE` (MIT), `README.md`,
  `CHANGELOG.md`, `ROADMAP.md`, `.gitignore`, `VERSION`.
- CMake skeleton (C++20, MSVC toolchain, Windows 10/11 only) with
  version stamping from the `VERSION` file into a generated
  `version.h`.
- Dependency fetching wired up via CMake `FetchContent`: SDL3, Dear
  ImGui (docking branch), stb single-header libraries.
- First buildable milestone: an application that opens an SDL3 window
  with an OpenGL context and closes cleanly.
