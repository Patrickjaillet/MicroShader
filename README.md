# µShader

Native Windows 10/11 GLSL shader golfer — minify a Shadertoy-style
`mainImage` fragment shader and preview the result live.

µShader pairs a tokenizer-based Rust minification engine with a
native Win32 + OpenGL (WGL) + Direct2D/DirectWrite/GDI+ application
shell — no Dear ImGui, no SDL3, nothing beyond APIs already in-box on
every Windows 10/11 edition, including Windows 10 LTSC 2019: paste a
shader, golf it, and immediately verify it renders identically in the
live viewport. The UI is a dark, tabbed editing workspace with a
custom borderless window frame.

![uShader screenshot: Source, Golfed, and other panel tabs](docs/screenshot.png)

## Features

- **Golfing engine**: identifier renaming, numeric literal shortening,
  and whitespace stripping always run; an aggressive mode adds 16
  further transformation passes (dead-code elimination, constant
  folding, declaration merging, function inlining, algebraic identity
  simplification, common subexpression elimination, and more), each
  individually toggleable, plus a protected-names list for identifiers
  that must never be renamed. Every pass is designed to never change
  shader behavior — each ships with its own regression fixture and
  Rust unit tests.
- **Live viewport** with the standard Shadertoy uniform set (`iTime`,
  `iResolution`, `iMouse`, `iDate`, `iFrame`, `iFrameRate`), and a
  Compare mode (`Ctrl+Shift+C`) that renders the source and golfed
  shaders side by side to confirm golfing didn't change the output.
- **GLSL-aware text editor** (syntax highlighting, error-line
  highlighting on compile failure) for both the Source and Golfed
  panels, with a "Formatted view" toggle (`Ctrl+Shift+F`) for reading
  the golfed one-liner across multiple lines.
- **Reduction stats**: char/byte counts, reduction percentage,
  per-pass counters, and size-budget badges (280/512/1024 bytes) via
  a click-to-cycle budget-preset selector.
- **Pass-by-pass trace**: a "Trace" tab lists every golfing pass
  considered on the last run, with its change count; expanding a pass
  shows a side-by-side before/after view of that one pass's own delta,
  syntax-highlighted the same way as the Source and Golfed editors.
  Passes that made no change stay listed, grayed out, so the trace is
  a complete record rather than only the passes that fired.
- **Diff panel**: tabbed alongside Trace, an inline unified diff shows
  exactly what changed between Source and Golfed as a whole, with
  removed text struck through in red and added text in green.
- **Open/Save and drag-and-drop**: open or save a single `.glsl` file
  (`Ctrl+O` / `Ctrl+S` by default, rebindable), or drag one onto the
  main window; a "Recent Files" list in the command palette tracks
  previously opened/saved shaders, persisted to
  `%APPDATA%\ushader\recent_files.json`, and prunes entries pointing
  at files that no longer exist.
- **Command palette** (`Ctrl+Shift+P` by default): a fuzzy-searchable
  list of every action in the app — run golf, switch tabs, toggle
  Formatted/Compare view, open/save, and recent files.
- **Rebindable keyboard shortcuts** for the command palette, open,
  save, and new-shader chords, persisted to
  `%APPDATA%\ushader\keybindings.json`.
- **Minimap**: a compact colored overview of the Source and Golfed
  editors, syntax-colored the same way as the editors themselves, for
  jumping around longer shaders.
- **Display correctness & accessibility**: a 13–28pt UI text-size
  slider and an off-by-default "Colorblind-safe status indicators"
  toggle (shape-differentiated instead of same-shaped colored status)
  live in the Appearance tab. Every owner-drawn control — title-bar
  buttons, tabs, checkboxes, sliders, links — exposes a name, role,
  location, and (where applicable) live toggle state to Windows UI
  Automation clients (e.g. Narrator), not just a build-time claim:
  verified against the real `System.Windows.Automation` client API.

### Not yet in the native shell

A handful of features from earlier (now-retired) ImGui-based releases
have not been ported to the Win32 shell yet and are tracked as
follow-up work rather than silently dropped: golfing profiles
(`.ushaderprofile` save/load), a multi-document workspace (multiple
open shaders as separate tabs) and the session-restore-on-launch
behavior that depended on it, one-click "Copy as Shadertoy / Bonzomatic
/ bare main()" clipboard exports, importing a Shader Minifier–style
exclude-name list, self-contained HTML session reports, PNG viewport
screenshots, and GIF/MP4/WebM viewport recording. See `ROADMAP.md`
Phase 28 for the tracked plan.

## Batch pipeline (CLI)

Alongside the GUI, `rust-core` builds a `golf` command-line tool for
embedding µShader in an offline asset pipeline. Given a single file (or
stdin) it prints the golfed shader to stdout; given a directory or a
glob it golfs every `.glsl` file found — using the exact same engine
entry point as the GUI, so the two can never diverge — and writes each
result next to its input as `<name>.min.glsl`.

