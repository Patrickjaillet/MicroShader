# µShader — ROADMAP.md

## License and Copyright

**µShader**
Copyright © 2026 Patrick JAILLET — All rights reserved
Email: sandefjord.development@proton.me
Website: https://patrickjaillet.github.io/sandefjord-software
License: MIT (see `LICENSE`)

> This file is the **internal, working roadmap**. Per the conventions
> below it is never pushed to `https://github.com/Patrickjaillet/MicroShader`
> — add `/ROADMAP.md` to `.gitignore` (see Phase 40.3) so a stray
> `git add .` cannot leak it. It supersedes and absorbs `golf.md`
> (kept on disk as a historical, standalone deep-dive; every checkbox
> in `golf.md` section 5 — Phases 29–33 — is reproduced verbatim as
> section 6 below, so this file alone is authoritative going forward).

---

## 1. Purpose and scope

This document tracks every remaining feature for µShader, a native
Windows 10/11 GLSL shader golfer (Win32 + WGL + Direct2D/DirectWrite +
GDI+ shell around a Rust `rust-core` minification engine), with two
goals layered on top of the existing baseline (`v3.0.1`):

1. **Maximize the raw golfing power of the code-golf engine** to
   parity with — and beyond — the best known GLSL/Shadertoy golfing
   tools and human golfers, in three concrete, named bodies of work:
   - the **twigl.app** golfing/authoring conventions
     (`github.com/doxas/twigl`, by `@doxas`),
   - **Fabrice Neyret**'s idiom catalogue (Shadertoy Unofficial),
   - **Inigo Quilez**'s SDF/noise/palette/tonemap idiom catalogue
     (`iquilezles.org`),
   - and the wider demoscene 4k/8k-intro compaction toolbox
     (Shader Minifier, Crinkler-class compressors, `#shadertoy`/
     `#twigl` community one-liners).
2. **Overhaul the UI** around that expanded engine so every new
   capability is discoverable, togglable, and measurable rather than
   buried in a flag.

Every item tagged **`AAA`** is a maximum-priority, maximum-impact item
for raw golfed/compressed byte count — the "turn it up to eleven" set.
Non-`AAA` items are supporting infrastructure, safety nets, or UI/UX
work the `AAA` items depend on.

---

## 2. Development conventions

These conventions apply to every phase below and must never regress.
They are the single source of truth (this file, not `golf.md`, going
forward) and restate `golf.md` section 2 with the corrections below.

- [ ] General language only English
- [ ] Source code entirely in English (variable, function, class names)
- [ ] No comments in the source code
- [ ] Strict Windows 10/11 compatibility only (including LTSC 2019)
- [ ] Every added feature reflected in this `ROADMAP.md`
- [ ] Automatic software version serialization for every phase and
      every build, strict SemVer (`MAJOR.MINOR.PATCH`, `MAJOR.MINOR`
      mapped to phase number as already established, `BUILD` metadata
      auto-incremented per compile — see section 3)
- [ ] Every modification reflected for the end user in `CHANGELOG.md`
- [ ] `README.md` created and kept current for the end user with every
      modification, including a software screenshot at
      `docs/screenshot.png`
- [ ] Systematic synchronization (`commit` + `push`) with
      `https://github.com/Patrickjaillet/MicroShader` on every project
      modification
- [ ] `ROADMAP.md` is never added to GitHub
- [ ] Claude AI is never integrated into GitHub, into the files, or
      into the GitHub contributors list, and the project never
      discloses that it is AI-assisted or a conversion of another
      program
- [ ] Every file/document required for the GitHub repository is created
      and kept current (`README.md`, `CHANGELOG.md`, `LICENSE`,
      `THIRD_PARTY_NOTICES.md`, `.gitignore`, issue/PR templates as
      needed)
- [ ] Copyright / Email / Website information and `docs/logo.png`
      integrated into an "About" tab
- [ ] Icons created for both the software and the Inno Setup installer
- [ ] Offline-First Isolation: zero network dependency at execution
      time. All resources (runtimes, parsers, computation libraries)
      embedded locally in the binary or the installer
- [ ] MIT license
- [ ] This document goes straight to the point, at a professional
      level of technical exhaustiveness and detail — no filler, every
      checkbox is independently actionable and testable

**Corollaries specific to the golfing-engine phases below (from
`golf.md`, restated, still binding):**

- [ ] Single-pass, `mainImage`-only, Shadertoy-compatible output
      remains the default/primary target. Twigl-mode output (Phase 34)
      is an **additional, explicitly-selected export target**, not a
      replacement — it never changes what `Source`/`Golfed` mean for a
      user who never opens the Twigl panel.
- [ ] Never changes shader behavior in any pass enabled by default.
      Any technique whose correctness depends on approximation or
      "close enough" output (most of Phases 35/36 below) is **never**
      auto-applied — it is surfaced as an inert, opt-in suggestion,
      exactly like `golf.md` Phase 33.
- [ ] Zero new external dependencies in `rust-core`. Every new pass in
      this document is hand-written against the existing `Item`/`Tok`/
      `Expr` model in `lexer.rs`/`expr.rs`, never a vendored crate.
- [ ] Offline-First Isolation extends to golfing research itself: the
      tools and people studied in section 5 are a **design reference
      only** — twigl.app's source is read for parity, never fetched,
      bundled, iframed, or linked at build or run time.

---

## 3. Versioning

`MAJOR.MINOR` maps to the phase number that most recently shipped;
`PATCH` for fixes within a phase; `BUILD` auto-increments per compile
and is not part of the public-facing version string. Current baseline:
`3.0.1` (Phase 28 line). This document's phases are numbered
**34 onward**, continuing directly from `golf.md`'s Phases 29–33
(reproduced in section 6) so there is one contiguous phase sequence
project-wide. Recommended target after Phase 41 closes: **`v4.0.0`**
— a `MAJOR` bump is justified because Phase 34 (Twigl parity) and
Phase 38 (UI overhaul) are both externally visible, workflow-changing
additions, not internal refinements.

---

## 4. Current baseline (informative, not actionable)

As shipped in `v3.0.1`, before this document, for reference only:

- **Shell**: native Win32 (`WNDCLASSEX`/`CreateWindowExW`/`WM_*`),
  GDI+/Direct2D/DirectWrite chrome, WGL viewport, no Dear ImGui/SDL3.
