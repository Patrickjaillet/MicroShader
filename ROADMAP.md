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
- [ ] Theme White only
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
  ... Phase 8 → `1.0.x`).
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
   ├─ UI shell       : Dear ImGui (docking), white theme, resizable panels
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

- [ ] `docs/logo.png` created
- [ ] "About" tab: copyright, email, website, repository link, logo
- [ ] Application icon (`assets/icons/app.ico`)
- [ ] Installer icon (`assets/icons/installer.ico`)

### Phase 8 — v1.0.x — Packaging & release

- [ ] Inno Setup script (`installer/ushader.iss`) producing a signed
      installer using the Phase 7 icons and the stamped version
- [ ] Manual compatibility pass on Windows 10 and Windows 11
- [ ] Final `README.md` pass with an up-to-date `docs/screenshot.png`
- [ ] `CHANGELOG.md` 1.0.0 entry
- [ ] Tag `v1.0.0`, push, publish release on
      `Patrickjaillet/MicroShader`

### Phase 9 — Post-1.0 (unscheduled)

Deliberately out of scope until requested explicitly:

- [ ] Video/GIF recording of the viewport
- [ ] Any multi-language support (contradicts the English-only
      convention unless that convention is revisited)
- [ ] Multi-buffer Shadertoy support (buffers A–D, channel wiring,
      shared "Common" code) — explicitly rejected, not just deferred;
      see section 7

---

## 7. Explicitly out of scope for v1.0

- **Multi-buffer Shadertoy rendering** — no buffers A–D, no
  `iChannel` wiring, no shared "Common" pass. µShader golfs and
  previews a single `mainImage` fragment shader only. This is a
  deliberate design rejection, not a "later" item.
- Dark theme / any theme other than white
- Any language other than English in the UI or source
- Any AI-tool attribution anywhere in the repository, commit history,
  or contributors list