```bash
cargo build --release --manifest-path rust-core/Cargo.toml --bin golf

golf shader.glsl > shader.min.glsl
golf -a "shaders/**/*.glsl"
golf --profile studio.ushaderprofile --budget "4KB intro" \
     --report sizes.json shaders/
golf --diff shader.glsl
```

Key flags: `--profile` loads a `.ushaderprofile` (produced by an
earlier release's GUI, or hand-written against the published schema —
see below), `--budget <preset>` exits non-zero when any file exceeds
the size threshold (for failing a CI build on regression), `--report
<path.json|path.csv>` writes a machine-readable per-file report,
`--diff` prints a unified source/golfed diff for a dry-run,
`--diff-only` prints just the stats summary instead of the golfed
code (for scripts that only want pass/fail-style output), `--protect
NAMES` adds a comma-separated list of identifiers to never rename on
top of any `--profile` list, `--watch FILE` re-golfs a single file to
stdout every time it changes on disk (polling every 300ms, stop with
Ctrl+C — for a local live-reload loop during authoring, not a
build-system integration), and `--pretty` opts into colored output
(off by default so CI logs stay clean). Run `golf --help` for the
full list. The `.ushaderprofile` format is a published, versioned
JSON schema — see
[`docs/ushaderprofile-schema.md`](docs/ushaderprofile-schema.md) — so
other tooling can read or write profiles directly even while the GUI
itself doesn't yet.

### Build system integration

`golf.exe` is a plain console executable with a scriptable exit code
(non-zero on a budget failure or a read/parse error), so it drops into
any local build pipeline without µShader bundling a build-system
plugin or a file-watcher service itself — the two examples below are
documentation only.

**MSBuild pre-build step.** Add a `PreBuildEvent` to a `.vcxproj` (or
the equivalent target in a `.csproj`) that golfs every shader under
the project and fails the build if any file busts its size budget:

```xml
<Target Name="GolfShaders" BeforeTargets="PreBuildEvent">
  <Exec Command="golf.exe --budget &quot;4KB intro&quot; --report &quot;$(ProjectDir)shaders\golf_report.json&quot; &quot;$(ProjectDir)shaders&quot;"
        WorkingDirectory="$(ProjectDir)" />
</Target>
```

`golf.exe` must be reachable from the project's `PATH`, or replace
`golf.exe` above with the full path to the built binary (for example
`$(ProjectDir)tools\golf.exe`). Because `--report` writes before
`--budget` can fail the step, `shaders\golf_report.json` still lands on
disk for inspection even when the build stops on a regression.

**Plain `.bat` watch script.** `golf.exe` already has a built-in
`--watch` mode for a single file; a `.bat` wrapper just picks the file
and re-launches it if it ever exits (for example after a transient
read error), rather than reimplementing polling itself:

```bat
@echo off
setlocal
set SHADER=%1
if "%SHADER%"=="" set SHADER=shader.glsl

:loop
golf.exe --watch "%SHADER%" > "%SHADER%.min.glsl"
timeout /t 1 /nobreak >nul
goto loop
```

Run it as `watch.bat shaders\fractal.glsl`. For a whole directory
instead of one file, loop `golf.exe` itself on a timer since `--watch`
only takes a single `FILE`:

```bat
@echo off
setlocal
set SHADERS_DIR=%1
if "%SHADERS_DIR%"=="" set SHADERS_DIR=shaders

:loop
golf.exe "%SHADERS_DIR%"
timeout /t 2 /nobreak >nul
goto loop
```

## Installing

Download the latest `uShader-Setup-*.exe` from the
[Releases](https://github.com/Patrickjaillet/MicroShader/releases)
page and run it. The installer is not code-signed, so Windows
SmartScreen may show an "unknown publisher" warning on first run —
click "More info" -> "Run anyway" to proceed.

Requires Windows 10 or 11 (64-bit), using only in-box Win32/GDI+/
Direct2D/DirectWrite APIs — no separate runtime install. Targets
Windows 10 (LTSC 2019, build 17763) compatibility; a hardware/VM
acceptance pass against an actual LTSC 2019 install is still pending
(see `ROADMAP.md` Phase 27).

## Building from source

Requirements:

- Windows 10 or 11
- Visual Studio 2022 Build Tools (MSVC, C++20)
- CMake ≥ 3.21
- Rust toolchain with the `x86_64-pc-windows-msvc` target

```bash
cmake -S . -B build
cmake --build build
```
```bash (Release) 
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

To build the installer, install [Inno Setup 6](https://jrsoftware.org/isinfo.php)
and run:

```bash
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /DMyAppVersion=1.0.0.0 installer\ushader.iss
```

## License

[MIT](LICENSE) — free to reuse, modify, and redistribute. Bundles no
third-party binaries; see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## About

**µShader**
Copyright © 2026 SANDEFJORD DEVELOPMENT (Patrick JAILLET) — All rights reserved
Email: contact.shaderstudio@gmail.com
Website: https://github.com/Patrickjaillet
Repository: https://github.com/Patrickjaillet/MicroShader