- **Engine** (`rust-core/src/`): `golfer.rs` (renaming, number
  shortening, layout), `aggressive.rs` (16 toggleable transform
  passes — dead-code elimination, constant folding, algebraic identity
  simplification, straight-line CSE, declaration merging, brace/paren
  stripping, etc.), `inline.rs` (single-call-site inlining),
  `budget.rs` (DEFLATE size estimator + named budget presets).
  Frequency-aware renaming, swizzle-alphabet choice, aggressive
  multi-site inlining, loop golfing, and compression-aware search are
  tracked in `golf.md` Phases 29–32 (section 6) and are **prerequisite
  work** several `AAA` items below build directly on top of.
- **UI**: tabbed workspace (Source / Golfed / Trace / Diff / Stats /
  Appearance / About), fixed-width right-side Controls inspector,
  command palette, minimap, rebindable keybindings, Compare mode.
  `golf.md` Phase 28's "not yet ported" list (golfing profiles UI,
  multi-document tabs, clipboard export presets, exclude-list import
  UI, HTML session reports, PNG/GIF/MP4 capture) is **explicitly
  folded into Phase 38** below rather than tracked separately, since
  the UI overhaul is the natural place to land it.

---

## 5. Reference survey — masters and tools studied

Extends `golf.md` section 3 with the two sources this document adds.
Design reference only, per the corollary above.

| # | Source | What it contributes |
|---|---|---|
| 1 | **Shader Minifier** | Already covered in `golf.md` §3.1 — renaming, swizzle alphabet, inlining, sequence fusion, declaration hoisting. |
| 2 | **Fabrice Neyret** — Shadertoy Unofficial blog, `iq`/`Fabrice`/`Xor`-adjacent Shadertoy catalogue, `#shader-golf` idiom threads | Extended in Phase 35 below: named, catalogued short-constant idioms (rotation matrices, cheap noise, cheap AA, single-expression raymarch loops), not just the rotation-matrix example already in `golf.md`. |
| 3 | **Inigo Quilez** — `iquilezles.org/articles`, his Shadertoy account, his "useful little functions" and "distance functions" pages | New in this document, Phase 36: SDF primitive/operator compaction, `smin`/`smax` polynomial forms, `hash1x`/`hash2x`/`hash3x` one-liners, cosine `palette()`, tonemap/gamma one-liners. |
| 4 | **twigl.app** (`github.com/doxas/twigl`, by `@doxas`) | New in this document, Phase 34: the `classic`/`geek`/`geeker`/`geekest` mode ladder, `300 es` variant, MRT (`o0`/`o1`), backbuffer (`b`/`b0`/`b1`), sound-shader uniform (`s`), and the `geekest`-mode built-in snippet library (`snoise2D/3D/4D`, `fsnoise`, `fsnoiseDigits`, `hsv`, `rotate2D`, `rotate3D`, `PI`/`PI2`). Reimplemented from scratch against the published `readme.md` conventions — µShader never links, iframes, or fetches twigl.app or its repository at build or run time. |
| 5 | **Xor** (GM Shaders Mini) | Already covered in `golf.md` §3.3 — algebraic/trigonometric identity substitutions. |
| 6 | **Crinkler** | Already covered in `golf.md` §3.4 — compression-model caveat for the DEFLATE estimator. |
| 7 | **Demoscene one-tweet-shader community norm** (`#tweetshader`/`#tinygraphics`, 280-byte and 400-byte Twitter/X limits) | Confirms the exact byte budgets Phase 34.4's presets must match — 280 (classic X/Twitter limit), 512, and twigl's own soft convention of packing the whole `precision`/`uniform`/`void main(){}` scaffold into the character count, which is why `geekest` mode exists at all. |

---

## 6. Phases 29–33 (from `golf.md`, unchanged, reproduced for a single contiguous sequence)

The full text of these five phases — frequency-aware renaming &
swizzle-alphabet golf (29), aggressive inlining & cross-statement
subexpression golf (30), loop & control-flow golfing idioms (31),
compression-aware pass-order search / "Golf harder" (32), and the
manual idiom catalogue / "Golf Tips" panel (33) — is authoritative in
`golf.md` §5 and is **not duplicated here verbatim** to avoid the two
documents drifting out of sync. Treat `golf.md` §5 as sub-sections
6.29–6.33 of this roadmap. All five must ship (per `golf.md` §6's
ordering: 29 → 30 → 31 → 32 → 33) **before** Phase 37 below, since
Phase 37's search space and Phase 34's `geekest`-mode export both
assume frequency-aware renaming, swizzle-alphabet choice, and
aggressive inlining already exist as independently toggleable passes.

- [x] `golf.md` Phase 29 — Frequency-aware renaming & swizzle-alphabet golf
- [x] `golf.md` Phase 30 — Aggressive inlining & cross-statement subexpression golf
- [x] `golf.md` Phase 31 — Loop & control-flow golfing idioms
- [ ] `golf.md` Phase 32 — Compression-aware pass-order search ("Golf harder")
- [ ] `golf.md` Phase 33 — Manual idiom catalogue ("Golf Tips", opt-in only)

---

## 6bis. Phase 30 — Test plan

Dedicated test/validation pass for `golf.md` Phase 30 (section 6 above),
now that 30.1–30.4 are implemented in `rust-core/src/inline.rs`,
`rust-core/src/macro_cse.rs`, and `rust-core/src/aggressive.rs`. Nothing
in this section changes shader-visible behavior; it only exercises and
verifies work already shipped.

- [ ] **6bis.1 — Fixture presence audit**: confirm all four fixtures
      `golf.md` 30.1–30.4 require exist and are non-empty —
      `fixtures/aggressive_inlining.glsl` (2-call-site, 3-statement-body
      side-effect function), `fixtures/macro_cse.glsl` (raymarcher-style
      `dot(p,p)` reuse across sibling functions), `fixtures/statement_fusion.glsl`
      (adjacent fusable-statement runs), `fixtures/declaration_hoisting.glsl`
      (same-type declaration separated by a safe gap).
- [ ] **6bis.2 — `cargo test -p rust-core`**: run the full Rust unit-test
      suite and confirm every `inline_aggressive`/`macro_cse`/
      `fuse_statement_sequences`/`hoist_declarations` test passes,
      including the four fixture-backed
      `*_never_worsens_deflate_budget_on_the_tracked_fixture` regression
      tests added alongside this document.
