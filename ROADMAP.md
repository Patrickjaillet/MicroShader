# µShader — Roadmap

**µShader**
Copyright © 2026 SANDEFJORD DEVELOPMENT (Patrick JAILLET) — All rights reserved
Email: contact.shaderstudio@gmail.com
Website: https://github.com/Patrickjaillet
Official Repository: https://github.com/Patrickjaillet/MicroShader
License: MIT (see LICENSE)

---

## 1. Purpose

µShader is a native Windows 10/11 application that minifies
("golfs") Shadertoy-style GLSL fragment shaders and previews the
result live. The GLSL minification engine (Rust, `rust-core/`) is
exposed to the application through a C ABI bridge. The editor, UI
panels, and shader viewport are built natively with SDL3, OpenGL and
Dear ImGui.

---

## 2. Development conventions

These conventions apply to every phase below and must never regress.

- [ ] General language only English
- [x] Visual theme: Adobe Premiere Pro–style dark UI (see Phase 10).
      This supersedes the original "white theme only" decision below;
      the white theme is kept as historical record only.
- [x] ~~Theme White only~~ — superseded by Phase 10 (v1.2.x)
- [ ] Source language entirely in English (variable names, functions, classes)
- [ ] No comments in the source code
- [ ] Strict Windows 10/11 compatibility only
- [ ] Every added feature must be reflected in this ROADMAP.md
- [ ] Automatic software version serialization for each Phase and each build
- [ ] Every modification must be reflected for the end-user in the CHANGELOG.md
- [ ] The README.md must be created and updated for the end-user with every modification and include a software screenshot in docs/
- [ ] Systematic synchronization (commit+push) with the https://github.com/Patrickjaillet/MicroShader repository upon every project modification
- [ ] Never integrate Claude AI into GitHub, the files, or the GitHub contributors list
- [ ] Creation of all files and documents required for the GitHub repository
- [ ] Integrate copyright / Email / Website information / logo in docs/logo.png into an "About" tab
- [ ] Create icons for both the "Inno Setup" installer and the software
- [ ] Never incorporates the fact that the program is a conversion of another one.
- [ ] MIT license

### Notes on the conventions

- White theme only, English only: no language toggle, no `i18n`
  module — this is a deliberate design decision, not a limitation.
- "No comments in the source code" applies uniformly across the whole
  codebase, `rust-core/` included.

---

## 3. Versioning scheme

- Format: `MAJOR.MINOR.PATCH.BUILD`
- `MAJOR.MINOR` maps 1:1 to the Phase number below (Phase 0 → `0.1.x`,
  ... Phase 8 → `1.0.x`). Phase 9 is unscheduled/no fixed version;
  numbered phases resume the scheme from Phase 10 → `1.1.x`.
- `PATCH` increments for fixes/adjustments within a phase.
- `BUILD` auto-increments on every CI/local release build (generated,
  never hand-edited).
- Single source of truth: a `VERSION` file at repo root
  (`MAJOR.MINOR.PATCH`), read by CMake via `configure_file()` into a
  generated `version.h` (`APP_VERSION`, `APP_BUILD`, `APP_VERSION_STRING`).
  `APP_BUILD` is stamped from `git rev-list --count HEAD` at configure
  time. The About tab and Inno Setup installer both read this same
  generated header/string — one number, everywhere.
- Every version bump gets a matching entry in `CHANGELOG.md` and a Git
  tag (`vMAJOR.MINOR.PATCH`).

---

## 4. Target architecture

```
Rust golf engine (rust-core/)
        │  extern "C" (cbindgen-generated header)
        ▼
C ABI bridge (new, minimal, no comments)
        │
        ▼
C++ application (SDL3 + OpenGL)
   ├─ Renderer      : fullscreen-triangle Shadertoy-style runner, single pass only
   ├─ UI shell       : Dear ImGui (docking), Premiere-style dark theme, resizable panels
   ├─ Text editor    : ImGui-based widget with GLSL syntax highlighting
   └─ About / Setup  : copyright panel, Inno Setup installer, icons
```

---

## 5. Repository layout (target)

