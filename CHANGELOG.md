# Changelog

All notable changes to µShader are documented in this file.

## [Unreleased]

## [2.6.0] - 2026-07-20

Continues Phase 22 (WinUI 3 migration: architecture decision &
feasibility spike); 2 of 8 items done.

### Added

- Phase 22: pinned the **Windows App SDK channel** to **2.2.0**
  (Stable, released 2026-06-09) — the latest non-preview release at
  decision time, confirmed against Microsoft's release-channels page
  (supersedes 2.1.3, the prior latest stable). Deployment mode is
  self-contained (runtime packaged with the app, no separate system
  install), matching Offline-First Isolation and the existing Inno
  Setup "no separate runtime install" user experience. No runtime code
  changes in this release — decision-only, per Phase 22's scope.

## [2.5.0] - 2026-07-20

Starts Phase 22 (WinUI 3 migration: architecture decision & feasibility
spike); 1 of 8 items done.

### Added

- Phase 22: architecture decision locked — the WinUI 3 rewrite (Phases
  22–27) will use **C++/WinRT**, not C#/.NET. C#/.NET was rejected
  despite its faster XAML iteration loop because it would force the
  existing direct, zero-marshaling call into `rust-core`'s C ABI
  (`ushader_golf`, `ushader_golf_traced`, `ushader_estimate_budget`, …)
  through P/Invoke, and would add the .NET runtime as a new dependency
  the app has never had — in tension with Offline-First Isolation's
  "embedded locally" spirit even where .NET can be self-contained-
  deployed. C++/WinRT keeps `main.cpp` calling into
  `include/ushader/golf_core.h` exactly as today; only the UI shell
  above it changes in later phases. No runtime code changes in this
  release — decision-only, per Phase 22's scope.

### Fixed

- `ROADMAP.md` section 3: documented that Phase 21's per-item releases
  already consumed `2.2.x`–`2.4.x` instead of stopping at `2.2.x`, so
  Phase 22 starts at `2.5.x` rather than the `2.3.x` the versioning
  scheme's Phase 22–27 note predicted. The original prediction text is
  kept as historical record rather than silently edited.

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
