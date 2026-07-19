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
- [ ] Optional PDF variant: local HTML-to-PDF via a bundled, embedded
      renderer only (no system-installed browser dependency, no
      network call) — if no such offline-capable embeddable renderer
      can be sourced and vendored under Offline-First Isolation, this
      bullet is dropped in favor of "print the HTML report from any
      browser" and documented as such in `README.md`, rather than
      silently violating the zero-network-dependency rule.

### Phase 20 — v2.1.x — Display correctness & accessibility

Multi-monitor and accessibility gaps the Phase 10 visual overhaul
did not target, closed as their own dedicated professional-polish
phase.

- [ ] Correct per-monitor DPI handling: SDL3 high-DPI window flags,
      ImGui `FontGlobalScale`/style scaling recomputed on
      `SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED`, verified by dragging
      the window between two monitors with different scale factors.
- [ ] User-adjustable base UI font size (separate from the fixed
      Phase 10.2 metrics), persisted per Phase 16 workspace/user
      settings, with panel paddings/line-heights that scale
      proportionally rather than only the font glyph size.
- [ ] Colorblind-safe alternate token set for `status.ok` /
      `status.warning` / `status.error` (shape/icon-differentiated,
      not color-only — a small filled circle/triangle/square instead
      of three same-shaped colored dots), toggleable, sitting on top
      of the existing Phase 10.1 token table rather than replacing it.
- [ ] Windows UI Automation names/roles set on every custom-drawn
      control introduced since Phase 10 (title-bar buttons, themed
      checkboxes, icon buttons) so the app is at minimum
      screen-reader-nameable, even though ImGui's immediate-mode model
      limits full accessibility-tree fidelity.
- [ ] Contrast re-verification (same WCAG AA script used in 10.1/10.8)
      run against both the default and colorblind-safe token sets.

### Phase 21 — v2.2.x — Offline interop with other golf/shader tools

Closes out the professional-integration arc with local, file-based
compatibility for artists who already use other established,
non-networked shader tools alongside µShader — still zero network
dependency, since every adapter here is a local text-format reader or
a local boilerplate template, never an API client.

- [ ] Import adapter for Shader Minifier–style exclude-name lists
      (plain-text, one identifier per line) into the Phase 5
      protected-names field, so a project's existing exclude list does
      not need retyping.
- [ ] Export wrappers: one-click "Copy as Shadertoy `mainImage`",
      "Copy as Bonzomatic-ready source", and "Copy as bare
      `void main()`" variants of the golfed output, each a fixed local
      string template — not a live integration with any of those
      tools, since none is networked and Offline-First Isolation rules
      out anything that would require one running.
- [ ] `.ushaderprofile` (Phase 13) documented, versioned JSON schema
      published in `docs/` so external tooling can generate/consume
      profiles without reverse-engineering the format.
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