```
/ (repo root)
├── rust-core/                # GLSL minification engine + capi module
│   ├── src/
│   │   └── capi.rs           # new: extern "C" functions, cbindgen input
│   └── Cargo.toml            # + [features] capi
├── include/
│   └── ushader/
│       └── golf_core.h        # cbindgen-generated, committed
├── src/
│   ├── main.cpp
│   ├── render/                # SDL3 window, GL context, single-pass shader runner
│   ├── ui/                    # ImGui panels, layout, theme, about tab
│   ├── editor/                 # text editor widget + GLSL highlighting rules
│   └── platform/               # Windows-specific glue (file dialogs, clipboard)
├── assets/
│   ├── icons/                  # app.ico, installer.ico
│   └── fonts/
├── installer/
│   └── ushader.iss             # Inno Setup script
├── docs/
│   ├── screenshot.png
│   └── logo.png
├── CMakeLists.txt
├── VERSION
├── LICENSE                     # MIT
├── README.md
├── CHANGELOG.md
├── ROADMAP.md                  # this file
└── .gitignore
```

---

## 6. Phases

Each phase ends with: a version bump, a `CHANGELOG.md` entry, a
`README.md` update (with a fresh screenshot once there's a UI to
capture), and a commit+push to
`https://github.com/Patrickjaillet/MicroShader`.

### Phase 0 — v0.1.x — Repository & toolchain bootstrap

- [x] Create new repository structure (section 5)
- [x] Add `LICENSE` (MIT), `README.md` (placeholder), `CHANGELOG.md`,
      `ROADMAP.md` (this file), `.gitignore`
- [x] `VERSION` file + CMake `configure_file()` version stamping
- [x] CMake skeleton (C++20, MSVC toolchain, Windows 10/11 only)
- [x] Dependency fetching (vcpkg manifest or CMake `FetchContent`):
      SDL3, Dear ImGui (docking branch), stb_image, stb_image_write,
      stb_truetype (custom font + icon atlas loading), an ImGui
      text-editor widget, cbindgen (build-time, via Cargo)
- [x] Rust toolchain: `rustup target add x86_64-pc-windows-msvc`
- [x] Empty `main.cpp` that opens an SDL3 window and closes cleanly —
      first buildable milestone
- [x] Initial commit+push to `Patrickjaillet/MicroShader`

### Phase 1 — v0.2.x — Rust core → native C ABI bridge

- [x] Implement `rust-core/`: a tokenizer-based GLSL minifier
      (renaming, number-shortening, whitespace stripping, and
      aggressive passes such as dead-code elimination, constant
      folding, declaration merging)
- [x] Add `capi` feature + `src/capi.rs`: `extern "C"` wrappers around
      `golf_with_protected_names` returning heap C strings, plus a
      matching `ushader_free_string`
- [x] Generate `include/ushader/golf_core.h` with `cbindgen`, commit it
- [x] Build `rust-core` as a static lib for `x86_64-pc-windows-msvc`,
      link into a throwaway console test target
- [x] Verify golf output against a regression fixture corpus
      (`fixtures/*.glsl`) covering each transformation pass
- [x] CMake wiring: Cargo build invoked from CMake, static lib linked
      into the main target

### Phase 2 — v0.3.x — Window & rendering

- [x] SDL3 window + OpenGL 3.3 core context
- [x] Fullscreen-triangle Shadertoy-style fragment shader runner,
      single pass only — no buffers, no channel wiring, no "Common"
      code block (see section 7 for the multi-buffer rejection)
- [x] Standard uniform set: `iTime`, `iResolution`, `iMouse`, `iDate`,
      `iFrame`, `iFrameRate`
- [x] Compile/link error reporting surfaced to stdout (UI surfacing
      comes in Phase 3)
- [x] Hardcoded default shader (the single `mainImage` fragment body)
      renders and animates correctly

### Phase 3 — v0.4.x — Core UI shell

- [x] Dear ImGui integration (docking), **white theme only**
- [x] Custom visual theme — this is a first-class deliverable, not a
      cosmetic afterthought: custom sans-serif font (e.g. Inter or
      Segoe UI Variable) at a readable base size, rounded corners on
      windows/buttons/inputs, generous padding/spacing, a restrained
      accent-color palette, and a vector icon set (rasterized to a
      texture atlas) replacing the default ImGui look entirely.
      Reference target for "does this look dated" checks: ImHex
      (github.com/WerWolv/ImHex), a restyled-ImGui tool app with the
      same panel/tab/editor shape as µShader.
      Note: the icon font (Lucide, `assets/fonts/lucide.ttf`) is
      rendered as a real vector-glyph texture atlas via a dedicated
      `ImFont*`, pushed with `PushFont()`/`PopFont()` around each icon
      glyph — a font-atlas `MergeMode` bug in the current ImGui build
      returns the wrong glyph when the icon font is merged into the
      text font, so icons cannot yet sit inline inside a single label
      (e.g. window/tab titles stay plain text). Icons are currently
      used where a standalone glyph call is possible (the "Run golf"
      button, the shader-error indicator).
