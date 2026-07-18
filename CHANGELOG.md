# Changelog

All notable changes to µShader are documented in this file.

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