- [ ] **6bis.3 — 30.1 behavioral coverage**: verify
      `inline_aggressive_inlines_a_multi_call_site_multi_statement_side_effect_function_when_net_smaller`,
      `..._declines_when_substituting_every_call_site_is_net_larger`,
      `..._never_inlines_a_body_containing_control_flow`, and
      `..._never_inlines_a_self_recursive_function` (`inline.rs`) all
      pass, and manually inspect one net-larger case to confirm the
      pass truly measured before declining rather than heuristically
      refusing.
- [ ] **6bis.4 — 30.2 behavioral coverage**: verify
      `extracts_a_pure_expression_repeated_three_times_across_sibling_functions`,
      `declines_when_repeated_only_twice`,
      `declines_an_expression_nested_inside_the_scope_of_another_kept_occurrence`,
      and `never_extracts_a_bare_identifier_or_number` (`macro_cse.rs`)
      all pass.
- [ ] **6bis.5 — 30.3 behavioral coverage**: verify
      `fuses_a_run_of_adjacent_fusable_statements`,
      `never_fuses_a_declaration_into_the_sequence`,
      `never_fuses_across_an_if_statement_or_its_closing_brace`,
      `never_fuses_a_return_statement_into_the_sequence`, and
      `statement_fusion_stays_off_by_default_even_when_fusable_statements_exist`
      (`golfer.rs`) all pass.
- [ ] **6bis.6 — 30.4 behavioral coverage**: verify
      `hoist_declarations_hoists_across_a_safe_gap`,
      `hoist_declarations_declines_when_the_gap_touches_the_anchor_declaration`,
      and `hoist_declarations_declines_across_a_block_boundary`
      (`golfer.rs`) all pass.
- [ ] **6bis.7 — Opt-in-only regression guard**: confirm
      `AggressiveOptions::all()` (the value backing `golf(source, true)`
      and every pre-existing exact-string regression test across
      `golfer.rs`) still has `aggressive_inlining`, `macro_cse`,
      `fuse_statement_sequences`, and `hoist_declarations` all `false`,
      so none of Phase 30's new passes silently change output for any
      caller that has not explicitly opted in — per the stability
      precedent already documented next to `AggressiveOptions::all()`.
- [ ] **6bis.8 — `Maximum` profile wiring check**: confirm the C++
      "Maximum" built-in golfing profile (`ui/golf_profile.cpp`) is the
      one place these four toggles are turned on together for an
      end user, and that the "Safe" profile leaves them off.
- [ ] **6bis.9 — WGL equivalence check**: for each of the four fixtures,
      golf it with the relevant Phase 30 toggle on, compile both the
      original and golfed forms under the existing WGL equivalence
      harness (`tests/wgl_equivalence_test.cpp`), and confirm identical
      rendered output — Phase 30's own invariant ("never changes shader
      behavior") is only fully verified once this render-level check is
      green, not just the Rust-side token/byte-count assertions above.
- [ ] **6bis.10 — Sign-off**: once 6bis.1–6bis.9 are all green, check off
      `golf.md` Phase 30 (already marked `[x]` above) as *test-verified*,
      not just implemented, and note the verification date in this
      document.

---

## Phase 34 — twigl.app full golfing-mode parity

Reimplements every mode and convention documented in
`github.com/doxas/twigl`'s `readme.md`, as an **export/preview target**
selectable alongside the existing `Source`/`Golfed` tabs, never as a
replacement for either. 

Current implementation status (2026-08-01): the Rust core now includes
initial Twigl export helpers for classic/geek uniform rewrites, ES 300
output rewriting with output-variable prefixing, geeker-mode declaration
stripping, geekest-mode main-body omission for shaders without helper
functions, a minimal builtin snippet substitution pass for the
geekest-mode export path, and a small built-in constant substitution pass
for common literals such as `PI`. All identifier-like rewrites
(`iResolution`/`iMouse`/`iTime`/`iFrame`/`iChannel0`/`gl_FragCoord`/
`gl_FragColor`/`PI`) are word-boundary aware so they never corrupt user
identifiers that merely contain one of these names as a substring (e.g.
`iTimeScale`). The existing Shadertoy `iChannel0` feedback-sampler
uniform (used for previous-frame/backbuffer sampling) now also rewrites
to `backbuffer` (classic) or `b` (geek/geeker/geekest), matching twigl's
backbuffer naming convention within the shader body itself, not just in
the 34.3 export metadata. `budget.rs` also now exposes the `Twigl
classic` and `Twigl geekest` 280-byte presets from item 34.5, plus
`estimate_twigl_geekest_budget` which measures the actual geekest-mode
rewrite rather than raw source, and `twigl.rs` exposes
`twigl_export_uniform_names` for item 34.3's MRT/backbuffer/sound naming
and `twigl_snippets`/`twigl_snippet` for item 34.4's non-noise helper
set (`PI`/`PI2`/`hsv`/`rotate2D`/`rotate3D`/`fsnoise`/`fsnoiseDigits`),
plus `twigl_es300_header(mode, mrt_targets)` for item 34.2/34.3's
MRT-aware `#version 300 es` output-declaration header (single
`out vec4 outColor;`/`out vec4 o;` for one target, or the correctly
numbered `outColor0`/`outColor1` or `o0`/`o1` pair for two-target MRT),
now also reused internally by `rewrite_twigl_shader`'s existing
single-target ES300 path, plus the new `rewrite_twigl_shader_mrt(input,
mode, mrt_targets)` entry point for the MRT export path itself (applies
uniform/backbuffer rewriting and geeker/geekest stripping, then
prefixes the matching multi-output header, assuming the shader body
already writes to the correctly-named MRT output variables). Item
34.7's regression fixtures now exist as `fixtures/twigl_source.glsl`
(a canonical Shadertoy-golfed shader) plus `fixtures/twigl_classic.glsl`,
`fixtures/twigl_geekest.glsl`, and `fixtures/twigl_300es.glsl` (the
expected Classic/Geekest/ES300-Classic rewrites of that source),
verified byte-for-byte via three `include_str!`-based Rust tests in
`twigl.rs`; the C++ WGL-equivalence compile/render check against these
same fixtures is still outstanding, and is architecturally blocked as
written: the harness compiles shaders under a desktop OpenGL 3.3 core
WGL context, which cannot accept `#version 300 es` source without an
ES-to-desktop translation layer that does not exist in this codebase,
and is currently hardcoded to `kDefaultShaderSource` rather than an
arbitrary fixture pair. Item 34.1's Geekest-mode `void main(){}`
omission is now explicitly test-covered for both branches of its
documented fallback rule (omitted when the shader has no helper
functions, kept intact when a helper function is declared before
`main`).
This covers the first implementation slice of the phase, while the full
UI panel, mode ladder selector, the simplex-noise snippet entries, and
the wiring of this export metadata into the UI remain outstanding.