- [x] Three-panel layout: Source / Golfed / Viewport, resizable
      dividers, collapsing to tabs under a narrow window
- [x] Wire the "run golf" action to the Phase 1 C ABI bridge
- [x] Render errors surfaced in the UI (stage, log, source line)

### Phase 4 — v0.5.x — Text editor

- [x] Integrate the chosen ImGui text-editor widget
- [x] GLSL syntax highlighting rules (keywords, types, builtins,
      qualifiers, preprocessor directives)
- [x] Read-only "Golfed" view + "Formatted view" toggle
- [x] Error-line highlighting in the source editor

### Phase 5 — v0.6.x — Golfing controls & stats

- [x] Aggressive level selector + individual pass toggles for the
      golfing engine's transformation passes
- [x] Protected-names input field
- [x] Reduction stats panel: char counts, byte count, renamed count,
      numbers-shortened count, per-pass counters, size-budget badges
- [x] "Compare" mode: source vs. golfed single-pass viewport,
      side-by-side

### Phase 6 — v0.7.x — Import / export / capture

- [x] Open/save `.glsl` files (native Windows file dialogs)
- [x] Copy-to-clipboard for golfed output
- [x] Shadertoy-format export
- [x] PNG screenshot export (`glReadPixels` + `stb_image_write`)

### Phase 7 — v0.8.x — Branding & About

- [x] `docs/logo.png` created
- [x] "About" tab: copyright, email, website, repository link, logo
- [x] Application icon (`assets/icons/app.ico`)
- [x] Installer icon (`assets/icons/installer.ico`)

### Phase 8 — v1.0.x — Packaging & release

- [x] Inno Setup script (`installer/ushader.iss`) producing an
      installer using the Phase 7 icons and the stamped version.
      Unsigned: no code-signing certificate is available in the
      current build environment; `SignTool=` can be added to the
      `[Setup]` section once one is.
- [x] Manual compatibility pass on Windows 10 (LTSC 2019, build 17763)
      — built, installed, and exercised end to end. Windows 11 has
      not been independently verified; nothing in the codebase is
      known to be Windows-11-specific, but this has not been tested
      directly.
- [x] Final `README.md` pass with an up-to-date `docs/screenshot.png`
- [x] `CHANGELOG.md` 1.0.0 entry
- [x] Tag `v1.0.0`, push, publish release on
      `Patrickjaillet/MicroShader`

### Phase 9 — Post-1.0 (unscheduled)

Deliberately out of scope until requested explicitly:

- [x] Video (Webm, Mp4) /GIF recording of the viewport — GIF encoding
      is always available (bundled `gif-h`). MP4/WebM encode through a
      bundled `ffmpeg.exe` (fetched at build time from
      `BtbN/FFmpeg-Builds`, copied next to `ushader.exe`, and shipped
      by the installer), so no system `ffmpeg` install or `PATH` entry
      is required; the UI still disables MP4/WebM with a tooltip if
      the bundled binary is ever missing. See
      `THIRD_PARTY_NOTICES.md` for the FFmpeg (GPL) attribution this
      bundling requires.

### Phase 10 — v1.2.x — Adobe Premiere Pro–style UI/UX overhaul (dark theme)

Full replacement of the current light, stock-ImGui-derived look with a
dedicated dark visual identity modeled on Adobe Premiere Pro's editing
workspace. This is a first-class deliverable, at the same level as
Phase 3's original theme work — not a recolor pass. Every window,
control, and asset must be touched; nothing may still read as
"default ImGui" or as the Phase 3 white theme when this phase closes.

**Supersedes**: the Phase 3 white theme and the "white theme only /
no dark theme" entries in sections 2 and 7. `apply_theme()` becomes
the single dark theme; no light/dark toggle is introduced (still no
`i18n`-style user-facing theme switch — one theme, deliberately, as
before, just a different one).

#### 10.1 — Design tokens (`src/ui/theme.cpp`, new `theme_tokens.h`)

Introduce a named token table so every color used anywhere in the UI
traces back to one of these, no ad-hoc `ImVec4` literals scattered
across panels:

