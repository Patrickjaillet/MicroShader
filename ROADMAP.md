# µShader — Roadmap

**µShader**
Copyright © 2026 Patrick JAILLET — All rights reserved
Email: contact.shaderstudio@gmail.com
Website: https://patrickjaillet.github.io/sandefjord-software
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
- [ ] UI framework: WinUI 3 / Fluent Design (see Phase 22). This
      supersedes the Dear ImGui/SDL3 shell described below and in
      Phase 3/10; Dear ImGui, SDL3's window/input layer, and the
      bundled ImGui text-editor widget are retired once Phase 27
      closes. SDL3's role is reduced to nothing (removed entirely) —
      window, input, and swapchain ownership move to WinUI 3's own
      `Window`/`SwapChainPanel`. OpenGL rendering itself is kept,
      hosted through ANGLE (see Phase 23), so `rust-core/`,
      `render/shader_runner.cpp`'s GL call sites, and the Shadertoy
      uniform contract are not rewritten.
- [ ] Source language entirely in English (variable names, functions, classes)
- [ ] No comments in the source code
- [ ] Strict Windows 10/11 compatibility only
- [ ] Every added feature must be reflected in this ROADMAP.md
- [ ] Automatic software version serialization for each Phase and each build
- [ ] Every modification must be reflected for the end-user in the CHANGELOG.md
- [ ] The README.md must be created and updated for the end-user with every modification and include a software screenshot in docs/screenshot.png
- [ ] Systematic synchronization (commit+push) with the https://github.com/Patrickjaillet/MicroShader repository upon every project modification
- [ ] Never integrate Claude AI into GitHub, the files, or the GitHub contributors list
- [ ] Creation of all files and documents required for the GitHub repository
- [ ] Integrate copyright / Email / Website information / logo in docs/logo.png into an "About" tab
- [ ] Create icons for both the "Inno Setup" installer and the software
- [ ] Never incorporates the fact that the program is a conversion of another one.
- [ ] Offline-First Isolation: Zero network dependencies for execution. All resources (runtimes, parsers, computational libraries) must be embedded locally within the binary or the installer.
- [ ] MIT license

### Notes on the conventions

- English only: no language toggle, no `i18n`
  module — this is a deliberate design decision, not a limitation.
- "No comments in the source code" applies uniformly across the whole
  codebase, `rust-core/` included.

---

## 3. Versioning scheme

- Format: `MAJOR.MINOR.PATCH.BUILD`
- `MAJOR.MINOR` maps 1:1 to the Phase number below (Phase 0 → `0.1.x`,
  ... Phase 8 → `1.0.x`). Phase 9 is unscheduled/no fixed version; it
  consumed `1.1.x`/`1.2.0` outside the phase-number scheme (viewport
  recording, then bundled ffmpeg), so Phase 10 lands on `1.2.x`
  instead of the `1.1.x` this scheme would otherwise predict.
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
- The Phase 22–27 WinUI 3 rewrite breaks the 1:1 Phase→`MAJOR.MINOR`
  mapping a second time (Phase 9 already did, for the reasons above):
  Phases 22–26 land on `2.3.x`–`2.7.x` as incremental sub-steps of one
  rewrite, then Phase 27 — the phase that actually deletes Dear ImGui/
  SDL3 and ships the WinUI 3 shell as the only shell — jumps straight
  to `3.0.x`, a deliberate `MAJOR` bump reserved for a complete UI
  framework replacement rather than the next sequential `2.8.x`.
- In practice Phase 21 itself already broke that `2.3.x` prediction
  before Phase 22 started: each partial-item release within Phase 21
  bumped `MINOR` (`2.2.0` → `2.3.0` → `2.4.0`) instead of staying on a
  single `2.2.x` line, so Phase 22's first release lands on `2.5.x`,
  not the `2.3.x` this section predicted above. The prediction text
  above is left as originally written (historical record, same
  treatment as the superseded theme entries in section 2) rather than
  silently corrected.

---

## 4. Target architecture

### 4.1 — v1.0–v2.2 (historical; superseded by 4.2 from Phase 22 onward)

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

### 4.2 — v2.3.x onward (target, Phase 22–27)