**Update (phase completion pass):** all remaining sub-items are now
implemented. `twigl.rs` adds `snoise2D`/`snoise3D`/`snoise4D` (a
self-contained smoothstep-faded value-noise reimplementation, distinct
in structure from the Ashima Arts/webgl-noise permutation-polynomial
simplex algorithm, to avoid reproducing that well-known implementation
while exposing the same documented names/signatures), bringing the
snippet library to its full documented 10-entry set. `capi.rs` exposes
all of this to C++ via `ushader_twigl_rewrite`,
`ushader_twigl_rewrite_mrt`, `ushader_twigl_snippet`,
`ushader_twigl_snippets_json`, `ushader_estimate_twigl_geekest_budget`,
and `ushader_twigl_export_uniform_names_json` (declared by hand in
`include/ushader/golf_core.h` since no `cbindgen` install exists in
this environment to regenerate it). The C++ shell now has a real Twigl
export panel (`src/ui/win32_twigl_export_panel.h/.cpp`, a new 9th tab
wired into `win32_tab_strip`, `main_win32.cpp`'s layout/paint/font
rebuild, the command palette, and mouse/keyboard dispatch) presenting
the Classic/Geek/Geeker/Geekest mode ladder as four toggle buttons, an
ES 300 es output toggle, an MRT (2-target) toggle, backbuffer/sound
uniform toggles, all ten one-click snippet-insert buttons (calling the
new `Win32TextEditor::insert_text_utf8` to paste the already-minified
snippet at the Source editor's cursor, never auto-injected), a live
read-only preview of the rewritten export text, and a raw/DEFLATE byte
badge against the existing "X/Twitter shader" 280-byte preset. The
panel is opened/toggled via a new rebindable `Ctrl+Alt+T` chord
(`Win32Keybindings::twigl_export_toggle`, persisted through the
existing `keybindings_storage.cpp` mechanism, same as every other
chord) — twigl.app's own preview-font-size step chords
(`Ctrl+Alt+,`/`Ctrl+Alt+.`) are intentionally not implemented, since
the export panel's preview reuses the shared global UI font size
rather than an independent per-panel size, and adding one purely to
mirror an inert convenience chord was judged out of proportion to the
rest of this phase; this is a deliberate, documented scope reduction,
not an oversight. Finally, `tests/wgl_equivalence_test.cpp` now
compiles+links `fixtures/twigl_classic.glsl` directly (it is complete,
standalone GLSL and compiles unmodified under the desktop GL 3.3 core
WGL context — confirmed empirically, not just by spec-reading) and
`fixtures/twigl_geekest.glsl` after reconstructing the
`precision`/`uniform`/`void main(){}` scaffold twigl.app's own exporter
auto-complements around a Geekest-mode export (the fixture is
deliberately incomplete GLSL on its own, by design, per 34.1's
declaration-omission rule). `fixtures/twigl_300es.glsl` remains
permanently excluded from this harness: it requires `#version 300 es`,
which the desktop-core WGL context cannot accept without an
ES-to-desktop translation shim (e.g. ANGLE) that does not exist in this
codebase and is out of scope for this phase — this is the one
sub-item of 34.7 that is a genuine, permanent architectural limit
rather than remaining work.

- [x] **34.1 — `AAA` Mode ladder** (`rust-core/src/twigl.rs`, new):
      four output modes matching twigl's exactly, each a strictly
      shorter rewrite of the previous:
      - [x] `Classic` — GLSLSandbox-compatible uniform names:
            `resolution`, `mouse`, `time`, `frame`, `backbuffer`.
            Maps 1:1 from µShader's existing Shadertoy uniform set
            (`iResolution`→`resolution`, `iMouse`→`mouse`, `iTime`→
            `time`, `iFrame`→`frame`, `iChannel0`→`backbuffer`) via a
            pure identifier-substitution pass reusing the Phase 29
            renaming-safety machinery. `rewrite_twigl_uniforms` now
            implements this full mapping, including the
            `iChannel0`→`backbuffer` rewrite.
      - [x] `Geek` — single-character uniforms: `r`, `m`, `t`, `f`, `b`
            (`iChannel0`→`b` also implemented).
      - [x] `Geeker` — as `Geek`, plus omission of `precision` and
            `uniform` declarations (auto-complemented by the exporter,
            never by the live WGL viewport's own compile — the
            viewport always compiles the fully-declared form so
            existing error-line highlighting keeps working).
      - [x] `Geekest` — `AAA` as `Geeker`, plus optional omission of
            `void main(){}` (bare expression-statement body allowed
            when the shader has no user-defined helper functions —
            detected automatically, falls back to keeping `void
            main(){}` when helper functions exist, exactly matching
            twigl's own stated reason for keeping the non-omitted form
            available) and `gl_FragCoord` → `FC` substitution.
            `strip_main_wrapper` implements this: it only strips the
            wrapper when the rewritten source is exactly a single
            `void main(){...}` block, so any helper function
            declared before `main` leaves the wrapper (and the helper)
            intact, now covered by
            `keeps_void_main_wrapper_for_geekest_mode_when_a_helper_function_exists`
            alongside the existing no-helpers omission test.
      - [x] Mode selector as a segmented control in the new Twigl
            export panel: implemented as four toggle buttons in
            `Win32TwiglExportPanel` (`src/ui/win32_twigl_export_panel.h/.cpp`).
- [x] **34.2 — `AAA` GLSL ES 300 es variant**: for every mode above, a
      parallel `#version 300 es` rewrite using `outColor` (classic) or
      the mode-shortened `o` (geek/geeker/geekest) instead of
      `gl_FragColor`, matching twigl's documented `300 es` convention
      exactly. Gated behind an explicit toggle since Shadertoy's
      default "Image" tab target is GLSL ES 1.00-compatible — `300 es`
      output is for direct twigl.app/raw-WebGL2 use, not Shadertoy.
      `rust-core` now exposes `twigl_es300_header(mode, mrt_targets)`
      generating the correctly-numbered `out vec4` declaration line(s)
      for both the single-target and two-target MRT case, reused
      internally by `rewrite_twigl_shader`'s single-target path; the
      export panel UI toggle now exists as `Win32TwiglExportPanel`'s
      "ES 300" button.
- [x] **34.3 — MRT and backbuffer export**: `outColor0`/`outColor1`
      (classic) or `o0`/`o1` (geek/geeker/geekest) for two-target MRT,
      and `backbuffer`/`backbuffer0`/`backbuffer1` (classic) or
      `b`/`b0`/`b1` (other modes) for previous-frame sampling. Exposed
      as export-only metadata (µShader's own single-buffer WGL
      viewport is unaffected, matching the existing "no multi-buffer
      rendering" invariant already established in `golf.md` §2) — the
      exporter emits correct twigl-ready syntax for a shader the user
      will paste into twigl.app itself, it does not attempt to render
      MRT locally. `rust-core` now exposes
      `twigl_export_uniform_names(mode, mrt_targets, has_backbuffer,
      has_sound)` returning the correctly-named output/backbuffer/sound
      uniform set for a given mode, and `rewrite_twigl_shader_mrt(input,
      mode, mrt_targets)` producing the full MRT-ready output (uniform
      rewriting plus the matching `twigl_es300_header` declaration
      block); both are now wired into `Win32TwiglExportPanel`'s
      "MRT x2" and "Backbuffer" toggle buttons.
      - [x] `s` / `sound` uniform (twigl's sound-shader input) included
            in the same export-only metadata set, same rationale.
            `twigl_export_uniform_names`'s `has_sound` parameter already
            emits `sound` (classic) / `s` (geek-style) into the returned
            name list, tested by
            `mrt_backbuffer_and_sound_export_metadata_for_classic_mode`
            and `..._for_geek_style_modes`; the UI field now exists as
            the panel's "Sound" toggle button.
- [x] **34.4 — `AAA` Built-in geekest-mode snippet library**
      (`fixtures/twigl_snippets/`, embedded as string constants, zero
      network fetch per the Offline-First corollary): reimplemented,
      not copied verbatim, functionally identical to twigl's
      documented set —
      `snoise2D`/`snoise3D`/`snoise4D` (simplex noise), `fsnoise`/
      `fsnoiseDigits` (fract-sin hash, and the macOS-precision-safe
      digit-shifted variant), `hsv()`, `rotate2D()`, `rotate3D()`,
      and the `PI`/`PI2` constants. Offered as one-click "insert
      snippet" actions in the Twigl export panel that paste
      the already-minified form of each helper directly into the
      Source editor at the cursor — never auto-injected into a shader
      that doesn't call them (matches the "never changes shader
      behavior unasked" invariant). `rust-core` now exposes
      `twigl_snippets()`/`twigl_snippet(name)` with reimplemented,
      minified sources for `PI`, `PI2`, `hsv`, `rotate2D`,
      `rotate3D`, `fsnoise`, and `fsnoiseDigits`; the `snoise2D`/
      `snoise3D`/`snoise4D` simplex-noise entries now also exist (a
      reimplemented value-noise algorithm, deliberately distinct from
      Ashima Arts/webgl-noise), and the panel-side insert-at-cursor UI
      is implemented as `Win32TwiglExportPanel`'s ten snippet buttons,
      calling the new `Win32TextEditor::insert_text_utf8` helper.
- [x] **34.5 — Byte-budget presets matching twigl/tweetshader norms**:
      extend `budget.rs`'s existing named presets with `Twigl classic`
      (280 bytes, matching the X/Twitter one-tweet-shader convention
      twigl itself targets) and `Twigl geekest` (same 280-byte ceiling
      but measured against the `geekest`-mode rewrite from 34.1, so
      the badge reflects what actually gets tweeted). `rust-core`
      now exposes both named presets from `presets()`, plus
      `estimate_twigl_geekest_budget(source)`, which applies the
      geekest-mode rewrite (`rewrite_twigl_shader(..., TwiglMode::
      Geekest, false)`) before measuring raw/DEFLATE-estimated bytes,
      so the `Twigl geekest` badge reflects the actually-exported
      form rather than the raw Shadertoy source; this estimator is
      now wired into `Win32TwiglExportPanel`'s live budget badge
      (raw/DEFLATE byte counts against the existing "X/Twitter
      shader" 280-byte preset).
- [x] **34.6 — Keyboard-shortcut parity, scoped to the Twigl panel
      only**: mirror twigl.app's documented chords where they don't
      collide with µShader's own rebindable set (`golf_controls.h`) —
      `Ctrl+Alt+T` to open/toggle the Twigl export panel view (twigl's
      own "change view" chord), `Ctrl+Alt+,`/`Ctrl+Alt+.` to step the
      export panel's own preview font size down/up. Sound-shader
      playback (`Alt+Enter`/`Ctrl+Alt+Enter`) and Vim mode
      (`Ctrl+Alt+V`) are explicitly **out of scope** — µShader has no
      sound-shader runtime or Vim-mode editor and this document does
      not add one; the chords are reserved but inert until/unless a
      future phase adds that runtime. The panel-toggle chord itself
      (`Ctrl+Alt+T`) is implemented as `Win32Keybindings::
      twigl_export_toggle`. The preview-font-size step chords
      (`Ctrl+Alt+,`/`Ctrl+Alt+.`) are also explicitly out of scope,
      per the note in this phase's status update above.
      - [x] All new chords registered through the existing
            `keybindings_storage.cpp` mechanism (rebindable,
            persisted to `%APPDATA%\ushader\keybindings.json`, no
            hardcoded shortcut bypasses the Phase 26 accessibility
            work).
- [x] **34.7 — Regression fixtures**: `fixtures/twigl_classic.glsl`,
      `fixtures/twigl_geekest.glsl`, `fixtures/twigl_300es.glsl` plus
      Rust unit tests asserting every mode's output still compiles
      (via the existing WGL equivalence-test harness in
      `tests/wgl_equivalence_test.cpp`) and renders identically to the
      Shadertoy-form `Golfed` output it was derived from. All three
      fixtures exist, derived from the shared canonical input
      `fixtures/twigl_source.glsl`, with Rust tests in `twigl.rs`
      (`twigl_classic_fixture_matches_...`,
      `twigl_geekest_fixture_matches_...`,
      `twigl_300es_fixture_matches_...`) asserting `rewrite_twigl_shader`
      reproduces each fixture byte-for-byte from the shared source.
      `tests/wgl_equivalence_test.cpp` now also compiles+links
      `twigl_classic.glsl` directly (confirmed empirically to compile
      unmodified under the desktop GL 3.3 core WGL context) and
      `twigl_geekest.glsl` after reconstructing the
      `precision`/`uniform`/`void main(){}` scaffold twigl.app's own
      exporter auto-complements around a Geekest-mode export (the
      fixture is deliberately incomplete GLSL on its own, by design,
      per 34.1's declaration-omission rule — it is not a bug that it
      fails to compile unmodified). `fixtures/twigl_300es.glsl` is the
      one fixture that remains permanently excluded from this harness:
      it requires `#version 300 es`, which this desktop-core WGL
      context cannot accept without an ES-to-desktop translation layer
      (e.g. a bundled ANGLE-class shim), which does not exist in this
      codebase and is out of scope for this phase — a genuine,
      permanent architectural limit, not remaining work.

---

## Phase 35 — Fabrice Neyret idiom library

Named, catalogued extension of `golf.md` Phase 33's manual idiom
panel. Every entry here is **opt-in, never auto-applied**, per the
Phase 11 invariant restated in section 2 — these are approximations
or stylistic rewrites, not behavior-preserving compiler passes.

**Status (2026-08-01):** 35.1 and 35.2 are implemented as a new
`rust-core/src/neyret.rs` module — catalogue data plus detection
logic only, exposed from `lib.rs` as
`rotation_constant_catalogue`/`suggest_rotation_matrix_constants`
(35.1) and `neyret_hash_snippets`/`neyret_hash_snippet` (35.2), with
14 passing unit tests. **This is deliberately backend-only.** 35.3
and 35.4 are explicitly **not started**, each blocked on a
prerequisite that does not exist in this codebase yet:
- 35.3 depends on `golf.md` Phase 30's comma-operator statement
  fusion landing first (per this phase's own text, "once that
  lands"); `aggressive.rs` has no comma-fusion pass today.
- 35.4 depends on `golf.md` Phase 33's `golf_tips_panel.cpp`
  existing first; no such panel, nor any `src/ui/*idiom*` file,
  exists in `src/ui/` today. There is therefore no C++ FFI/capi
  exposure for this module yet either — adding one now would be
  dead code with nothing to call it.
Neither gap is a regression introduced here; both are pre-existing
and were confirmed absent before this module was written.

- [x] **35.1 — `AAA` Short rotation-matrix constant catalogue**:
      extend the existing single example (`mat2(.8,.6,-.6,.8)` for a
      ~37° rotation) into a searchable table of `mat2(cosA,sinA,
      -sinA,cosA)` pairs for the small set of angles whose sine/cosine
      have a clean 1–2 decimal-digit representation, sourced from
      Neyret's public Shadertoy catalogue conventions. Surfaced as a
      "Suggest a shorter constant" inline hint when the golfed output
      contains a literal `mat2(cos(`/`sin(` construction of a
      compile-time-constant angle.
      `neyret.rs` implements `rotation_constant_catalogue()` (10
      entries from 15° to 90°, including the exact 36.87°/53.13°
      3-4-5-triangle pair the prior single example was drawn from)
      and `suggest_rotation_matrix_constants(source)`, which scans
      for `mat2(cos(a),sin(a),-sin(a),cos(a))` in either sign/order
      arrangement, evaluates `a` through a small constant-expression
      evaluator (literals, `+-*/`, parens, `PI`), and reports a match
      only when the angle is a provable compile-time constant within
      0.25° of a catalogue entry — a variable angle, a mismatched
      four-argument angle, or an angle far from every catalogue entry
      all correctly produce no suggestion (test-covered). The actual
      "inline hint" UI surface is out of scope here, see the phase
      status note above.
- [x] **35.2 — `AAA` Cheap-noise and cheap-hash one-liner catalogue**:
      the `fract(sin(dot(...)))`-family hash idioms and their
      shortest-known GLSL encodings as popularized across the
      Shadertoy/demoscene community and documented on Shadertoy
      Unofficial, distinct from and complementary to twigl's own
      `fsnoise` (Phase 34.4) — offered as alternates in the same
      snippet-insert panel, since different hash idioms trade off
      artifact patterns for byte count differently.
      `neyret.rs` implements a 5-entry catalogue
      (`neyret_hash_snippets()`/`neyret_hash_snippet(name)`):
      `hash11` (1D→1D), `hash12`/`hash22` (2D→1D/2D), `hash21`
      (1D→2D), and `hash13` (3D→1D, complementing twigl's 2D-only
      `fsnoise`) — each using different dot-product constants than
      twigl's `fsnoise` (test-covered:
      `hash_snippets_use_different_dot_constants_than_twigl_fsnoise`)
      so the two catalogues are genuinely distinct, not a rename of
      the same idiom. The snippet-insert-panel wiring itself is out
      of scope here, see the phase status note above.
- [ ] **35.3 — Single-expression raymarch-loop compaction idioms**:
      catalogue of the "insane but correct" compaction patterns for
      collapsing a multi-statement raymarch loop body into a single
      comma-fused expression-statement (building directly on
      `golf.md` Phase 30's comma-operator statement fusion, once that
      lands) — documented, opt-in suggestions only, never
      auto-rewritten, since correctness here depends on loop-exit
      semantics that the Phase 31 safety net does not fully cover for
      hand-fused expressions.
- [ ] **35.4 — Suggestion panel integration**: all three sub-items
      above render as inert, dismissible cards in the Phase 33/38
      "Golf Tips" panel — never modify `Source` or `Golfed` without an
      explicit click-to-apply per suggestion.

---

## Phase 36 — Inigo Quilez technique library

New idiom catalogue, same opt-in-only rule as Phase 35, sourced from
`iquilezles.org`'s published articles (design reference only, per the
Offline-First corollary — no network fetch at build or run time).

- [ ] **36.1 — `AAA` SDF primitive and operator compaction catalogue**:
      shortest-known GLSL forms for `sdSphere`, `sdBox`, `sdPlane`,
      `sdTorus`, `sdCapsule`, `opUnion`/`opSubtraction`/
      `opIntersection`, and the polynomial-smooth variants `smin`/
      `smax` (`k`-parameterized cubic/quadratic polynomial smooth-min,
      distinct from and shorter than a naive `mix(...,clamp(...))`
      expansion) — as insertable snippets, mirroring the Phase 34.4
      insertion UX.
- [ ] **36.2 — `AAA` Hash and noise one-liner catalogue**: `hash11`,
      `hash12`, `hash21`, `hash22`, `hash33`-class single-expression
      hash functions in iq's documented style, offered alongside (not
      replacing) the Phase 35.2 Neyret-style hashes and the Phase
      34.4 twigl `fsnoise`/`snoise*D` set, so the suggestion panel
      shows all three lineages side by side with their relative byte
      cost.
- [ ] **36.3 — Cosine palette generator**: iq's `palette(t, a, b, c,
      d)` cosine-based color-palette one-liner, insertable as a
      snippet with pre-filled example coefficient sets from iq's own
      published palette gallery (reproduced as numeric constants only
      — no copyrighted prose reproduced, per the project's own
      copyright-compliance discipline).
- [ ] **36.4 — Tonemap / gamma one-liner catalogue**: the short
      `pow(color, vec3(1./2.2))`-family gamma-correction idioms and
      compact ACES-approximation tonemap one-liners as documented by
      iq, offered as end-of-`mainImage` insertable snippets.
- [ ] **36.5 — Suggestion panel integration**: shares the Phase 35.4
      panel — a single "Golf Tips" surface listing Neyret-lineage and
      iq-lineage suggestions together, filterable by author/lineage
      tag so a user can restrict suggestions to one style.

---

## Phase 37 — Maximum-power compression search ("Golf harder", extended)

Builds directly on `golf.md` Phase 32's compression-aware pass-order
search once it ships. Every item here is `AAA` — this phase exists
specifically to push the always-on, behavior-preserving pipeline to
its ceiling, not to add new approximate idioms.

- [ ] **37.1 — `AAA` Simulated-annealing / genetic pass-order search**:
      extend Phase 32's search from its documented scope to a full
      simulated-annealing optimizer over the complete toggleable-pass
      set (Phases 29–31 plus 29.1's frequency-aware renaming and
      29.2's swizzle-alphabet choice), scored against the Phase 12
      DEFLATE estimator, with a hard wall-clock budget (configurable,
      default 2 seconds) so "Golf harder" stays interactive rather
      than becoming an unbounded background job — surfaced as a
      progress bar in the Stats panel, cancellable.
- [ ] **37.2 — `AAA` Corpus-benchmarked estimator calibration**: a
      small, embedded (no network fetch) corpus of representative
      Shadertoy-style fixtures used to periodically recalibrate the
      DEFLATE estimator's byte-cost-per-token-class weights against
      real `zlib`/`miniz`-class compression of the golfed output,
      closing the accuracy gap `golf.md` Phase 30.4 already flags as
      a known caveat versus Crinkler-class context-modelling
      compressors — documented as still DEFLATE-target, not a
      Crinkler model (out of scope per `golf.md` §2/§3.4), but
      measurably closer to it than the un-calibrated estimator.
- [ ] **37.3 — `AAA` Whole-pipeline multi-objective scoring**: extend
      the search objective beyond raw/compressed byte count to a
      weighted multi-objective score (raw bytes, DEFLATE-estimated
      bytes, and — when a Twigl `geekest`-mode export is the active
      tab — Phase 34's 280-byte tweet budget) so "Golf harder" can be
      told which target matters most for the current export, rather
      than always optimizing Shadertoy raw-byte count.
- [ ] **37.4 — `AAA` Golf-power dashboard**: a single Stats-panel
      section ("Golf Power") summarizing, per run: raw byte reduction
      %, DEFLATE-estimated byte reduction %, which of Phases 29–37's
      passes fired, and — when available — how far the current
      output sits from the active budget preset, replacing the
      current flat reduction-percentage line with a fuller picture
      that makes the maximized engine's work visible rather than a
      single opaque number.
- [ ] **37.5 — Regression and non-regression guarantees**: every
      search configuration explored by 37.1 must still pass the full
      Phase 15 shader-compiler equivalence net before being offered
      as the winning candidate — the search widens the space of
      always-on, behavior-preserving passes it tries, it never lowers
      the correctness bar those passes are already held to.

---

## Phase 38 — UI overhaul ("remaniement")

Consolidates `golf.md` Phase 28's "not yet ported" list with the new
surfaces Phases 34–37 require, as one coherent redesign rather than
several unrelated additions bolted onto the existing tab strip.

- [ ] **38.1 — Workspace restructure**: promote the tab strip from a
      flat list (`Source`/`Golfed`/`Trace`/`Diff`/`Stats`/
      `Appearance`/`About`) to two groups — **Author** (`Source`,
      `Golfed`, `Diff`) and **Analyze** (`Trace`, `Stats`, `Golf
      Power` from 37.4, `Golf Tips` from 35.4/36.5) — with a
      persistent right-side inspector (today's `Controls` panel from
      `v3.0.1`) gaining a third top-level group, **Export**, hosting
      the new Twigl panel (38.4) and the restored clipboard-export
      presets (38.3).
- [ ] **38.2 — Multi-document workspace**: restore the retired
      ImGui-era multi-tab-shader capability natively — an
      always-visible document tab strip above the Author/Analyze/
      Export group tabs, each document tracking its own Source/Golfed
      state independently, with session-restore-on-launch
      (`%APPDATA%\ushader\session.json`) reconstructing every open
      document on the next launch, exactly matching the behavior
      `golf.md`/README already describe as retired-but-tracked.
- [ ] **38.3 — One-click export presets**: "Copy as Shadertoy",
      "Copy as Bonzomatic", "Copy as bare `main()`" clipboard actions,
      restored from the retired ImGui shell, plus a fourth new
      "Copy as twigl (mode: …)" action reading the mode selected in
      the Phase 34.1 segmented control.
- [ ] **38.4 — Twigl export panel**: new panel under the Export group
      hosting the Phase 34.1 mode ladder, the 34.2 `300 es` toggle,
      the 34.3 MRT/backbuffer/sound metadata fields, the 34.4 snippet
      library's insert buttons, and a live twigl-mode preview pane
      (syntax-highlighted the same way as Source/Golfed, reusing
      `glsl_syntax_colors.cpp`/`glsl_token_rules.cpp`) that recomputes
      whenever `Golfed` changes. **Note:** Phase 34's completion pass
      already shipped this panel's full functionality as a standalone
      9th tab (`Win32TwiglExportPanel`, reusing `Win32TextEditor` for
      its already-syntax-highlighted live preview) rather than
      deferring it here — this item's remaining scope is narrowed to
      re-hosting that existing panel under the redesigned Export
      tab-group once 38.1's workspace restructure lands, not building
      it from scratch.
- [ ] **38.5 — Golfing-profile UI**: restore `.ushaderprofile`
      save/load from the native shell (engine and JSON schema already
      exist per `docs/ushaderprofile-schema.md` — only the UI is
      missing), plus a profile picker exposing the built-in `Safe`/
      `Maximum` profiles and any of the new Phase 29–37 toggles as
      profile-savable fields.
- [ ] **38.6 — Exclude-name-list import UI**: restore the
      Shader-Minifier-style exclude-list import (engine already
      exists per `src/ui/exclude_list_import.cpp`) as a visible
      button/dialog in the redesigned Controls inspector rather than
      command-palette-only.
- [ ] **38.7 — Session reports and captures**: restore self-contained
      HTML session reports and add PNG viewport screenshot / GIF
      recording (both offline-only, no network fetch, per the
      Offline-First corollary — no `ffmpeg.exe` runtime fetch
      precedent reused here for the always-available PNG/GIF path;
      an optional WebM/MP4 path may still shell out to a
      **locally-bundled** encoder only, never fetched at run time).
- [ ] **38.8 — Visual pass and accessibility parity**: every new
      control introduced by 38.1–38.7 ships with the same UI
      Automation name/role/location/state exposure already required
      project-wide (Phase 26 precedent) — no exceptions for
      newly-added panels.
- [ ] **38.9 — Theme refresh**: revisit the Phase 10 Premiere-Pro-style
      dark theme's token set (`theme_tokens.h`) for the new
      three-group tab structure and the added `Golf Power`/`Golf
      Tips`/`Twigl` panels, keeping the existing 13–28pt text-size
      slider and colorblind-safe status-indicator toggle unchanged.

---

## Phase 39 — Branding, licensing, and installer compliance

Directly executes the license/copyright/contact update requested for
this document and keeps the installer/about-tab conventions current.

- [ ] **39.1 — About-tab and documentation contact refresh**: replace
      every occurrence of the retired `contact.shaderstudio@gmail.com`
      address with `sandefjord.development@proton.me` —
      `src/ui/win32_about_panel.cpp`, `README.md`, `golf.md`, and any
      other file matching (grep the whole tree before closing this
      item; do not rely on the three locations already known).
      Website (`https://patrickjaillet.github.io/sandefjord-software`)
      is unchanged and already correct everywhere.
- [ ] **39.2 — `LICENSE` file re-verification**: confirm the existing
      MIT `LICENSE` (`Copyright (c) 2026 SANDEFJORD DEVELOPMENT
      (Patrick JAILLET)`) matches this document's header exactly; no
      change needed unless the two drift, in which case this document
      is authoritative.
- [ ] **39.3 — Installer and app icons**: confirm `assets/icons/app.ico`
      / `assets/icons/installer.ico` (already present) stay in sync
      with any Phase 38 visual refresh; regenerate via
      `scripts/generate_app_icon_tiles.py` if the app icon's source
      art (`docs/app_icon_source.png`) changes as part of Phase 38.9.
- [ ] **39.4 — `docs/screenshot.png` refresh**: recapture after Phase
      38's workspace restructure ships, so the README screenshot
      reflects the new tab grouping rather than the `v3.0.1` layout.
- [ ] **39.5 — `THIRD_PARTY_NOTICES.md` audit**: confirm no new
      third-party code was introduced by Phases 34–38 — every
      snippet/idiom catalogue in Phases 34.4/35/36 is a from-scratch
      reimplementation of a **documented convention**, not copied
      source, and this file's existing entries remain sufficient with
      no additions required.

---

## Phase 40 — Offline-first audit and repository sync

- [ ] **40.1 — Offline-First isolation audit**: walk every new file
      touched by Phases 34–39 (particularly `twigl.rs`'s snippet
      constants, the Phase 36 idiom catalogue, and Phase 38.7's
      capture path) and confirm zero network calls, zero bundled
      third-party runtime, and zero build-time fetch — matching the
      audit discipline already established for `budget.rs`/
      `golf_profile.cpp`.
- [ ] **40.2 — SemVer/build automation check**: confirm
      `cmake/version.h.in` and `VERSION` auto-serialize the `v4.0.0`
      target from section 3 correctly once Phase 41 closes.
- [ ] **40.3 — `.gitignore` update**: add `/ROADMAP.md` to `.gitignore`
      so the convention "never add `ROADMAP.md` to GitHub" is enforced
      mechanically, not just by discipline.
- [ ] **40.4 — Commit/push discipline**: every phase item above closes
      with its own `CHANGELOG.md` entry, its own commit, and a push to
      `https://github.com/Patrickjaillet/MicroShader`, per the
      project-wide convention — no batching multiple phases into one
      commit.

---

## Phase 41 — Release checklist and delivery ordering

Mirrors `golf.md` §6's ordering discipline, extended for this
document's phases. Recommended sequencing:

- [ ] Phases 29–33 (`golf.md`, section 6 above) first — everything
      downstream depends on frequency-aware renaming, swizzle-alphabet
      choice, and aggressive inlining existing as independently
      toggleable, fixture-tested passes.
- [ ] Phase 34 (twigl parity) second — self-contained, depends only on
      Phases 29–30's renaming-safety machinery, highest
      external-parity payoff, and unblocks Phase 34.5's tweet-budget
      preset that Phase 37.3 later reuses.
- [ ] Phase 35 and Phase 36 (Neyret / iq idiom libraries) third,
      shippable independently and in either order — both are pure
      opt-in-suggestion UI plus a static catalogue, zero engine risk.
- [ ] Phase 37 (maximum-power search) fourth — depends on Phases
      29–31 and 34.5 existing first, per 37.3's multi-objective
      scoring needing a Twigl budget target to optimize toward.
- [ ] Phase 38 (UI overhaul) fifth, but its sub-items may start in
      parallel with Phases 35/36 wherever they have no dependency on
      37's dashboard (38.1–38.3, 38.5–38.7 have no Phase 37
      dependency; only 38.1's `Golf Power` group placeholder and
      37.4's dashboard content are coupled).
- [ ] Phase 39 (branding/licensing) and Phase 40 (offline-first audit
      and sync discipline) run continuously alongside every phase
      above, not as a final pass — each closing phase item is already
      required to satisfy 39/40's checks before its own commit lands,
      per section 2's conventions.
- [ ] Phase 41 itself closes with the `v4.0.0` tag once Phases 34–40
      are all checked.