| Token              | Hex       | Usage                                                        |
|--------------------|-----------|---------------------------------------------------------------|
| `bg.app`           | `#1E1E1E` | Outer window / dockspace background                          |
| `bg.panel`         | `#232323` | Panel body background (Source, Golfed, Stats)                |
| `bg.panel.raised`  | `#2B2B2B` | Toolbars, menu bar, panel headers                             |
| `bg.field.sunken`  | `#1B1B1B` | Text editor body, input fields, viewport letterbox            |
| `bg.field.deepest` | `#000000` | Viewport render surface itself (Program-Monitor black)        |
| `bg.hover`         | `#3D3D3D` | Row/button/tab hover state                                    |
| `bg.active`        | `#4A4A4A` | Row/button/tab pressed state                                  |
| `border.hairline`  | `#000000` | 1px separators between panels (near-black, not gray)          |
| `border.subtle`    | `#454545` | Internal dividers inside a panel (e.g. stats rows)            |
| `accent`           | `#2680EB` | Primary accent — active tab underline, focus ring, run button |
| `accent.hover`     | `#4B9EFF` | Accent hover                                                  |
| `accent.active`    | `#1B5FBD` | Accent pressed                                                |
| `text.primary`     | `#E6E6E6` | Body text, labels                                             |
| `text.secondary`   | `#9E9E9E` | Muted labels, timecodes, panel sub-headers                    |
| `text.disabled`    | `#5C5C5C` | Disabled controls                                             |
| `status.ok`        | `#4CAF50` | Shader compiled successfully                                  |
| `status.warning`   | `#E9A23B` | Non-fatal golf warning                                        |
| `status.error`     | `#E5484D` | Compile/link error                                             |

All must pass WCAG AA contrast for `text.primary`/`text.secondary`
against `bg.panel` and `bg.app`; verify with a contrast-ratio check
before closing this sub-phase.

#### 10.2 — Global style metrics (replace current Phase 3 rounding)

- Corner radius: `2px` everywhere (`WindowRounding`, `FrameRounding`,
  `TabRounding`, `PopupRounding`, `GrabRounding` all reduced from the
  current `4–8px` to `2px`) — Premiere is almost square, not pill-shaped.
- `WindowBorderSize` / `PopupBorderSize`: keep at `1px` but recolor to
  `border.hairline` (near-black, not light gray).
- Panel header height fixed at `24px`; tab height fixed at `28px`, to
  match Premiere's compact chrome.
- Scrollbars: reduce `ScrollbarSize` from `14px` to `8px`, flat, no
  rounding, `bg.hover` on hover only (invisible at rest at low
  contrast, per Premiere).
- Splitters/resize grips: `4px` hit zone, invisible at rest, painted
  `accent` only while hovered/dragged (Premiere's panel-resize
  behavior, not the current always-visible gray grip).

#### 10.3 — Custom window chrome

- Replace the default OS title bar with an SDL3 borderless window +
  custom hit-test region: `bg.field.sunken` bar, app icon (from the
  Phase 10.6 recolor) at the far left, window title centered-left in
  `text.secondary`, flat minimize/maximize/close glyph buttons at the
  far right (`text.secondary` idle, `bg.hover` hit background,
  close button turns `status.error` on hover — standard Windows dark
  app convention, matches Premiere's own chrome).
- Menu bar sits directly under the title bar, `bg.panel.raised`, flat
  (no border), hover row highlight `bg.hover` with a `2px` accent bar
  on the left edge of the hovered item.

#### 10.4 — Panels, tabs, and controls (touch every panel listed in
section 5: Source, Golfed, Viewport, Stats, golf controls, About)

- Tabs: flat rectangle, `bg.panel.raised` idle, `bg.hover` hover,
  active tab gets a `2px` `accent` underline (no filled pill, no glow
  — this is the single most recognizable Premiere tab cue).
- Panel section headers (e.g. "REDUCTION STATS", "GOLF PASSES"):
  uppercase, letter-spaced, `text.secondary`, `11px`, sitting on
  `bg.panel.raised`, not `bg.panel` — visually pins the header the
  way Premiere pins its panel toolbars.
- Buttons: flat rectangle, `2px` radius, `bg.hover`/`bg.active` for
  secondary buttons; the primary "Run golf" button is solid `accent`
  fill with the existing play icon, `accent.hover`/`accent.active` on
  interaction — the one clearly "primary" surface in the whole UI.
- Checkboxes: flat `12px` square, `border.subtle` idle, `accent` fill
  with a white check glyph when active — not the current circular
  ImGui check.
- Sliders (aggressive-level, any future numeric control): thin `2px`
  groove on `bg.field.sunken`, small rectangular (not round) grab
  handle filled `accent`.