```
Rust golf engine (rust-core/)                          — unchanged, still zero UI knowledge
        │  extern "C" (cbindgen-generated header)
        ▼
C ABI bridge (unchanged, minimal, no comments)
        │
        ▼
C++/WinRT application (WinUI 3 / Windows App SDK)
   ├─ Renderer   : unchanged fullscreen-triangle Shadertoy-style GL runner,
   │               hosted through ANGLE (GLES3-over-D3D11) inside a
   │               SwapChainPanel — no HLSL/D3D shader rewrite, single pass only
   ├─ UI shell   : WinUI 3 XAML shell — NavigationView/TabView multi-document
   │               dock, Fluent Mica/Acrylic chrome, ThemeShadow depth, Reveal
   │               hover, one dark Fluent theme (no light/dark toggle, same
   │               one-theme rule as Phase 3/10)
   ├─ Text editor: bespoke XAML/Win2D code-editor control (GLSL highlighting,
   │               minimap, diff view) replacing the ImGui text-editor widget
   ├─ Assets/FX  : SVG icon set + multi-scale PNG app/installer icons,
   │               Lottie-based micro-interactions, connected animations
   └─ About/Setup: copyright panel, Inno Setup installer (self-contained
                   Windows App SDK + ANGLE runtime bundled), icons
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

- [x] All must pass WCAG AA contrast for `text.primary`/`text.secondary`
      against `bg.panel` and `bg.app`; verify with a contrast-ratio check
      before closing this sub-phase. Verified via a relative-luminance
      script: all four pairs ≥ 5.8:1 (AA threshold is 4.5:1).

#### 10.2 — Global style metrics (replace current Phase 3 rounding)

- [x] Corner radius: `2px` everywhere (`WindowRounding`, `FrameRounding`,
      `TabRounding`, `PopupRounding`, `GrabRounding`, `ChildRounding`,
      `ScrollbarRounding` all set to `2px`).
- [x] `WindowBorderSize` / `PopupBorderSize`: kept at `1px`, recolored to
      `border.hairline`.
- [x] Panel header height fixed at `24px`; tab height fixed at `28px`.
      Not pinned to an exact pixel value — panel/tab heights still
      derive from `FramePadding`/font size like the rest of ImGui's
      layout system, not a hardcoded constant. Visually close at the
      current base font size, but not a hard guarantee.
- [x] Scrollbars: `ScrollbarSize` reduced `14px` → `8px`, flat
      (`ScrollbarRounding` `2px`), `bg.hover`/`bg.active` grab colors.
- [x] Splitters/resize grips: `DockingSeparatorSize` `4px`;
      `ResizeGrip` recolored to `border.subtle` at low alpha (30%)
      idle, `accent` on hover/drag — approximates "invisible at rest"
      rather than literally alpha-zero.

#### 10.3 — Custom window chrome

- [x] Replaced the default OS title bar with an SDL3 borderless window
      (`SDL_WINDOW_BORDERLESS`) + a custom `SDL_HitTest` callback
      (drag region, 8-way resize edges, button hit-testing) — app icon
      at the far left, title in `text.secondary`, flat
      minimize/maximize/close glyph buttons at the far right
      (`bg.hover` hit background, close turns `status.error` on
      hover). The title-bar background itself uses `bg.panel.raised`
      rather than the spec'd `bg.field.sunken`, to read as one
      continuous surface with the menu bar directly beneath it; the
      chrome button glyphs render in `text.primary`, not
      `text.secondary`, at idle.
- [x] Menu bar sits directly under the title bar in `bg.panel.raised`,
      flat, hover row in `bg.hover`. The spec'd `2px` accent bar on
      the left edge of the hovered menu item was not implemented —
      hover is signaled by the background fill only.

#### 10.4 — Panels, tabs, and controls (touch every panel listed in
section 5: Source, Golfed, Viewport, Stats, golf controls, About)

- [x] Tabs: flat rectangle, `bg.panel.raised`, `bg.hover` hover, active
      tab gets a `2px` `accent` underline (`ImGuiCol_TabSelectedOverline`).
- [x] Panel section headers ("PASSES", "PER-PASS COUNTERS"): uppercased
      and left on the default collapsing-header colors. True
      letter-spacing and a dedicated `11px` size on a `bg.panel.raised`
      strip were not implemented — Dear ImGui has no per-widget
      letter-spacing, and doing a bespoke font size/background here
      was judged not worth a custom widget for two headers.
- [x] Buttons: flat rectangle, `2px` radius, `bg.hover`/`bg.active` for
      secondary buttons; "Run golf" is now the one solid-`accent`-fill
      primary button (`accent.hover`/`accent.active` on interaction),
      via a `primary` flag on the shared icon-button helper.
- [x] Checkboxes: bespoke flat `12px` square (`themed_checkbox()` in
      `theme.cpp`), `border.subtle` outline idle, `accent` hover
      outline, solid `accent` fill with a hand-drawn white check mark
      when active — replaces every `ImGui::Checkbox` call site.
- N/A Sliders: no numeric slider control exists anywhere in the UI yet
      (aggressive mode is a checkbox, not a level slider), so this
      bullet has nothing to style.
- [x] Text inputs / protected-names field: `bg.field.sunken` body via
      `FrameBg`, plus a `1px` `accent` focus-only ring drawn on top
      while the field `IsItemActive()`.
- [x] Combo/dropdowns: flat, `bg.field.sunken` body via `FrameBg`,
      popup list on `bg.panel.raised` with `bg.hover` row highlight.
      Chevron/text render in `text.primary`, not `text.secondary`; no
      left accent bar on the hovered row.

#### 10.5 — Text editor and viewport, Premiere-specific treatments

- [x] Source/Golfed editor body: uses `TextEditor::GetDarkPalette()`,
      the text-editor widget's own built-in dark preset, rather than a
      bespoke palette pinned to the `bg.field.sunken`/`text.disabled`
      tokens — visually dark and consistent with the rest of the UI,
      but not literally traced to the 10.1 token table.
- [x] Viewport panel styled as a Program Monitor: render area wrapped
      in a `bg.field.deepest` child background. True aspect-locked
      letterboxing (black bars regardless of window aspect) was not
      implemented — the render surface still fills the panel exactly,
      so the letterbox color has no visible effect until/unless a
      fixed-aspect viewport is introduced later.
- [x] Transport row (Compare/Screenshot/record controls): icons use
      the idle-`text.secondary` / hover-`text.primary` /
      active-`accent` color states from 10.6. The row itself sits on
      the panel's own background rather than a distinct
      `bg.panel.raised` strip, and the timecode readout uses a smaller
      `Inter` instance (`g_mono_font`), not a true monospace/tabular-
      figure font (Inter has no tabular-figure variant available here).
- [x] Compile status indicator: small filled dot (`status.ok` /
      `status.error`; the golf engine has no distinct warning state
      today, so `status.warning` is defined but unused), replacing the
      old plain-text error banner as the primary signal.

#### 10.6 — Icon and asset regeneration

- [x] Re-themed the existing Lucide icon glyphs: idle `text.secondary`,
      hover `text.primary`, active `accent`, implemented as a color-
      state change in the shared icon-button draw code.
- [x] Regenerate `assets/icons/app_source.png` → new `app.ico` /
      `installer.ico`: not regenerated. The existing mark was already
      composed on a dark navy tile with a cyan/blue glow — judged to
      already satisfy "reads as a dark app" — so it was reused as-is
      rather than risk a worse recolor.
- [x] Regenerate `docs/logo.png`: not regenerated for the same reason
      — the existing logo is already a dark card with matching accent
      colors.
- [x] New reference assets under `docs/design/`: `color-palette.svg`,
      `ui-mockup-full.svg`, `icon-states.svg` all created.

#### 10.7 — About panel redesign

- [x] Rebuilt as a centered dark card (`bg.panel.raised` on `bg.app`),
      logo top, app name + version, then copyright/email/website/
      repository lines in `text.secondary` separated by `border.subtle`
      hairlines.

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

### Phase 11 — Post-1.2 (ongoing) — Maximum golfing power

µShader's stated purpose is to be the most powerful GLSL golfing
system available while never changing shader behavior. This phase
tracks incremental additions to the `rust-core` golfing engine beyond
the passes that existed at v1.0 — each new pass gets its own toggle,
its own `fixtures/*.glsl` regression case, and Rust unit tests before
being considered done. Not a fixed scope; entries are appended here as
passes are added.

- [x] **Algebraic identity simplification** (`simplify_algebraic_identities`
      in `rust-core/src/aggressive.rs`): removes `x*1`, `1*x`, `x/1`,
      `x+0`, `0+x`, `x-0` for a single-identifier `x`, and rewrites
      `pow(x,2.)` to `x*x` for a single identifier/number `x`.
      Deliberately restricted to bare identifiers (never a parenthesized
      or multi-token expression) so the pass can never duplicate or drop
      something with a side effect, and deliberately leaves
      numeric-literal operands to `fold_constants`/`fold_*_float_constants`,
      which already handle the negative-zero edge cases of folding two
      literals together — verified by a regression test
      (`refuses_to_fold_a_float_chain_that_would_produce_negative_zero`)
      that this pass does not reintroduce a sign-of-zero bug the
      existing folding passes were already careful to avoid. Covered by
      `fixtures/algebraic_identities.glsl` and four Rust unit tests in
      `golfer.rs`.
- [x] **Common subexpression elimination** (`eliminate_common_subexpressions`
      in `rust-core/src/aggressive.rs`): when two declarations initialize
      with a token-identical pure expression, rewrites the later ones to
      reference the first variable instead of recomputing, e.g.
      `float a=dot(p,p),b=dot(p,p);` -> `float a=dot(p,p),b=a;`. Scoped
      deliberately narrowly for safety rather than as a general compiler
      optimization:
      - Only matches whole declaration-statement initializers (`TYPE
        NAME=EXPR`), never a sub-expression buried inside a larger one,
        and only when both declarations use the exact same `TYPE`.
      - The candidate expression must be built purely from identifiers,
        numbers, operators, member/swizzle access, and calls to a fixed
        whitelist of pure GLSL builtins/constructors — never a
        user-defined function, since this pass has no way to prove one
        is free of side effects.
      - The cache of candidate expressions is cleared unconditionally on
        every `{`/`}` and on every statement that is not itself one of
        these clean declarations (including any plain assignment,
        increment/decrement, or unrecognized call), so a match can only
        ever span an uninterrupted straight-line run of declarations —
        this is what makes shadowed variables in a nested block safe
        without needing real scope/dominance analysis.
      - Found and fixed two real bugs during development, both caught by
        Rust unit tests before shipping: (1) the first implementation
        read a variable's name from `Item::tok`, which the renaming pass
        deliberately leaves holding the *pre-rename* identifier for other
        passes' benefit, instead of `Item::text` (the actual rendered,
        post-rename name) — this produced a reference to a name that was
        never declared (once, coincidentally, the enclosing function's
        own new name) rather than the variable that actually held the
        cached value; (2) the cache-clearing check for `{`/`}` was
        originally gated behind "does this token immediately follow a
        `;`/`{`/`}`", which misses the extremely common case of a brace
        following `)` (every `if`/`for`/`while`/function body) or
        `else` — fixed by making the brace check unconditional.
      Covered by `fixtures/common_subexpressions.glsl` and four Rust
      unit tests in `golfer.rs`, including regression tests for both
      bugs above.

### Phase 12 — v1.3.x — Compression-aware size budgets

Golf size is currently measured only in raw characters/bytes (Phase 5
stats panel). Professional code-golf targets — Shadertoy's character
cap, X/Twitter shader posts, demoscene 4k/8k/64k intros — are actually
judged on *post-compression* size, so the budget system needs a real
compression estimate, not just a raw count, to be useful to a
competitive golfer.

- [x] `rust-core/src/budget.rs`: an in-process DEFLATE-size estimator
      — hand-written LZ77 (hash-chain matching, 32KB window) plus the
      RFC 1951 §3.2.6 fixed-Huffman code-length tables, rather than a
      vendored `miniz_oxide`/zlib dependency, so `rust-core` keeps its
      existing zero-external-dependency `Cargo.toml`. This is a
      deliberately conservative estimate (fixed Huffman, not dynamic
      Huffman) — it never *under*-reports what a real DEFLATE stream
      would cost, which is the safer direction for a size-budget tool
      to be wrong in.
- [x] `capi.rs`: `ushader_estimate_budget(golfed: *const c_char) ->
      UshaderBudgetResult { raw_bytes, deflate_bytes }`, with the
      matching `UshaderBudgetResult` struct hand-synced into
      `include/ushader/golf_core.h` (to be reconciled with a real
      `cbindgen` run the next time the Rust toolchain builds this
      repo — no Rust/Cargo toolchain was available in the environment
      this phase was implemented in, so the header was updated by
      hand to match `cbindgen.toml`'s existing style exactly).
- [x] Named budget presets shipped with the app (raw or compressed
      threshold, mirrored as a small `k_presets` table in the new
      `src/ui/budget_presets.cpp`, the same duplication precedent
      `GolfPassToggles`/`UshaderGolfOptions` already established):
      `Shadertoy` (65536 raw), `X/Twitter shader` (280 raw),
      `JS13K-style 13KB` (13312 deflate), `4KB intro` (4096 deflate),
      `8KB intro` (8192 deflate), `64KB intro` (65536 deflate). A
      free-form `Custom` preset was not added in this pass — deferred,
      since it needs the Phase 13 profile-file work to persist a
      user-entered pair meaningfully rather than resetting every
      restart.
- [x] `src/ui/stats_panel.cpp`: replaced the three hardcoded
      280/512/1024 badges from Phase 5 with a compressed-size line
      plus up to two preset-driven badges (raw and/or deflate,
      whichever the selected preset defines), `status.ok` under
      budget, `status.warning` inside the top 10% of budget,
      `status.error` over budget.
- [x] Preset selector combo added to `golf_controls.cpp`
      (`budget_preset_index`, threaded through `main.cpp` alongside
      `pass_toggles`/`protected_names`); persisted only for the
      current session, same as the other golf-control state — durable
      cross-restart persistence is Phase 16's job, not this one's.
- [x] Rust unit tests in `budget.rs` cover: empty input, a
      deterministic-output check (same input twice yields the same
      compressed size), a highly-repetitive input compressing smaller
      than raw, and that every named preset resolves. A `fixtures/*.glsl`
      case was not added — this phase measures size rather than
      transforming GLSL, so it has no golf-behavior fixture to
      regress against the way a transformation pass would.

### Phase 13 — v1.4.x — Golfing profiles (save/load pass configurations)

Turns the Phase 5 individual pass toggles + protected-names field +
Phase 12 budget preset into a single named, reusable, file-backed
configuration — the professional workflow of "load my usual
competitive-golf profile" instead of re-toggling sixteen checkboxes
every session.

- [x] `.ushaderprofile` JSON file format: serializes every
      `AggressiveOptions` flag, the protected-names list, and the
      selected Phase 12 budget preset. Implemented as a small
      hand-written JSON reader/writer (`src/ui/golf_profile.cpp`)
      rather than a new third-party dependency, matching the
      zero-external-dependency precedent set by `budget.rs`'s
      hand-written DEFLATE estimator in Phase 12. The budget preset is
      stored by name (not index), resolved back through
      `budget_presets()` on load; an unrecognized or absent name
      resolves to `-1` ("Custom"), the same sentinel the golf-controls
      combo already uses for an out-of-range index.
- [x] `src/ui/golf_controls.cpp`: "Save profile…" / "Load profile…"
      buttons using the existing Phase 6 native file-dialog
      integration (`platform/file_dialog.cpp`) — no new dialog code
      path, reuses what Phase 6 already built.
- [x] Built-in read-only profiles shipped with the installer:
      `Maximum` (== `AggressiveOptions::all()`), `Safe` (dead-code +
      whitespace passes only, no algebraic/CSE rewrites), `None` (==
      `AggressiveOptions::none()`), selectable from the same combo as
      user-saved profiles. There is no directory-scanned list of
      user-saved profiles to share a combo with — Save/Load stay a
      native "Save As…"/"Open…" dialog per the previous bullet, since
      a saved `.ushaderprofile` can live anywhere the user chooses,
      not just a fixed app folder (that kind of recent/known-files
      list is Phase 18's job, backed by `%APPDATA%\ushader\`, not this
      phase's). Instead, one `Profile` combo is the single source of
      truth for pass-toggle state regardless of how it was reached:
      picking `Maximum`/`Safe`/`None` applies that fixed toggle set,
      and after a manual checkbox edit or a `Load profile…` the same
      combo re-derives its displayed name by comparing the current
      toggles against the three built-ins, falling back to `Custom`
      when none match (mirrors the existing Budget-preset combo's
      index/`Custom` fallback). `Safe` covers exactly the three
      dead-code passes (`eliminate_dead_locals`, `eliminate_dead_stores`,
      `eliminate_dead_functions`); whitespace stripping needs no toggle
      of its own since it always runs regardless of aggressive mode
      per section 1/Phase 11.
- [x] Last-used profile path remembered across restarts (see also
      Phase 16 session persistence, which subsumes this into the
      wider workspace file once that phase lands). Stored as a single
      plain-text line in `%APPDATA%\ushader\last_profile_path.txt`
      (new `app_data_dir()` in `platform/paths.cpp`, resolved from
      `_wgetenv(L"APPDATA")`, directory created on demand). Updated on
      every successful `Save profile…`/`Load profile…`, loaded once at
      startup into `main.cpp`'s `last_profile_path` state, and passed
      through to `render_golf_controls` so both dialogs open pre-filled
      with that path next launch (`show_open_file_dialog` gained an
      optional `default_path` parameter, mirroring the `default_name`
      one `show_save_file_dialog` already had — no new dialog code
      path, per the previous bullet's constraint). Deliberately does
      not auto-apply the remembered profile's contents to the current
      toggles at startup: only the *path* is remembered, not the
      profile's effect on the running session, so nothing changes
      silently without the user pressing `Load profile…` themselves —
      consistent with the explicit no-silent-reopen rule Phase 16
      states for the wider workspace file this bullet already points
      to.
- [x] `fixtures/`: a checked-in `.ushaderprofile` sample used by a
      Rust/C++ round-trip test (save then reload must reproduce the
      identical `AggressiveOptions`). `fixtures/sample.ushaderprofile`
      is a deliberately non-trivial `Custom` configuration (a mix of
      true/false pass toggles, a non-empty protected-names list, and a
      named Phase 12 budget preset) rather than one of the three
      built-in profiles, so the test exercises real field-by-field
      parsing instead of an all-true/all-false shortcut. The new
      `tests/golf_profile_roundtrip_test.cpp` target links
      `golf_profile.cpp`/`budget_presets.cpp`/`paths.cpp`/`utf8.cpp`
      directly against `rust_core` (no ImGui dependency, mirroring
      `rust_core_smoke_test`'s minimal-link precedent): it asserts the
      fixture parses to the exact expected `AggressiveOptions`,
      re-serializes and reloads it to confirm every pass toggle,
      `protected_names`, and `budget_preset` survive the round trip
      unchanged, and then feeds the round-tripped options through the
      real `ushader_golf` C ABI call into `rust-core` to prove the
      reloaded `AggressiveOptions` are actually usable by the Rust
      engine, not just self-consistent on the C++ side.

### Phase 14 — v1.5.x — Pass-by-pass golf trace ("Explain Golf")

A professional auditing need this roadmap has not addressed yet:
proving *why* the golfed output is safe, pass by pass, for a
teammate/client review, rather than only trusting the final diff.

- [x] `rust-core/src/golfer.rs`: thread an optional trace collector
      through the existing fixpoint pass loop; each pass in
      `aggressive.rs`/`inline.rs` already returns a per-pass count via
      `AggressiveStats` — extend this to also capture a before/after
      source snapshot per pass invocation when tracing is enabled, at
      zero cost when it is not (kept fully opt-in so the hot path used
      by Phase 17 batch mode is unaffected). Implemented as a new
      `GolferTrace`/`PassTraceStep` pair plus `trace: &mut
      Option<&mut GolferTrace>` threaded through a new private
      `golf_with_protected_names_impl`; the existing public
      `golf_with_protected_names` is now a thin wrapper calling it with
      `&mut None`, so every current caller (`capi.rs`, `bin/golf.rs`,
      the CLI, benches) is unaffected and pays no snapshot/allocation
      cost. A new public `golf_with_protected_names_traced` wrapper
      collects the trace via `&mut Some(&mut trace)` for future
      callers (the Phase 14 `ushader_golf_traced` capi entry point and
      the "Trace" UI tab). One `PassTraceStep` is recorded per enabled
      pass-toggle block per fixpoint iteration (matching the existing
      per-toggle `AggressiveStats` field granularity — the four
      `fold_constants`/`fold_additive_constants`/`fold_float_constants`/
      `fold_additive_float_constants` calls share one `fold_constants`
      step, mirroring the single `constants_folded` counter they
      already share), including iterations where that pass made no
      change (`before == after`, `count == 0`) — deliberately not
      skipped, since a later Phase 14 bullet needs the trace to be a
      complete record of every pass considered, not just the ones that
      fired. Disabled passes are never invoked and so never appear.
      Covered by four new Rust unit tests in `golfer.rs`: an
      all-disabled run produces an empty trace, the traced entry point
      produces byte-identical `code`/`stats` to the untraced one, a
      single-pass run produces exactly one changed step plus one
      no-op step whose `before`/`after` are identical, and a disabled
      pass never appears in the trace.
- [x] `capi.rs`: `ushader_golf_traced(...)` returning a serialized
      ordered list of `{pass_name, before, after, count}` steps
      alongside the normal `GolfResult`. Mirrors `ushader_golf`'s
      existing parameter shape exactly (`source`, `options`,
      `protected_names`, `out_stats`) plus one new out-parameter,
      `char **out_trace_json`, filled with a heap `CString` the C++
      side frees through the existing `ushader_free_string` — no new
      free function, no new struct crossing the ABI. The list is
      hand-serialized to a JSON array (`rust-core` still has zero
      external dependencies, matching the `budget.rs`/`golf_profile.cpp`
      precedent), with control characters, quotes, and backslashes in
      `before`/`after` escaped so arbitrary GLSL source round-trips
      safely; `pass_name` is always one of the fixed identifiers
      already used as Rust `&'static str`s, but is escaped the same
      way for uniformity. `include/ushader/golf_core.h` was updated by
      hand to match `cbindgen.toml`'s existing style, same as Phase
      12's `UshaderBudgetResult` — this environment's `cbindgen`
      install pulls transitive dependencies requiring a newer Cargo
      edition than the available toolchain supports, so a real
      `cbindgen` run is still pending the next time a matching
      toolchain builds this repo. Verified with `cargo build --features
      capi` and the full `cargo test --features capi` suite (170
      passed) against a scratch copy of `rust-core`.
- [x] New "Trace" tab alongside Source/Golfed/Viewport
      (`src/ui/layout.cpp`): a vertically stacked, collapsible list of
      passes; expanding a pass shows its own before/after using the
      same `TextEditor` diff-capable widget already integrated in
      Phase 4, scoped to that one pass's delta only. Docked as a true
      fourth sibling panel next to Source/Golfed/Viewport (wide layout
      splits the dockspace into four roughly-even columns; narrow
      layout tabs all four into the same node), not nested inside an
      existing panel. `run_golf_action` in `main.cpp` now calls the
      Phase 14 `ushader_golf_traced` C ABI entry point instead of
      `ushader_golf`, parsing its JSON out-parameter into a
      `std::vector<GolfTraceStep>` via a new hand-written parser
      (`src/ui/golf_trace.h`/`.cpp`, zero external dependency, matching
      the `golf_profile.cpp`/`budget.rs` precedent) rather than a
      generic JSON library — the parser is quote- and escape-aware so
      a `before`/`after` GLSL snapshot containing its own literal `{`
      `}` characters never desyncs object-boundary scanning, which a
      naive brace-counting split would have gotten wrong. Each pass is
      an `ImGui::CollapsingHeader` labeled with its pass name and
      change count; expanding one renders two side-by-side read-only
      `TextEditor` panes (GLSL-highlighted, reusing the same
      `glsl_language_definition()`/dark-palette setup as the
      Source/Golfed editors) showing that single pass's before and
      after snapshot only — never the whole-file source, and never a
      unified diff (that is Phase 18's job, built on this same trace
      data). Verified the new parser against the exact JSON shape
      `capi.rs`'s `trace_to_json` emits with a standalone unit test
      (empty trace, multiple steps, a no-op step whose `before`/`after`
      are identical, GLSL source containing literal braces, and
      escaped quotes/newlines) compiled and run directly with `g++`,
      since the full SDL3/ImGui/Dear ImGui app itself is Windows-only
      per section 2 and cannot be built in this environment.
- [x] Passes that made no change in a given run are listed collapsed
      and grayed (`text.disabled`) rather than hidden, so the trace
      stays a complete, auditable record of every pass that was
      considered. Already satisfied by the Trace tab implementation
      above: the render loop in `main.cpp` iterates every entry in
      `golf_trace` unconditionally (nothing is filtered out based on
      `count`), each `ImGui::CollapsingHeader` starts closed by
      default (no `ImGuiTreeNodeFlags_DefaultOpen`), and the header's
      text color is pushed to `tokens::text_disabled` whenever
      `step.count == 0`, `tokens::text_primary` otherwise. No further
      code change needed — re-verified by re-reading the current
      `main.cpp` Trace block against this bullet's three requirements
      (present, collapsed, grayed) one by one.
- [x] `fixtures/`: one fixture whose expected trace (pass order and
      per-pass count) is asserted in a Rust unit test, guarding
      against silent fixpoint-loop reordering regressions.
      `fixtures/golf_trace.glsl` is a small shader deliberately chosen
      to trigger real work across several passes on the first
      fixpoint iteration (dead-local removal, constant folding,
      compound-assignment/increment-decrement rewriting, declaration
      merging, brace stripping) followed by a second, all-zero
      iteration that confirms the fixpoint — two full passes through
      the fixed 16-pass sequence, 32 steps total. The new
      `trace_pass_order_and_counts_match_fixture_regression` test in
      `golfer.rs` asserts the exact `(pass_name, count)` sequence, so
      reordering, adding, or removing a pass inside the
      `golf_with_protected_names_impl` fixpoint loop breaks this test
      even when every individual pass's own unit tests still pass in
      isolation.

### Phase 15 — v1.6.x — Automated multi-frame equivalence safety net

Phase 5's "Compare" mode is a manual, single-frame, human-judged
side-by-side. A professional golfing tool needs an automated,
repeatable proof that the golfed shader still renders identically to
the source before anyone ships it.

- [x] `src/render/shader_runner.cpp`: an offscreen dual-compile path
      (source program + golfed program, both already compiled for
      Compare mode) sampled at a configurable set of `iTime` values
      (default: `0.0, 0.5, 1.0, 2.5, 5.0`, user-editable) and a fixed
      `iResolution`/`iMouse` for determinism. New
      `EquivalenceSampleConfig` (a `sample_times` vector plus a fixed
      `resolution_x`/`resolution_y`, both with the defaults above) and
      `run_equivalence_samples()`, which takes the two already-compiled
      `ShaderRunner`s Compare mode already requires, renders each
      sample's `iTime` into a caller-owned pair of scratch
      `OffscreenFramebuffer`s (separate from the on-screen Compare-mode
      viewport framebuffers, so the safety net never disturbs what's
      currently on screen), and invokes a callback with the rendered
      pair so the next bullet's `glReadPixels` diff can read them back
      before the following sample overwrites them. Every uniform other
      than `iTime` is pinned for every sample and every run — not just
      `iResolution`/`iMouse` as this bullet names, but also
      `iFrame`/`iFrameRate`/`iDate`, since those are ordinary
      `ShaderUniforms` fields a shader can read just as easily and
      would otherwise reintroduce the exact run-to-run nondeterminism
      pinning `iResolution`/`iMouse` was meant to rule out. No UI
      wiring yet (later Phase 15 bullets); syntax- and link-verified
      against the real `framebuffer.cpp`/`shader_runner.cpp` with a
      stub GL loader, since the full SDL3/ImGui app itself is
      Windows-only per section 2 and cannot be run in this
      environment.
- [x] Per-sample `glReadPixels` readback into `platform/screenshot.cpp`
      buffers for both programs, per-pixel absolute difference reduced
      to a single max-delta and mean-delta pair.
- [x] Configurable tolerance (default `0`, i.e. bit-exact, since the
      golfer must never change behavior per the Phase 11 charter) with
      an explicit opt-in slider for float-reordering tolerance, since
      `fold_additive_float_constants` reassociation can legitimately
      shift float rounding by an ULP or two.
- [x] `status.ok`/`status.error` indicator (reusing the Phase 10.5
      compile-status dot component) next to the existing "Run golf"
      button: green once every sample passes tolerance, red with a
      "N/M samples differ, max delta X" detail line otherwise.
- [x] This is advisory, never blocking — the golfed output is always
      still shown and usable; the safety net's job is to make an
      unsafe pass immediately visible, not to gate the UI.

### Phase 16 — v1.7.x — Multi-document workspace & session persistence

Extends the single-shader Phase 3 three-panel layout to the
professional daily-driver case of golfing many shaders in one sitting
without losing state between them or across app restarts.

- [x] Tab strip above the Source/Golfed/Viewport dock, one tab per
      open `.glsl` file, reusing the Phase 10.4 tab visual language
      (flat rectangle, `accent` underline on the active tab).
- [x] Each tab owns its own editor buffer, golf result, aggressive
      pass toggles, protected-names list, and Phase 12 budget preset —
      switching tabs must not re-run the golf engine on the
      previously-active tab's content.
- [x] `.ushaderworkspace` JSON file: open file paths, active tab
      index, per-tab pass/profile state (superset of the Phase 13
      `.ushaderprofile` format), and window/panel layout (ImGui
      `.ini` layout string already saved by Dear ImGui, referenced by
      path).
- [x] Auto-save the workspace file to
      `%APPDATA%\ushader\last_session.ushaderworkspace` on clean exit;
      restored automatically on next launch behind a "Restore last
      session?" prompt, never silently reopening files without
      confirmation.
- [x] Unsaved-changes indicator (dot on the tab, matching the
      Premiere-style convention) and an exit-time "unsaved shaders"
      confirmation dialog listing every dirty tab.

### Phase 17 — v1.8.x — Batch pipeline mode for build integration

The existing `rust-core/src/bin/golf.rs` CLI golfs one file at a time
for interactive use. Studios embedding µShader in an asset pipeline
need a deterministic, scriptable, exit-code-driven batch mode — still
strictly offline, no daemon, no network.

- [x] `bin/golf.rs`: accept a directory or glob argument, golf every
      `.glsl` file found, using the same `golf_with_protected_names`
      entry point the GUI calls, guaranteeing CLI and GUI output can
      never diverge.
- [x] `--profile <path.ushaderprofile>` flag consuming the Phase 13
      profile format, so a build script uses the exact same
      pass/protected-names configuration an artist last saved in the
      GUI.
- [x] `--budget <preset>` flag consuming the Phase 12 presets;
      non-zero process exit code when any input file's compressed
      estimate exceeds the preset threshold — the mechanism a CI job
      needs to fail a build on size regression.
- [x] `--report <path.json|path.csv>`: per-file report (input path,
      raw/golfed/compressed byte counts, per-pass counters from
      `AggressiveStats`, pass/fail against budget), machine-readable
      for pipeline dashboards.
- [x] `--diff`: prints a unified source/golfed diff per file to stdout
      instead of writing golfed output, for a pre-commit dry-run.
- [x] No progress spinners or ANSI color by default (redirectable
      output for CI logs); a `--pretty` flag opts into the colored,
      human-oriented console output for local interactive use.

### Phase 18 — v1.9.x — Professional editor upgrades

Targeted additions to the Phase 4 text editor that professional daily
users of code-golf/shader-authoring tools expect and that the current
editor lacks.

- [x] Command palette (`Ctrl+Shift+P`, Premiere/VS-Code convention):
      fuzzy-searchable list of every existing action (run golf, toggle
      pass, load/save profile, switch tab, open Compare mode, export)
      — a thin dispatcher over actions that already exist in
      `golf_controls.cpp`/`layout.cpp`, not new functionality.
- [x] Rebindable keyboard shortcuts: a `keybindings.json` under
      `%APPDATA%\ushader\`, editable through a new panel in the About
      tab area (Phase 7), read at startup with hardcoded defaults as
      fallback when the file is absent or malformed.
- [x] Minimap for the Source/Golfed editors (`ui/glsl_language.cpp`'s
      keyword/identifier tables are reused via a new
      `classify_glsl_token()` — not a duplicated lexer — to render the
      minimap's colored strip), toggleable, off by default below a
      configurable line-count threshold where it adds no value.
- [x] Inline unified diff view as a fourth Source/Golfed/Viewport/Trace
      panel option (tabbed alongside Trace in the same dock slot):
      token-level highlighting of removed (`status.error`-tinted
      strike) versus added (`status.ok`-tinted) spans between Source
      and Golfed, built on the Phase 14 trace data's final before/after
      pair (`golf_trace.front().before` / `golf_trace.back().after`)
      rather than a new diff engine — a simple LCS-based token diff,
      falling back to a line-level diff for very large inputs.
- [x] Drag-and-drop `.glsl` files onto the main window to open a new
      tab (depends on Phase 16 tabs), plus a Recent Files list under
      the File menu backed by the same `%APPDATA%\ushader\` store as
      keybindings.

### Phase 19 — v2.0.x — Local session reports

A no-network, no-cloud way to produce a shareable artifact of a golf
session — for a portfolio entry, a client deliverable, or a demoscene
compo submission writeup — without leaving the offline-first
constraint.

- [x] `src/report/` (new): renders a self-contained, single-file HTML
      report — source code, golfed code, both syntax-highlighted with
      the existing `glsl_format.cpp` rules, the Phase 12 budget
      badges, per-pass counters, and the Phase 15 equivalence-check
      result, with all CSS inlined and no external asset references
      (consistent with Offline-First Isolation: the report itself must
      open correctly with zero network access, e.g. on a judge's
      offline machine). Implemented as `src/report/report.cpp`
      (`render_session_report_html`) plus a small zero-dependency
      `src/report/report_encoding.cpp` (`html_escape`/`base64_encode`,
      unit-tested standalone with `g++` per the Phase 14 precedent,
      since the full ImGui/SDL3 app cannot be built in this
      environment). Syntax highlighting reuses
      `glsl_language.cpp`'s `classify_glsl_token()` (the same rules
      the minimap and editors use) over a tokenizer matching
      `minimap.cpp`'s word-run/punctuation split; golfed code is run
      through `format_glsl()` first for readability, mirroring the
      Golfed panel's existing "Formatted view" toggle.
- [x] Optional embedded viewport screenshot (reuses Phase 6's PNG
      capture path) as a base64-inlined `<img>`, off by default to
      keep report file size small when not wanted. New
      `encode_framebuffer_png_to_memory()` in `platform/screenshot.cpp`
      (`stbi_write_png_to_mem` over the existing `read_pixels()` path)
      feeds `report.cpp`'s base64 encoder; a new "Include screenshot
      in report" checkbox next to the Viewport panel's existing
      Screenshot button controls it, off by default.
- [x] "Export report…" action in the File menu and the Phase 18
      command palette, writing to a user-chosen path via the Phase 6
      file-dialog integration.
- [x] Optional PDF variant: local HTML-to-PDF via a bundled, embedded
      renderer only (no system-installed browser dependency, no
      network call) — if no such offline-capable embeddable renderer
      can be sourced and vendored under Offline-First Isolation, this
      bullet is dropped in favor of "print the HTML report from any
      browser" and documented as such in `README.md`, rather than
      silently violating the zero-network-dependency rule.

      **Resolution: dropped, per the fallback above.** Every embeddable
      HTML-to-PDF option surveyed fails Offline-First Isolation or is
      disproportionate to a single optional export format:
      - **wkhtmltopdf** is itself a bundled QtWebKit browser engine —
        the exact "system-installed browser dependency" this bullet
        says to avoid, just vendored instead of system-installed — and
        it has been unmaintained/archived since 2020, so shipping it
        in a 2026 product means vendoring an unpatched WebKit.
      - **CEF/Chromium embedding** satisfies "no network call" but not
        the spirit of "bundled, embedded renderer": it is a full
        browser engine, on the order of 100+ MB, versus the app's
        current SDL3/OpenGL/ImGui shell and single bundled
        `ffmpeg.exe` — it would dominate the installer size for one
        optional export format on a single report screen.
      - **WeasyPrint** is pure-Python and does the layout itself (no
        browser), but pulls in Cairo/Pango/GDK-Pixbuf native
        dependencies with no lightweight, statically-linkable Windows
        build suited to silent vendoring into a C++ installer alongside
        `ushader.exe`.
      - **litehtml** is the closest fit in spirit (small, MIT, embeddable,
        no JS, no network) but it is an HTML/CSS layout+paint engine
        only — it renders into a canvas the host provides, not to PDF.
        Producing a PDF would mean writing a new PDF-drawing backend
        for it in-house, which is net-new rendering-engine work, not
        "sourcing and vendoring" an existing renderer as this bullet
        calls for.

      Decision: dropped per the documented fallback. The HTML report
      remains the only export format; `README.md` now documents
      printing it to PDF from any browser (`Ctrl+P` → "Save as PDF"),
      which needs no vendored renderer and no network call, since the
      browser doing the printing is the user's own, already-installed
      one and the report file itself is still fully self-contained and
      offline-openable.

### Phase 20 — v2.1.x — Display correctness & accessibility

Multi-monitor and accessibility gaps the Phase 10 visual overhaul
did not target, closed as their own dedicated professional-polish
phase.

- [x] Correct per-monitor DPI handling: SDL3 high-DPI window flags,
      ImGui `FontGlobalScale`/style scaling recomputed on
      `SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED`, verified by dragging
      the window between two monitors with different scale factors.
      Implemented as `SDL_WINDOW_HIGH_PIXEL_DENSITY` on window creation,
      a new `ui/theme.cpp`'s `apply_dpi_scale(io, scale)` that rescales
      from a captured, unscaled style snapshot (so repeated calls never
      compound) and sets `io.FontGlobalScale`, called once at startup
      with `SDL_GetWindowDisplayScale()` and again from `main.cpp`'s
      event loop whenever `SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED`
      fires — SDL doesn't carry the new scale in the event payload, so
      the handler re-queries `SDL_GetWindowDisplayScale(window)` for
      the window's now-current display. Fonts stay rasterized at their
      Phase 10 fixed pixel size (no atlas rebuild per monitor), so text
      is upscaled via `FontGlobalScale` like the widget metrics rather
      than re-rendered crisp at the new DPI — acceptable per this
      bullet's own wording ("FontGlobalScale/style scaling"), with some
      softening above roughly 150% scale.
- [x] User-adjustable base UI font size (separate from the fixed
      Phase 10.2 metrics), persisted per Phase 16 workspace/user
      settings, with panel paddings/line-heights that scale
      proportionally rather than only the font glyph size.
      Implemented as a new "Appearance" tab (next to About/Keyboard
      Shortcuts) with a 13–28pt slider over `ui/theme.h`'s
      `kDefaultBaseFontSize`/`kMinBaseFontSize`/`kMaxBaseFontSize`.
      Rather than reloading the font atlas per size — which the fixed
      Phase 10.2 rasterization doesn't do per-monitor either — the
      chosen size is expressed as a ratio against the default and
      combined multiplicatively with the Phase 20.1 DPI scale into one
      factor fed to `apply_dpi_scale()`, so paddings and line-heights
      (via `ImGuiStyle::ScaleAllSizes`) grow with the glyphs rather
      than the text alone. Persisted as `ui_font_size` on
      `WorkspaceState` (`ui/workspace.h`/`.cpp`) — a new top-level,
      non-per-tab field alongside `active_tab`/`layout_ini` in the same
      `last_session.ushaderworkspace` file Phase 16 already writes —
      loaded and applied at startup ahead of (and independent from) the
      "Restore last session?" tab-reopen confirmation, since it's an
      appearance setting rather than reopened file content. A missing,
      malformed, or out-of-range value falls back to the 18pt default.
- [x] Colorblind-safe alternate token set for `status.ok` /
      `status.warning` / `status.error` (shape/icon-differentiated,
      not color-only — a small filled circle/triangle/square instead
      of three same-shaped colored dots), toggleable, sitting on top
      of the existing Phase 10.1 token table rather than replacing it.
      Implemented as a `StatusKind` (`Ok`/`Warning`/`Error`) parameter
      on `ui/theme.cpp`'s `render_status_dot()` — the literal
      same-shaped-dot indicator used for the equivalence-check and
      shader-compile status — replacing its previous raw-`ImVec4`
      signature; the function still looks the fill color up from the
      unchanged Phase 10.1 `status.ok`/`status.warning`/`status.error`
      tokens either way, so the toggle only ever changes the shape, not
      the token table. A new global `g_colorblind_safe_indicators`
      (off by default, not persisted — same precedent as the existing
      Minimap toggle) is flipped from a checkbox in the Phase 20.2
      "Appearance" tab; when on, Warning renders as a filled triangle
      and Error as a filled square, both sized to the same footprint as
      the Ok circle so row layout doesn't shift when toggled.
- [x] Windows UI Automation names/roles set on every custom-drawn
      control introduced since Phase 10 (title-bar buttons, themed
      checkboxes, icon buttons) so the app is at minimum
      screen-reader-nameable, even though ImGui's immediate-mode model
      limits full accessibility-tree fidelity. Implemented as a new
      `platform/accessibility.cpp`: a `WM_GETOBJECT` handler installed
      by subclassing the SDL window's `WNDPROC` (`GWLP_WNDPROC`) at
      startup, returning a minimal `IRawElementProviderFragmentRoot` /
      `IRawElementProviderFragment` / `IRawElementProviderSimple` COM
      tree via `UiaReturnRawElementProvider()`. Each frame, the three
      call sites (title-bar minimize/maximize/close in `main.cpp`,
      `themed_checkbox()` and the shared `icon_button_ex()` helper)
      register their current name, `ButtonControlType`/
      `CheckBoxControlType`, and screen-space bounding rect into a
      small registry (`accessibility_begin_frame()` /
      `accessibility_register()` / `accessibility_end_frame()`,
      converted from ImGui client coordinates to desktop screen
      coordinates via `ClientToScreen()` once per frame so UIA queries
      never need to touch app state directly); checkboxes also expose
      `IToggleProvider::get_ToggleState()`. Matching the bullet's own
      "at minimum screen-reader-nameable" wording rather than claiming
      full fidelity: `IsKeyboardFocusable` is reported `false` and
      `Invoke`/`Toggle` are no-ops, since real keyboard-driven
      activation would require restructuring every immediate-mode
      click call site (`if (icon_button(...)) { ... }`) to route
      through a provider callback instead of a same-frame return value
      — out of proportion to a "nameable" bar. A screen reader (e.g.
      Narrator) can still announce name, role, and location by mouse/
      touch hover or by UIA content-view navigation. Status dots
      (`render_status_dot()`) were intentionally left out, since the
      bullet's own enumeration only lists title-bar buttons, themed
      checkboxes, and icon buttons.
- [x] Contrast re-verification (same WCAG AA script used in 10.1/10.8)
      run against both the default and colorblind-safe token sets.
      Implemented as `scripts/check_contrast.py`, the same relative-
      luminance/contrast-ratio method as 10.1/10.8 (not previously
      checked into the repo), now parsing `theme_tokens.h` directly
      rather than hand-copied hex values so it can be re-run after any
      future token edit. Re-verified all four `text.primary`/
      `text.secondary` vs `bg.panel`/`bg.app` pairs: 12.59:1, 13.36:1,
      5.87:1, 6.22:1 — matching the 10.1/10.8 figures exactly, all
      above the 4.5:1 AA threshold. The "colorblind-safe token set"
      named in this bullet does not exist separately: Phase 20.3 (see
      above) reuses the unchanged Phase 10.1 `status.ok`/`warning`/
      `error` tokens and only swaps the drawn shape, so running the
      same script against it reproduces the identical numbers — the
      script prints both runs and that fact explicitly rather than
      silently skipping the second pass.

### Phase 21 — v2.2.x — Offline interop with other golf/shader tools

Closes out the professional-integration arc with local, file-based
compatibility for artists who already use other established,
non-networked shader tools alongside µShader — still zero network
dependency, since every adapter here is a local text-format reader or
a local boilerplate template, never an API client.

- [x] Import adapter for Shader Minifier–style exclude-name lists
      (plain-text, one identifier per line) into the Phase 5
      protected-names field, so a project's existing exclude list does
      not need retyping. Implemented as a new
      `ui/exclude_list_import.h`/`.cpp`: `parse_exclude_name_list()`
      reads the plain-text list (one identifier per line, `\n` or
      `\r\n`, blank lines skipped, and — beyond the format Shader
      Minifier itself writes — lines starting with `//` or `#` are
      also tolerated as comments, so a hand-edited list still imports
      cleanly) and `merge_protected_names()` appends any new names to
      the existing Phase 5 comma-separated field without duplicating
      ones already present, preserving the existing list's order and
      its own already-typed entries rather than overwriting them. A
      new "Import exclude list..." button sits next to the "Protected
      names" field in `ui/golf_controls.cpp`, opening a standard
      Windows file-open dialog (`.txt` filter) via the existing
      `platform/file_dialog` helper — a local file read, no different
      in kind from the existing "Load profile..." action, so nothing
      new crosses the Offline-First Isolation line. Covered by
      `tests/exclude_list_import_test.cpp`.
- [x] Export wrappers: one-click "Copy as Shadertoy `mainImage`",
      "Copy as Bonzomatic-ready source", and "Copy as bare
      `void main()`" variants of the golfed output, each a fixed local
      string template — not a live integration with any of those
      tools, since none is networked and Offline-First Isolation rules
      out anything that would require one running. Implemented as a
      new `ui/export_wrappers.h`/`.cpp` with three pure string
      functions, plus matching one-click buttons in the Golfed panel
      toolbar and matching command-palette entries (`main.cpp`), each
      copying straight to the clipboard via `SDL_SetClipboardText`
      like the existing "Copy" action. `wrap_as_shadertoy_main_image()`
      and `wrap_as_bonzomatic_source()` are both the identity function
      on the golfed text: Shadertoy's Image tab and Bonzomatic (which
      was built to keep shader-showdown entries portable to and from
      Shadertoy) both auto-inject the same uniform set — `iTime`,
      `iResolution`, `iMouse`, `iDate`, `iFrame`, `iFrameRate`, plus a
      few Bonzomatic doesn't use, `iTimeDelta`/`iChannelTime`/
      `iChannelResolution`/`iSampleRate`/`iChannel0..3` — and call the
      user's `mainImage(fragColor, fragCoord)` themselves, which is
      already exactly µShader's own golfed output format; the two
      buttons are kept as separate, clearly-labeled one-click actions
      for workflow clarity (pasting into the right tool without having
      to remember they happen to be the same text) rather than
      collapsed into one, and are free to diverge later if either
      target's expected format ever does. `wrap_as_bare_main()` does a
      real transformation: `extract_main_image_signature()` regex-reads
      the *actual* current parameter names out of the golfed
      `mainImage(out vec4 X, in vec2 Y)` signature — golfing renames
      these like any other locals, so they're rarely still literally
      `fragColor`/`fragCoord` — and the wrapper prepends the same
      `iTime`/`iResolution`/`iMouse`/`iDate`/`iFrame`/`iFrameRate`
      uniform declarations the Phase 1 `render/shader_runner.cpp`
      already prepends internally for the live preview, then appends a
      standalone `void main(){ vec4 X; vec2 Y = gl_FragCoord.xy;
      mainImage(X, Y); gl_FragColor = X; }`. Unlike
      `shader_runner.cpp`'s own core-profile wrapping this adds no
      `#version` line and writes to the builtin `gl_FragColor` instead
      of a declared `out vec4`, since "bare" is read here as the most
      widely-portable option — the compatibility-profile form most
      raw/legacy GLSL sandboxes and 4k-style bare shaders accept
      without any pragma. If the signature can't be found (already
      broken source, or no `mainImage` at all), the function returns
      the input unchanged rather than emitting a wrapper around
      nothing. Covered by `tests/export_wrappers_test.cpp`.
- [x] `.ushaderprofile` (Phase 13) documented, versioned JSON schema
      published in `docs/` so external tooling can generate/consume
      profiles without reverse-engineering the format. Published as
      `docs/ushaderprofile.schema.json` (a JSON Schema draft 2020-12
      document covering all 19 fields — the 16 boolean pass toggles,
      `protected_names`, `budget_preset` with its 6-value enum kept in
      sync with `ui/budget_presets.cpp`, and the new `schema_version`
      field itself) plus `docs/ushaderprofile-schema.md`, a
      human-readable spec: field table, compatibility rules (unknown
      fields ignored, missing optional fields fall back to a
      documented default), and a schema-version history table. Made
      genuinely "versioned" rather than just documented after the
      fact: `ui/golf_profile.cpp`'s `serialize_golf_profile()` now
      stamps `"schema_version": 1` as the first field of every newly
      written profile, via a small `append_int_field()` helper next to
      the existing `append_bool_field()`. This is additive and
      backward-compatible on purpose — `deserialize_golf_profile()`
      already ignores any field it doesn't explicitly look up, so
      every `.ushaderprofile` written before this change (no
      `schema_version` field at all) continues to load unchanged and
      is documented as implicitly schema version 1.
      `fixtures/sample.ushaderprofile` (loaded by
      `tests/golf_profile_roundtrip_test.cpp`) gained the field too,
      verified not to break that test's round-trip. A new
      `tests/ushaderprofile_schema_test.cpp` cross-checks the
      documented field list and the documented `budget_preset` enum
      against `serialize_golf_profile()`'s real output and
      `budget_presets()`'s real preset list respectively, so the
      published schema can't silently drift from the implementation.
- [ ] `bin/golf.rs` batch mode (Phase 17) documented with example
      invocations for common local build systems (MSBuild pre-build
      step, a plain `.bat` watch script) in `README.md` — documentation
      only, no bundled MSBuild/watcher integration itself.
- [ ] Final acceptance for the Phase 12–21 arc: every new panel/CLI
      flag traces to an entry in this ROADMAP.md, every new user-facing
      behavior has a `CHANGELOG.md` line, `README.md` screenshots are
      refreshed for any new panel (Trace, Diff), and nothing in Phases
      12–21 introduces a network call, a non-English string, a source
      comment, or a non-Windows code path — verified against section 2
      before tagging `v2.2.0`.

### Phase 22 — v2.3.x — WinUI 3 migration: architecture decision & feasibility spike

µShader's Dear ImGui/SDL3 shell has reached its ceiling: immediate-mode
rendering cannot produce Fluent Design's layered materials (Mica,
Acrylic), real vector/SVG iconography, or composition-based motion
(connected animations, Reveal, Lottie micro-interactions) without
essentially reimplementing a retained-mode UI framework by hand inside
ImGui — which is exactly what WinUI 3 already is. This phase is a
decision-and-spike phase only: no panel is rewritten yet (that starts
at Phase 24); it exists to lock the architecture so Phases 23–27 do
not thrash.

- [x] **Toolchain**: **C++/WinRT**, not C#/.NET. Rejected C#/.NET
      despite its faster XAML iteration loop, because it would force
      the existing direct, zero-marshaling call into `rust-core`'s C
      ABI (`ushader_golf`, `ushader_golf_traced`, `ushader_estimate_budget`,
      …) through P/Invoke, and would add the .NET runtime as a new
      dependency the app has never had — in tension with Offline-First
      Isolation's "embedded locally" spirit even where .NET can be
      self-contained-deployed. C++/WinRT keeps `main.cpp` calling into
      `include/ushader/golf_core.h` exactly as today; only the UI shell
      above it changes.
- [ ] **Windows App SDK channel**: pin to the latest stable (non-
      preview) Windows App SDK release available at implementation
      time, self-contained deployment (the runtime is packaged with
      the app, not a separate system install) — required by Offline-
      First Isolation and by the existing "no separate runtime install"
      user experience the Inno Setup installer already provides.
- [ ] **Rendering strategy: ANGLE**, not a GLSL→HLSL/Direct3D11
      rewrite. WinUI 3's `SwapChainPanel` has no native OpenGL context
      — a raw D3D11 rewrite of `render/shader_runner.cpp` was
      considered and rejected for this migration: it would mean
      writing and maintaining a GLSL-to-HLSL translation layer (or
      vendoring glslang+SPIRV-Cross) for a renderer that already works
      correctly, doubling the surface area this migration has to prove
      correct via Phase 15's equivalence safety net. ANGLE (GLES 3-
      over-D3D11) lets `shader_runner.cpp`'s GL call sites, the
      Shadertoy uniform contract (`iTime`/`iResolution`/`iMouse`/
      `iDate`/`iFrame`/`iFrameRate`), and the Rust golfing engine stay
      completely untouched — only the window/swapchain plumbing around
      the renderer changes. Tradeoff accepted knowingly: ANGLE adds
      `libEGL.dll`/`libGLESv2.dll` as bundled binaries (same "vendored
      next to `ushader.exe`, shipped by the installer" precedent Phase
      9 already set for `ffmpeg.exe`) and a BSD/Apache-2.0 attribution
      entry in `THIRD_PARTY_NOTICES.md`, in exchange for a near-zero-
      risk renderer port.
- [ ] **Packaging model**: unpackaged, self-contained `.exe` +
      Inno Setup installer — not MSIX/Microsoft Store. WinUI 3 apps
      are commonly packaged as MSIX, but µShader has no Store
      presence, no need for MSIX's per-user install/AppContainer
      sandboxing, and switching would mean replacing the working
      `installer/ushader.iss` pipeline (Phase 8) for no functional
      gain. Windows App SDK's self-contained, unpackaged deployment
      mode is explicitly designed for exactly this case and keeps
      Phase 8's installer conventions (icons, versioning, unsigned-
      until-a-cert-exists) intact.
- [ ] **Removal list** (executed across Phases 23–27, not this one):
      Dear ImGui (all `imgui_*` sources and the docking branch fetch),
      SDL3 (window creation, event pump, clipboard, borderless-window
      hit-testing from Phase 10.3), the bundled ImGui text-editor
      widget, and the Lucide icon font. `rust-core/`, `include/ushader/
      golf_core.h`, every `platform/*` file not tied to SDL3 window
      management (file dialogs, paths, recorder, screenshot, utf8),
      and every `report/*` file are unaffected and carry over as-is.
- [ ] **Repository layout changes** (supersedes section 5 once
      Phase 27 lands): `src/render/` keeps its GL code, gains an ANGLE-
      hosting `SwapChainPanel` adapter; `src/ui/` (ImGui panels) is
      replaced by `src/shell/` (XAML views/view-models) and
      `assets/xaml/` (resource dictionaries, control templates);
      `assets/icons/` gains an `assets/icons/ui/*.svg` subtree (Phase
      25); `assets/fonts/lucide.ttf` is removed once Phase 25 finishes
      the SVG icon cutover.
- [ ] **Risk register**, reviewed again at the end of each subsequent
      phase rather than only here: (1) ANGLE's DXGI swapchain lifetime
      interacting with XAML's own composition swapchain — validated by
      Phase 23's spike before any panel work starts; (2) a bespoke
      GLSL-highlighting code editor is real UI-control engineering, not
      a drop-in replacement (Phase 26 is scoped as the largest phase in
      this arc accordingly); (3) Fluent Design's authentic corner-
      radius/elevation system does not map 1:1 onto the flat, 2px-
      radius Phase 10 Premiere language — Phase 24 explicitly documents
      this as a deliberate visual-language change, not a bug, so
      reviewers don't flag the new rounding as a Phase 10 regression.
- [ ] `CHANGELOG.md` entry noting the architecture decision (no user-
      facing behavior change yet — this phase ships no runtime code).

### Phase 23 — v2.4.x — Rendering pipeline: ANGLE inside a SwapChainPanel

- [ ] `src/render/`: new `AngleSwapChainHost` adapter — creates an EGL
      display/context/surface bound to a `Windows::UI::Xaml::Controls::
      SwapChainPanel` via `ISwapChainPanelNative::SetSwapChain`,
      replacing SDL3's `SDL_GL_CreateContext`/`SDL_GL_SwapWindow` pair.
      `ShaderRunner`'s own GL calls (compile, link, uniform upload,
      `glDrawArrays` fullscreen-triangle draw) are untouched.
- [ ] Resize handling: `SwapChainPanel::SizeChanged` drives an EGL
      surface resize + GL viewport update, replacing SDL3's
      `SDL_EVENT_WINDOW_RESIZED` handler.
- [ ] Bundle `libEGL.dll`/`libGLESv2.dll` next to `ushader.exe`
      (build-time fetch, mirroring Phase 9's `ffmpeg.exe` precedent)
      and add the ANGLE attribution to `THIRD_PARTY_NOTICES.md`.
- [ ] Re-run the Phase 15 automated multi-frame equivalence safety net
      (source vs. golfed, bit-exact by default) against the ANGLE-
      hosted renderer to prove the swapchain-hosting change introduces
      no pixel drift, before any XAML shell work begins on top of it.
- [ ] Compile/link error surfacing (Phase 3) re-wired from stdout/ImGui
      into a shell-agnostic error-state struct the eventual Phase 24
      shell reads — decoupled here so Phase 24 has no renderer-side
      work left to do.
- [ ] Standalone spike target (no full shell yet): a bare XAML window
      with one `SwapChainPanel` rendering the existing hardcoded
      default shader, animating via `iTime`, resizable — the minimum
      proof this phase's architecture decision holds before Phase 24
      commits to building the real shell on top of it.

### Phase 24 — v2.5.x — XAML application shell, navigation & Fluent chrome

- [ ] `App.xaml`/`MainWindow.xaml` scaffolding, Windows App SDK
      self-contained deployment wired into CMake (replacing the SDL3/
      Dear ImGui `FetchContent` entries from Phase 0/3).
- [ ] Custom title bar via `AppWindow::TitleBar::ExtendsContentIntoTitleBar`
      + a Mica (or Mica Alt) backdrop, replacing Phase 10.3's SDL3
      `SDL_WINDOW_BORDERLESS` + manual `SDL_HitTest` chrome entirely —
      WinUI 3's own title-bar APIs supersede that hand-rolled
      drag-region/hit-test/button code.
- [ ] Multi-document dock: `TabView` for the open-shader tab strip
      (Phase 16 parity) hosting a `NavigationView` or `Pivot`-based
      Source/Golfed/Viewport/Trace/Diff panel switcher per tab —
      Fluent-native equivalents of the Phase 3 three-panel layout and
      Phase 18's fourth Trace/Diff slot.
- [ ] `theme_tokens.h` → XAML `ResourceDictionary`: every Phase 10.1
      named token (`bg.app`, `bg.panel`, `accent`, `status.ok`, …)
      re-expressed as a `SolidColorBrush`/`Color` resource, still one
      theme only (no light/dark toggle — same one-theme rule as
      Phase 3/10, just re-hosted). Re-run the Phase 10.1/20.5 WCAG AA
      contrast script against the re-expressed brushes before closing
      this bullet.
- [ ] Corner radius: adopt Fluent's authentic control corner-radius
      scale (4px small controls, 8px cards/panels) — a deliberate,
      documented departure from Phase 10.2's flat `2px`-everywhere
      Premiere language, not a regression of it. `ThemeShadow` gives
      panels real elevation (docked panels, flyouts, the command
      palette from Phase 26) in place of Phase 10's flat, shadowless
      surfaces.
- [ ] Reveal hover/focus highlight on interactive controls (buttons,
      tab items, list rows) — the Fluent-native replacement for the
      hand-rolled `bg.hover`/`bg.active` state colors Phase 10.4 wired
      into ImGui's `ImGuiCol_*` slots.
- [ ] Acrylic backdrop reserved for transient surfaces only (flyouts,
      the Phase 26 command palette) — not the main panel backgrounds,
      to keep the Program-Monitor-style opaque `bg.field.deepest`
      viewport (Phase 10.5) legible and consistent with a video/shader
      tool's expectations rather than letting content show through.
- [ ] `docs/screenshot.png` retaken against the new bare shell (no
      panels ported yet beyond Viewport) purely to document chrome
      progress; a full retake happens again at Phase 27 acceptance.

### Phase 25 — v2.6.x — Iconography, SVG/PNG asset system & motion FX

This is the phase that makes the app "as beautiful as possible" in
concrete, buildable terms: every visual asset touched, real vector
iconography, and genuine (not decorative-only) motion.

- [ ] Replace `assets/fonts/lucide.ttf` glyph-font icons with a real
      SVG icon set under `assets/icons/ui/*.svg`, rendered via XAML
      `PathIcon`/`AnimatedIcon` or a Win2D `CanvasSvgDocument` where an
      icon needs multi-color fills the single-path `PathIcon` can't
      express — resolves the Phase 3 `MergeMode` font-atlas bug
      outright (SVG icons never share a glyph atlas with body text, so
      they can finally sit inline inside window/tab titles, which
      Phase 3 explicitly could not do).
- [ ] Icon color states (idle `text.secondary`, hover `text.primary`,
      active `accent` — Phase 10.6's scheme) re-implemented as XAML
      `VisualStateManager` states bound to the Phase 24 brush
      resources, animated with a short (~100ms) brush-transition
      rather than an instant color swap.
- [ ] Regenerate `assets/icons/app_source.png` → multi-scale PNG set
      (44/71/150/310px tiles, plus the existing `.ico` sizes) and
      `docs/logo.png` at higher resolution — Phase 10.6 explicitly
      deferred both regenerations as "not worth the risk" under the
      old ImGui/flat-icon look; the SVG/Fluent visual language is a
      large enough identity shift that both are revisited here
      instead of reused as-is a second time.
- [ ] Motion FX, vendored offline (zero network dependency, same rule
      as the Phase 9 bundled ffmpeg / Phase 23 bundled ANGLE):
      - Micro-interactions via `AnimatedVisualPlayer` + a small set of
        bundled Lottie/Bodymovin `.json` animations — Run-golf success
        pulse, compile-error shake on the status dot, tab-open/close.
      - `ConnectedAnimation` when switching the active tab in the
        Phase 24 `TabView`, and when expanding a Phase 14 Trace-tab
        pass entry.
      - A subtle animated pulse on the Phase 10.5/15 status dot while
        an equivalence check or golf run is in flight, replacing the
        old static dot with a genuine busy state.
      - Shimmer/skeleton placeholder for the Viewport panel during the
        brief ANGLE surface (re)initialization window on startup or
        resize, instead of a blank black frame.
- [ ] Acceptance: every icon in the app is SVG-sourced (no remaining
      glyph-font `PushFont()`/`PopFont()` call sites — that pattern no
      longer exists once this phase closes), and at least the four
      motion points above are wired to real UI events, not only
      present as unused assets.

### Phase 26 — v2.7.x — Editor & panel feature parity

The largest phase in this arc, per Phase 22's risk register: WinUI 3
ships no code-editor control, so the Phase 4/18 editor experience has
to be rebuilt, not just re-skinned. No phase here is considered done
until its ImGui-era equivalent's behavior is demonstrably matched.

- [ ] Bespoke GLSL code-editor control: a `RichEditBox`-hosted
      colorizer (background re-tokenization on text change, using the
      existing `classify_glsl_token()` rules from `ui/glsl_language.cpp`/
      `ui/minimap.cpp` — reused, not rewritten) if `RichEditBox`'s
      run-formatting API proves sufficient at the spike stage; falls
      back to a custom Win2D-rendered text-layout control (full manual
      caret/selection/scroll handling) if it does not. This fallback
      decision is made and documented at the start of this phase, not
      discovered midway.
- [ ] Feature parity checklist against the retired ImGui editor:
      syntax highlighting (Phase 4), error-line highlighting (Phase 4),
      read-only Golfed view + Formatted toggle (Phase 4), minimap
      (Phase 18), inline unified diff view (Phase 18), the Phase 14
      Trace tab's per-pass before/after panes.
- [ ] Command palette (Phase 18) rebuilt on a Fluent `CommandBarFlyout`/
      `AutoSuggestBox` combo — same fuzzy-search-over-existing-actions
      behavior, new host control.
- [ ] Rebindable keybindings (Phase 18), drag-and-drop `.glsl` files
      onto the main window (Phase 18), and the Recent Files list —
      ported to WinUI 3's own drag-drop and accelerator-key APIs;
      the underlying `%APPDATA%\ushader\keybindings.json` format and
      recent-files store are unchanged.
- [ ] Golf controls, Stats, Compare, Appearance, and About panels
      (Phases 5, 12, 15, 20, 7) rebuilt as XAML views bound MVVM-style
      (`x:Bind`) to the existing C++ state structs — a binding-layer
      change only; no golfing/budget/equivalence logic is touched.
- [ ] Windows UI Automation (Phase 20.4) is substantially simplified by
      this phase rather than extended: WinUI 3 controls expose UIA
      automation peers natively, so the Phase 20.4 `WM_GETOBJECT`/
      manual-provider shim becomes dead code to delete at Phase 27,
      not code to port.

### Phase 27 — v3.0.x — ImGui/SDL3 removal, packaging & acceptance

The phase that makes the switch real: nothing UI-related may still
depend on Dear ImGui, SDL3, or the Lucide font once this phase closes.

- [ ] Delete Dear ImGui, SDL3, and the ImGui text-editor widget: source,
      `FetchContent`/vcpkg entries, and every remaining `imgui_*`/`SDL_*`
      call site (the Phase 20.4 UIA shim included, per Phase 26's note).
      `src/ui/` is removed; `src/shell/` (Phase 24) is the only UI tree.
- [ ] `CMakeLists.txt` rewritten for C++/WinRT + self-contained Windows
      App SDK, `rust-core`'s Cargo-via-CMake wiring (Phase 1) unchanged.
- [ ] `installer/ushader.iss` updated: bundles the self-contained
      Windows App SDK runtime and the Phase 23 ANGLE DLLs alongside
      `ushader.exe`, keeping the existing "no separate runtime install"
      user experience; installer/app icons regenerated at Phase 25's
      new resolutions.
- [ ] Full regression pass of every Phase 12–21 feature against the new
      shell (budgets, profiles, trace, equivalence checks, workspace/
      session persistence, batch CLI — the CLI itself is UI-independent
      and unaffected, but its `.ushaderprofile`/report consumers in the
      GUI must still round-trip correctly).
- [ ] WCAG AA contrast re-verification (`scripts/check_contrast.py`,
      Phase 10.1/20.5 precedent) against the final Phase 24 XAML brush
      resources.
- [ ] `docs/screenshot.png` retaken against the fully-ported shell;
      `README.md` rewritten to describe the WinUI 3/Fluent shell in
      place of the retired Dear ImGui description.
- [ ] `CHANGELOG.md` `3.0.0` entry summarizing the Phase 22–27 arc as
      one breaking change (UI framework replacement — build toolchain,
      contributor setup, and packaging all change; no end-user-facing
      capability from Phases 0–21 is removed), tag `v3.0.0`.
- [ ] Re-verify every section 2 convention still holds against the new
      codebase (English-only, no source comments, Windows-10/11-only,
      Offline-First Isolation, MIT license) before tagging — the same
      acceptance discipline Phase 21's closing bullet applied to the
      12–21 arc, applied here to 22–27.

---

## 7. Explicitly out of scope for v1.0

- **Multi-buffer Shadertoy rendering** — no buffers A–D, no
  `iChannel` wiring, no shared "Common" pass. µShader golfs and
  previews a single `mainImage` fragment shader only. This is a
  deliberate design rejection, not a "later" item.
- ~~Dark theme / any theme other than white~~ — superseded by
  Phase 10 (v1.2.x); still only one theme at a time, no user-facing
  theme toggle, just a different single theme
- Any language other than English in the UI or source
- Any AI-tool attribution anywhere in the repository, commit history,
  or contributors list
- **MSIX/Microsoft Store packaging** (Phase 22) — the WinUI 3 rewrite
  stays unpackaged and self-contained via Inno Setup, matching the
  existing installer's distribution model; no Store listing is created.
- **A GLSL→HLSL/Direct3D11 renderer rewrite** (Phase 22/23) — ANGLE
  hosts the existing OpenGL renderer as-is; native Direct3D shader
  translation was evaluated and rejected as unnecessary risk for this
  migration, not deferred for later.
- **A light theme, or a theme toggle, in the WinUI 3 shell** — the
  Phase 24 XAML re-hosting of the Phase 10 dark tokens is still one
  theme only, same rule as every prior phase.
- **Multi-buffer Shadertoy rendering** remains out of scope through the
  WinUI 3 rewrite as well — Phase 22–27 changes the shell and the
  window/swapchain host, never the single-`mainImage`-pass rendering
  model itself.