- Text inputs / protected-names field: `bg.field.sunken`, `1px`
  `border.hairline`, `1px` `accent` focus ring on focus only.
- Combo/dropdowns: flat, `bg.field.sunken` body, chevron icon in
  `text.secondary`, open list styled like the menu bar (hover row +
  left accent bar).

#### 10.5 — Text editor and viewport, Premiere-specific treatments

- Source/Golfed editor body: `bg.field.sunken`, gutter (line numbers)
  in `text.disabled` on `bg.panel`, current-line highlight a barely-
  lighter `bg.hover` band, error-line highlight `status.error` at low
  opacity (background wash, not full-saturation fill).
- Viewport panel styled as a Program Monitor: render surface framed
  by `bg.field.deepest` letterboxing regardless of window aspect,
  transport row immediately below in `bg.panel.raised` with the
  existing play/pause/stop icons recolored to `text.secondary` (idle)
  / `accent` (active/pressed), and a monospace timecode readout
  (reuse Inter but tabular-figure sized) in `text.secondary`.
- Compile status indicator: small dot, filled `status.ok` /
  `status.warning` / `status.error`, replacing the current plain-text
  error banner as the primary at-a-glance signal (detailed log stays
  as text below it).

#### 10.6 — Icon and asset regeneration

- Re-theme the existing Lucide icon glyphs: default state
  `text.secondary`, hover `text.primary`, active `accent` — glyphs
  are already vector (`lucide.ttf`), this is a color-state change in
  `theme.cpp`/call sites, not a re-draw.
- Regenerate `assets/icons/app_source.png` → new `app.ico` /
  `installer.ico`: same mark, recomposed on a `bg.app`-toned dark
  tile with an `accent`-blue treatment of the glyph, so the taskbar
  icon reads as "dark app" rather than the current light icon.
- Regenerate `docs/logo.png` for the About panel on a dark card
  background consistent with 10.7.
- New reference assets under `docs/design/` (not shipped in the
  installer, design-process artifacts only):
  - `color-palette.svg` — every token in 10.1 as a labeled swatch.
  - `ui-mockup-full.svg` — full-window mockup: title bar, menu bar,
    Source/Golfed/Viewport three-panel layout, stats panel, in the
    new palette, at the app's default window size.
  - `icon-states.svg` — each reused icon glyph in idle/hover/active.

#### 10.7 — About panel redesign

- Rebuilt as a centered dark card (`bg.panel.raised` on `bg.app`),
  logo top, app name + version, then copyright/email/website/
  repository lines in `text.secondary` separated by `1px`
  `border.subtle` hairlines — replacing the current plain list
  layout with the same card treatment Premiere uses for its own
  About dialog.

#### 10.8 — Acceptance / verification

- [x] Side-by-side screenshot check against a real Premiere Pro
      panel (menu bar height, tab shape, panel header height, accent
      usage) — no element should still visually match the Phase 3
      white theme or a stock ImGui demo. Verified by visual review of
      the built app against the described Premiere conventions (flat
      rectangular tabs with accent underline, compact chrome, square
      controls); not a pixel-level diff against an actual Premiere
      Pro screenshot.
- [x] Every color in `theme.cpp` traces to a token in 10.1 — no
      inline literal `ImVec4` left in panel code (the two exceptions
      are fully transparent `ImVec4(0,0,0,0)` push colors for the
      borderless title-bar buttons, which is not a themed color).
- [x] Contrast check for `text.primary` / `text.secondary` against
      `bg.panel` and `bg.app` passes WCAG AA (verified via a WCAG
      relative-luminance script: all four pairs ≥ 5.8:1, well above
      the 4.5:1 AA threshold).
- [x] `docs/screenshot.png` retaken in the new theme.
- [x] `README.md` updated with the new screenshot and a short note on
      the visual redesign.
- [x] `CHANGELOG.md` entry for `v1.2.1`, tag `v1.2.1`.

---

## 7. Explicitly out of scope for v1.0

- **Multi-buffer Shadertoy rendering** — no buffers A–D, no
  `iChannel` wiring, no shared "Common" pass. µShader golfs and
  previews a single `mainImage` fragment shader only. This is a
  deliberate design rejection, not a "later" item.
- ~~Dark theme / any theme other than white~~ — superseded by
  Phase 10 (v1.1.x); still only one theme at a time, no user-facing
  theme toggle, just a different single theme
- Any language other than English in the UI or source
- Any AI-tool attribution anywhere in the repository, commit history,
  or contributors list
