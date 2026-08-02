# µShader — golf.md — Ultimate Golfing Engine Roadmap

**µShader**
Copyright © 2026 Patrick JAILLET — All rights reserved
Email: contact.shaderstudio@gmail.com
Website: https://patrickjaillet.github.io/sandefjord-software
License: MIT (see LICENSE)

This document is a **standalone extension** of `ROADMAP.md`, scoped
entirely to the `rust-core` golfing engine and its UI surfaces
(`src/ui/golf_controls.*`, `golf_profile.*`, `golf_trace.*`,
`stats_panel`/`win32_stats_panel`, `budget_presets.*`). It picks up
where `ROADMAP.md` Phase 11 ("Post-1.2 — Maximum golfing power"),
Phase 12 (compression-aware budgets) and Phase 14 (pass trace) left
off, and is meant to be merged back into `ROADMAP.md` as Phases
29+ once approved. Every rule in `ROADMAP.md` section 2 still applies
in full — reproduced here verbatim so this file is self-contained:

## 2. Development conventions

These conventions apply to every phase below and must never regress.

- [ ] General language only English
- [x] Visual theme: Adobe Premiere Pro–style dark UI (see Phase 10).
      This supersedes the original "white theme only" decision below;
      the white theme is kept as historical record only.
- [x] ~~Theme White only~~ — superseded by Phase 10 (v1.2.x)
- [x] ~~UI framework: WinUI 3 / Fluent Design~~ — rejected outright,
      never implemented; Fluent's Mica/Acrylic materials and the
      Windows App SDK runtime itself are unreliable or unsupported on
      Windows 10 LTSC 2019 (Enterprise LTSC has no Store and no
      guaranteed in-box Windows App Runtime), which conflicts with the
      "Strict Windows 10/11 compatibility" and "Offline-First
      Isolation" conventions below. Superseded by the Win32-native
      plan immediately below.
- [ ] UI framework: native Win32 windowing (`WNDCLASSEX` /
      `CreateWindowExW` / standard `WM_*` message pump) with GDI+,
      Direct2D and DirectWrite for all chrome rendering (see Phase 22).
      This supersedes the Dear ImGui/SDL3 shell described below and in
      Phase 3/10; Dear ImGui, SDL3's window/input layer, and the
      bundled ImGui text-editor widget are retired once Phase 27
      closes. SDL3's role is reduced to nothing (removed entirely) —
      window, input, and swapchain ownership move to a raw Win32
      `HWND`. OpenGL rendering itself is kept, hosted through a native
      `WGL` context on a child `HWND` viewport (see Phase 23) — no
      ANGLE, no Direct3D translation layer, since WGL is a first-class,
      in-box API on every Windows 10/11 edition including LTSC 2019 —
      so `rust-core/`, `render/shader_runner.cpp`'s GL call sites, and
      the Shadertoy uniform contract are not rewritten. GDI+, Direct2D,
      DirectWrite and WGL are all in-box Windows components (no
      redistributable runtime to bundle), which strengthens rather
      than weakens the "Offline-First Isolation" convention below.
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

**golf.md-specific corollaries of the conventions above** (do not
weaken any convention, only make its consequence explicit for this
document's scope):

- [ ] **Single-pass, `mainImage`-only, Shadertoy-compatible output
      remains absolute.** `ROADMAP.md` section 7 already rules out
      multi-buffer/`iChannel` rendering as out of scope; every pass
      added by this document must produce GLSL that still compiles
      and runs unmodified as a single Shadertoy "Image" tab. No pass
      may emit Shadertoy-incompatible syntax (no HLSL-only constructs,
      no desktop-GLSL-only extensions beyond what `default_shader.h`'s
      existing uniform contract already assumes).
- [ ] **Never changes shader behavior — the Phase 11 invariant is the
      one rule every phase below is subordinate to.** Any technique
      whose correctness depends on approximation, numeric tolerance,
      or "close enough" visual output (see Phase 33) is **never**
      auto-applied; it is surfaced as an inert, opt-in suggestion only.
- [ ] **Zero new external dependencies in `rust-core`.** Matches the
      precedent already set by `budget.rs`'s hand-written DEFLATE
      estimator (Phase 12) and `golf_profile.cpp`'s hand-written JSON
      (Phase 13): every new pass in this document is hand-written
      against the existing `Item`/`Tok`/`Expr` model in `lexer.rs`/
      `expr.rs`, never a vendored crate.
- [ ] **Offline-First Isolation extends to golfing research itself**:
      the "top golfing tools" studied in section 3 below are a
      **design reference only** — none of them (nor Mono/.NET, nor any
      npm package) is fetched, bundled, shelled out to, or linked at
      build or run time. Any comparison against them (Phase 33.4) is a
      manual, offline, dev-machine-only benchmarking exercise, never a
      shipped feature or CI gate that fetches a third party binary at
      build time — this differs from Phase 9's `ffmpeg.exe` fetch
      precedent deliberately, because a code-golf **competitor**
      dependency has no reason to ever touch the shipped product.
- [ ] Every new pass below ships with: its own `AggressiveOptions`
      toggle, its own `fixtures/*.glsl` regression fixture, Rust unit
      tests in `golfer.rs`/`aggressive.rs`, a `Phase 14` trace step
      name, a `CHANGELOG.md` entry, and a `ROADMAP.md`/`golf.md`
      checkbox flip from `[ ]` to `[x]` — exactly the delivery bar
      Phase 11 already set for `simplify_algebraic_identities` and
      `eliminate_common_subexpressions`.

---

## 3. Reference survey — top Shadertoy-compatible GLSL golfing tools & sources

Studied for technique parity before implementation. Design reference
only, per the corollary above — nothing here is bundled, shelled out
to, or network-fetched by µShader.

| # | Tool / source | What it contributes to this roadmap |
|---|---|---|
| 1 | **Shader Minifier** (`laurentlb/shader-minifier`, GLSL+HLSL, demoscene-standard since 2010) | The reference implementation for almost every pass below: letter/bigram-frequency-driven renaming, `--field-names rgba\|xyzw\|stpq` swizzle-alphabet choice, `--preserve-externals`/`--preserve-all-globals`, `--no-inlining`/`--aggressive-inlining`, `--no-sequence` (comma-operator statement fusion), `--move-declarations`, `--no-remove-unused`, `--smoothstep` macro substitution, `--export-kkp-symbol-maps` (compression/Crinkler analytics). Phases 29–32 below each map to one or more of these flags, reimplemented from scratch in Rust against `rust-core`'s own `Item`/`Expr` model — never linked or shelled out to. |
| 2 | **Fabrice Neyret's "Shadertoy Unofficial" blog** (`shadertoyunofficial.wordpress.com`) and his Shadertoy catalogue | Primary source for the **manual idiom catalogue** in Phase 33: short rotation-matrix constants (e.g. `mat2(.8,.6,-.6,.8)` as a ~37° rotation instead of `mat2(cos(t),sin(t),-sin(t),cos(t))`), 2–3 digit trigonometric/geometric approximations, and general "insane but correct" compaction idioms widely reused across the demoscene/Shadertoy community. |
| 3 | **Xor's "Mini: Code Golfing"** (GM Shaders Mini) | Catalogue of algebraic/trigonometric **identity substitutions** (e.g. rewriting one built-in call as a cheaper equivalent chain) used to seed Phase 33's suggestion library and to sanity-check that Phase 29.1's algebraic-identity pass is not missing obvious, always-safe rewrites. |
| 4 | **Crinkler** (PE compressor used for 4k/64k demoscene intros) | Not GLSL-specific, but the reason Phase 12's DEFLATE estimator exists at all: Crinkler's context-modelling compressor tends to reward different renaming/ordering choices than plain DEFLATE. Referenced in Phase 30.4 as a documented, non-blocking accuracy caveat of the existing estimator — no Crinkler-class model is implemented in this roadmap (out of scope, see Phase 33.5). |
| 5 | **Generic JS/GLSL minifiers** (`glsl-minifier`-class npm tools) | Confirm which passes (whitespace/comment stripping, basic dead-code elimination) are already table-stakes and fully covered by µShader today (Phase 5/11), so this document does not re-litigate them and instead focuses on the golf-competition-grade passes that separate a generic minifier from a demoscene/Shadertoy-golf-grade one. |

---

## 4. Current engine baseline (as of `v3.0.1`, before this document)

Enumerated precisely so every phase below states an explicit delta,
not a vague "improve X". Current passes, all in `rust-core/src/`:

**`aggressive.rs`** — `eliminate_dead_locals`, `eliminate_dead_stores`,
`fold_constants`/`fold_additive_constants`/`fold_float_constants`/
`fold_additive_float_constants`, `simplify_algebraic_identities`,
`eliminate_common_subexpressions` (straight-line, whole-declaration-
initializer only), `reduce_constant_vectors` (constant-only, e.g.
`vec3(1.,1.,1.)` → `vec3(1.)`), `strip_trailing_void_return`,
`compound_assignments`, `increment_decrement`, `ternary_from_if_else`,
`merge_declarations`, `strip_redundant_braces`,
`strip_redundant_parens`, `eliminate_dead_functions`,
`strip_duplicate_precision`.

**`inline.rs`** — `inline_single_call_functions`: single-call-site,
single-`return`-expression bodies only, guarded by `is_safe_arg`.

**`golfer.rs`** — `shorten_number` (leading-zero/trailing-zero/
scientific-notation shortening), `find_renamable` (identifier renaming,
current ordering is scope-size/occurrence-count driven, **not**
letter/bigram-frequency driven — see Phase 29.1), whitespace/layout
compaction in `layout()`, ambiguous-token-pair spacing via
`forms_ambiguous_pair`.

**`budget.rs`** (Phase 12) — hand-written DEFLATE size estimator,
named budget presets (`Shadertoy`, `X/Twitter shader`, `JS13K-style
13KB`, `4KB intro`, `8KB intro`, `64KB intro`).

**Not yet present** (the gap this document closes): frequency-aware
renaming, aggressive/multi-site inlining, cross-statement CSE via
macro extraction, statement-sequence fusion via the comma operator,
declaration hoisting/merging across a whole function, loop-header
golfing idioms, swizzle-alphabet choice, non-constant swizzle/vector
factoring, compression-aware pass-order search, and a documented,
opt-in "manual idiom" library.

---

## 5. Phases

### Phase 29 — Frequency-aware renaming & swizzle-alphabet golf

Closest-to-`Shader Minifier`-parity work: today's `find_renamable` in
`golfer.rs` assigns short names by scope/occurrence count only, which
is character-optimal but not necessarily **compressed-byte-optimal**
against Phase 12's DEFLATE estimator.

- [x] **29.1 — Letter/bigram-frequency-driven identifier assignment**
      (`rust-core/src/golfer.rs`, new `rename_frequency.rs`): after the
      existing scope/collision analysis in `find_renamable` produces
      the *set* of renamable identifiers, replace the current
      allocation order (currently by descending occurrence count only)
      with a two-pass heuristic:
      1. Compute single-character and bigram frequency tables over the
         **golfed-so-far** source (post whitespace/number/dead-code
         passes, pre-rename), matching `Shader Minifier`'s documented
         approach of favoring identifier letters that are already
         common neighbors of surrounding tokens.
      2. Score every candidate 1–2 character name (drawn from the
         existing reserved-word-safe alphabet already used today)
         against `budget.rs`'s DEFLATE estimator on a small trial
         substitution, and assign the highest-frequency-scoring names
         to the highest-occurrence identifiers first, falling back to
         the current occurrence-count-only order when two candidates
         tie exactly (keeps existing fixture outputs byte-identical
         wherever the heuristic cannot possibly change the result, to
         avoid needlessly breaking prior regression fixtures whose
         golfed form must remain deterministic).
      - [x] `AggressiveOptions::frequency_aware_renaming` toggle,
            default **on** in the `Maximum` built-in profile, default
            **off** in `Safe` (renaming character choice never changes
            correctness either way, but `Safe` is defined as
            "no rewrite passes beyond dead-code elimination" per Phase
            13 — frequency-aware renaming is a refinement of an
            existing always-on rename, not a new rewrite pass, so it
            is gated purely for user predictability/diffability, not
            safety).
      - [x] Deterministic tie-breaking rule documented and unit-tested:
            same input + same protected-names list must always produce
            the same output across runs (no `HashMap` iteration-order
            leakage) — regression test `renaming_is_deterministic`.
      - [x] `fixtures/frequency_renaming.glsl` + Rust unit tests
            covering: a shader with heavily skewed character frequency
            (many `x`/`y`/`z` swizzles) golfs to a smaller **DEFLATE**
            estimate than the naive occurrence-count assignment on the
            same input, verified by asserting
            `budget::estimate(golfed_freq_aware) <=
            budget::estimate(golfed_naive)` on at least three
            realistic fixtures.
- [x] **29.2 — Swizzle-alphabet selection** (`rust-core/src/golfer.rs`
      or a new `swizzle.rs`): GLSL accepts three interchangeable
      swizzle-letter sets for vector field access — `.xyzw`, `.rgba`,
      `.stpq` — all three already valid, unrestricted Shadertoy GLSL.
      Mirrors `Shader Minifier`'s `--field-names` flag.
      - [x] Detect every `.xyzw`/`.rgba`/`.stpq`-style swizzle access in
            the source (never touching struct member access — reuse
            the existing `struct_body_ranges`/`strictly_inside_any`
            scope-safety machinery from `golfer.rs`) and offer
            recoloring to whichever of the three alphabets compresses
            best under the Phase 12 estimator, given the letter/bigram
            frequencies already computed for 29.1 (e.g. a shader
            already dense in `r`/`g`/`b` benefits from `.rgba`; a
            shader dense in identifiers named `s...`/`t...` may benefit
            from avoiding `.stpq` collision pressure).
      - [x] New `UshaderGolfOptions` field `swizzle_alphabet` (`Auto` /
            `Xyzw` / `Rgba` / `Stpq`), surfaced as a combo in
            `golf_controls.cpp`, persisted by the Phase 13
            `.ushaderprofile` format (bump the profile schema minor
            version, keep the JSON reader backward-compatible with
            profiles saved before this field existed — absent field
            defaults to `Auto`).
      - [x] `fixtures/swizzle_alphabet.glsl` exercising all three
            alphabets round-tripping through the shader compiler
            equivalence check (Phase 15's multi-frame safety net) to
            prove `.rgba`-recolored output renders identically to the
            `.xyzw` source.
- [x] **29.3 — Non-constant swizzle/vector factoring** (extends
      `reduce_constant_vectors` in `aggressive.rs`, which today only
      folds literal-constant vectors like `vec3(1.,1.,1.)` →
      `vec3(1.)`): generalize to identical **pure identifier**
      arguments, e.g. `vec3(a,a,a)` → `vec3(a)`, `vec4(p.x,p.x,p.x,1.)`
      is left alone (not all-equal, no-op), `vec2(n,n)` → `vec2(n)`.
      Reuses the exact purity/whole-argument constraints already
      established for `eliminate_common_subexpressions` (bare
      identifier or a call from the same pure-builtin whitelist,
      never a side-effecting sub-expression) so no new safety class is
      invented.
      - [x] `AggressiveOptions::factor_repeated_vector_args` toggle.
      - [x] `fixtures/vector_argument_factoring.glsl` + four Rust unit
            tests mirroring the existing `constant_vectors.glsl`
            fixture's shape but with identifiers instead of literals.

### Phase 30 — Aggressive inlining & cross-statement subexpression golf

- [x] **30.1 — Multi-call-site, multi-statement function inlining**
      (`rust-core/src/inline.rs`): today's `inline_single_call_functions`
      is restricted to a single call site and a single-`return`-expression
      body. Add a second, explicitly toggled pass,
      `inline_aggressive`, mirroring `Shader Minifier`'s
      `--aggressive-inlining`:
      - [x] Multi-statement bodies are inlined via the comma operator
            into an expression-statement sequence **only** when every
            statement in the body is itself a pure-or-safe assignment
            expression already representable as an `Expr` (reusing
            `expr.rs`'s existing `Expr` grammar) — a body containing
            `if`/`for`/`while`/`return`-in-the-middle is never inlined
            by this pass (falls through to the existing single-return
            path or is left uninlined).
      - [x] Multiple call sites are each substituted independently
            (parameters re-substituted per call site via the existing
            `substitute_params`), with a **char-length AND
            estimated-DEFLATE-size guard**: a candidate is only
            inlined if the total golfed size (raw and, when the
            budget preset is compression-based, DEFLATE-estimated)
            after inlining is not larger than keeping the function
            call — inlining a multi-call-site function can regress
            size once past a small body, unlike the always-beneficial
            single-call-site case, so this pass must measure before
            committing rather than assume monotonic improvement.
      - [x] `AggressiveOptions::aggressive_inlining` toggle, off by
            default even in `Maximum` (this is the one pass in this
            entire document that can legitimately make output larger
            if mis-tuned, so it stays an explicit opt-in even for
            competitive users, consistent with `Shader Minifier`
            treating `--aggressive-inlining` as separate from its
            default `--no-inlining`-off behavior).
      - [x] `fixtures/aggressive_inlining.glsl` with a 2-call-site,
            3-statement-body function; Rust unit tests: inlines when
            net-smaller, refuses when net-larger (asserted against
            `budget::estimate`), never inlines a body containing
            control flow, never inlines a recursive or self-referential
            function (reuses `callgraph.rs`'s existing call-graph
            construction to detect cycles before considering a
            candidate).
- [x] **30.2 — Cross-statement common subexpression elimination via
      macro extraction** (new `rust-core/src/macro_cse.rs`): today's
      `eliminate_common_subexpressions` only matches whole
      declaration-statement initializers in an uninterrupted
      straight-line run (Phase 11 design, deliberately conservative).
      Add a second, separately toggled, **whole-shader** pass that
      finds a pure sub-expression (same purity whitelist as the
      existing pass — identifiers, numbers, operators,
      member/swizzle access, whitelisted pure builtins, **never** a
      user function) repeated **verbatim** three or more times
      anywhere in the file (not just inside one straight-line run,
      not just as a whole initializer), and extracts it to a
      `#define` macro when doing so is net-smaller under the active
      budget metric:
      - [x] Never extracts an expression containing an identifier that
            is itself later renamed differently in different scopes
            (reuses the existing scope-tree/`block_scope_tree` and
            `mutually_disjoint` machinery from `golfer.rs` to confirm
            every occurrence resolves to the *same* declaration before
            treating two textually-identical occurrences as the same
            value).
      - [x] Placed **after** renaming (Phase 29.1) in the fixpoint
            pipeline, so the macro body itself benefits from the
            shortest already-assigned identifier names, and **before**
            whitespace/layout, matching `Shader Minifier`'s own macro-
            insertion-early-in-the-pipeline precedent referenced in
            section 3.
      - [x] `AggressiveOptions::macro_cse` toggle.
      - [x] `fixtures/macro_cse.glsl` (a raymarcher-style shader
            reusing `dot(p,p)`-class expressions across multiple
            unrelated functions) + Rust unit tests: extracts only when
            net-smaller, never extracts across a scope boundary where
            the identifier's meaning differs, never extracts an
            expression touching a name in the protected-names list
            (macros are never renamed, matching `Shader Minifier`'s
            documented "won't rename occurrences of the macro" rule,
            reused here as a safety invariant, not just a naming
            convenience).
- [x] **30.3 — Statement-sequence fusion via the comma operator**
      (`rust-core/src/aggressive.rs`, new `fuse_statement_sequences`):
      mirrors `Shader Minifier`'s `--no-sequence` flag in reverse —
      that flag *disables* comma-fusion, meaning fusion is the default,
      aggressive behavior µShader is missing today. Adjacent
      expression-statements inside the same block (assignments,
      increment/decrement, calls to void functions already proven
      side-effect-bearing-but-independent) are fused into a single
      `a=b,c++,f(d);`-style statement when every statement in the run
      is already representable as an `Expr`, using the exact
      "cache-clearing on any non-qualifying statement" boundary rule
      Phase 11's CSE pass already established (documented bug #2 in
      `ROADMAP.md` Phase 11 about brace-after-`)`/`else` detection is
      explicitly reused here, not re-derived).
      - [x] Never fuses across a statement that is a declaration
            (declarations cannot appear inside a comma-expression),
            a control-flow statement, or a `return`.
      - [x] `AggressiveOptions::fuse_statement_sequences` toggle.
      - [x] `fixtures/statement_fusion.glsl` + Rust unit tests
            including a regression test for the exact brace-boundary
            bug class already fixed once for CSE (same boundary
            function, `is_statement_boundary`/`void_function_body_closers`
            from `aggressive.rs`, is reused rather than re-implemented,
            so this pass cannot reintroduce that bug independently).
- [x] **30.4 — Declaration hoisting / merge-across-function**
      (extends `merge_declarations` in `aggressive.rs`, which today
      only merges adjacent same-type declarations): add
      `hoist_declarations`, mirroring `Shader Minifier`'s
      `--move-declarations`, that relocates a same-type declaration
      forward to merge with an earlier same-type declaration **only**
      when no intervening statement reads or writes a variable that
      would change meaning by the move (conservative straight-line
      dominance check, reusing the scope tree from `golfer.rs` — no
      full data-flow analysis is implemented, matching this document's
      "never invent a new safety class" rule; a hoist that cannot be
      proven safe by this conservative check is simply skipped, never
      attempted speculatively).
      - [x] `AggressiveOptions::hoist_declarations` toggle.
      - [x] `fixtures/declaration_hoisting.glsl` + Rust unit tests:
            hoists across a safe gap, refuses to hoist across a
            read/write of an intervening same-named or shadowing
            declaration, refuses to hoist out of / into a different
            scope depth.
      - [x] Documentation note in this file (not a caveat hidden in
            code comments, since "no comments" is a hard convention):
            this pass is intentionally weaker than a full compiler's
            reaching-definitions analysis; it will decline several
            theoretically-safe hoists rather than risk an unsafe one —
            acceptable per the Phase 11 invariant.

### Phase 31 — Loop & control-flow golfing idioms

- [x] **31.1 — `for`-loop header golfing** (new
      `rust-core/src/loop_golf.rs`): recognizes the extremely common
      Shadertoy/demoscene idiom of folding the loop body's counter
      increment and continuation test into the `for(...)` header
      itself, e.g. rewriting
      ```
      float i = 0.;
      for (int j = 0; j < 8; j++) {
          i += 1.;
          ...
      }
      ```
      pattern-classes into the header-only form
      `for(float i=0.;i++<8.;)` when and only when: the loop variable
      is used **exclusively** as a counter inside the body (no other
      read of the pre-increment value that the rewrite would change),
      the increment is a plain `i++`/`i+=1.`/`i=i+1.` at the very top
      or very bottom of the body, and the continuation condition is a
      simple `i<N`/`i<=N` comparison against a loop-invariant bound —
      reusing `expr.rs`'s `Expr` parser to prove the body statement is
      exactly one of the recognized increment shapes rather than
      pattern-matching raw tokens.
      - [x] Never applied when the loop contains a `continue` (changes
            where the increment executes relative to the skipped
            iteration) unless the increment is proven to run before
            every `continue` reaches it (conservative: skip whenever a
            `continue` is present at all, matching this document's
            "decline rather than risk" rule from 30.4).
      - [x] `AggressiveOptions::loop_header_golf` toggle.
      - [x] `fixtures/loop_header_golf.glsl` (a raymarching loop and a
            fractal-iteration loop, the two most common Shadertoy loop
            shapes) + Rust unit tests, plus a Phase 15 multi-frame
            equivalence case specifically for this pass (behavioral
            risk is higher than most passes above, since it is the
            first pass in this document to restructure loop
            semantics rather than purely rename/reorder/fold).
- [x] **31.2 — `do`/`while` → `for` and `while` → `for` normalization**
      when strictly shorter (some GLSL ES targets golf `while(cond)`
      to `for(;cond;)` when there is no init/increment clause to
      lose): toggle `AggressiveOptions::loop_form_golf`, its own
      fixture, unit tests for both directions plus a "no change when
      not shorter" test (this pass must never fire when it would not
      reduce size, unlike purely-structural passes elsewhere in this
      document which have no size precondition — loop-form choice is
      cosmetic and reversible, so it is gated purely on the
      char/DEFLATE-size comparison already used by 30.1).
- [x] **31.3 — Early-return / guard-clause ternary extension**
      (extends `ternary_from_if_else` in `aggressive.rs`, which today
      only matches a plain `if/else` assignment or return pair):
      widen `try_match_ternary` to also recognize the common
      guard-clause idiom
      `if(cond){return a;} return b;` → `return cond?a:b;` at the tail
      of a function body — a strictly more constrained special case of
      the existing ternary machinery (single trailing `return` after
      the `if`, no `else` present), not a new safety class.
      - [x] Existing `AggressiveOptions` toggle for
            `ternary_from_if_else` covers this without a new flag
            (documented explicitly here so a reviewer does not go
            looking for a phantom new toggle).
      - [x] `fixtures/ternary_from_if_else.glsl` extended with the new
            guard-clause case; existing Rust unit tests extended, no
            new test file.

### Phase 32 — Compression-aware pass-order search ("Golf harder")

- [ ] **32.1 — Pass-order/subset search objective function**
      (`rust-core/src/golfer.rs` or new `search.rs`): today all enabled
      passes run to a fixpoint in one fixed order every time. Because
      several passes in this document (29.1 renaming, 29.2 swizzle
      alphabet, 30.1 aggressive inlining, 30.2 macro CSE) can each
      individually help or hurt the **final compressed** size
      depending on interaction effects, add an optional search mode
      that runs a small, bounded number of candidate pass-order/subset
      combinations (never combinatorial-explosive — capped at a fixed,
      documented budget, e.g. a bounded local hill-climb over "toggle
      one Phase 29–31 pass on/off relative to the current best," not
      an exhaustive permutation of all passes) and keeps whichever
      candidate scores smallest under the currently-selected Phase 12
      budget metric (raw or DEFLATE).
      - [ ] Deterministic and reproducible: same input + same enabled
            pass set + same budget preset always converges to the same
            chosen combination (no randomized search, no time-based
            seed) — required so the Phase 15 equivalence net and CI
            golden-output tests stay stable.
      - [ ] Exposed as a single "Golf harder" button in
            `golf_controls.cpp` next to the existing "Run golf" primary
            button (per Phase 10.4's button conventions: secondary
            flat button, not a second primary-accent button), which
            runs the search and, if a smaller result is found, offers
            it as a one-click "Apply" **diff** against the current
            output rather than silently replacing it — consistent with
            Phase 13's "nothing changes silently" precedent for
            profile loading.
      - [ ] Bounded runtime: the search must complete within a fixed,
            documented wall-clock budget (target: sub-second on a
            typical Shadertoy-sized shader) so it never blocks the UI
            thread — run on the existing background-compile thread
            already used for live preview, not the UI thread.
      - [ ] Rust unit tests: search never returns a result larger than
            running the default fixed pass order alone; search is
            deterministic across repeated runs on the same input;
            search respects the protected-names list and every
            safety guard already established by the individual passes
            it is choosing between (it is purely an orchestration
            layer — it invents no new transformation of its own).
- [ ] **32.2 — Documented, non-blocking accuracy caveat for the DEFLATE
      objective** (`golf.md`/`ROADMAP.md` text only, no code): note,
      next to the Phase 12 budget section, that Crinkler-class
      context-modelling compressors (used for real 4k/64k demoscene
      executables, see section 3 above) can rank renaming/ordering
      choices differently than plain DEFLATE. This is recorded as a
      known, accepted limitation of the estimator this search
      optimizes against — not a defect to fix in this document's
      scope (see Phase 33.5, explicitly out of scope).

### Phase 33 — Manual idiom catalogue ("Golf Tips" — opt-in only, never auto-applied)

The one category of technique used by top Shadertoy golfers that
**cannot** be safely automated without risking a behavior change:
numeric approximations, cheaper-but-not-identical trigonometric/
geometric substitutions, and other "close enough" idioms. Per the
Phase 11 invariant and the corollary at the top of this document,
none of this phase's content is ever applied by a pass — it is a
read-only reference panel.

- [ ] **33.1 — `src/ui/golf_tips_panel.cpp` (new, Win32-shell-native
      per the section 2 UI-framework convention — built directly
      against the Phase 22–27 Win32/GDI+/Direct2D shell, never against
      the retired ImGui shell)**: a read-only, searchable list of
      well-known manual golfing idioms, each entry citing which
      technique/source catalogue (section 3 above) it is drawn from,
      for example:
      - Short rotation-matrix constants for common angles (Fabrice
        Neyret's catalogue).
      - Cheap trigonometric/geometric identity substitutions (Xor's
        catalogue).
      - Compact hash/noise one-liners commonly reused across
        Shadertoy golf entries.
      - [ ] Every entry states explicitly, in the UI copy itself, that
            applying it is the user's manual choice and changes shader
            *output*, not just its size — this is a hard requirement,
            not a suggestion, given the Phase 11 invariant this whole
            document is subordinate to.
      - [ ] No entry is ever inserted into the editor automatically;
            the panel only offers a "Copy snippet" action, mirroring
            the existing clipboard-copy pattern already used for
            golfed-output copy in Phase 6.
- [ ] **33.2 — Cross-reference from the Phase 14 "Explain Golf" trace
      view**: when the trace shows a shader that is already near a
      Phase 12 budget threshold, surface a non-modal hint pointing at
      the Phase 33.1 panel ("N bytes over budget — see Golf Tips for
      manual techniques") rather than silently suggesting a specific
      rewrite.
- [ ] **33.3 — Fixtures**: none required — this phase ships no
      transformation code, only static reference content and one new
      read-only panel; nothing here has golf-behavior to regress.
- [ ] **33.4 — Explicitly in scope**: an optional, dev-only, offline,
      not-shipped comparison script (`scripts/benchmark_vs_shader_minifier.*`,
      never built into `ushader.exe`, never run by CI against the
      network) that a contributor can run **locally** with their own
      separately-installed copy of `Shader Minifier`/Mono to sanity-
      check µShader's output size against it on the `fixtures/*.glsl`
      corpus — informational only, never a merge gate, never a runtime
      dependency, consistent with the Offline-First corollary above.
- [ ] **33.5 — Explicitly out of scope for this document**: a
      Crinkler-class context-modelling compression estimator (see
      32.2); a general reaching-definitions/data-flow engine beyond
      the conservative straight-line checks reused throughout Phases
      29–31; any multi-buffer/`iChannel` cross-pass golfing (ruled out
      already by `ROADMAP.md` section 7); auto-applying any Phase 33.1
      idiom without explicit user action.

---

## 6. Delivery ordering & versioning

Follows the exact scheme already defined in `ROADMAP.md` section 3:
`MAJOR.MINOR` maps to phase number, `PATCH` for fixes,
auto-incrementing `BUILD`. Recommended sequencing, each shippable
independently and each closing with its own `CHANGELOG.md` entry and
Git tag, per the section 2 conventions reproduced above:

- [ ] Phase 29 (renaming/swizzle golf) first — purely refines existing,
      always-on behavior; lowest behavioral risk, highest immediate
      compressed-size payoff given it plugs directly into the Phase 12
      estimator already shipped.
- [ ] Phase 30 (inlining/CSE/fusion/hoisting) second — each sub-phase
      is independently toggleable and independently fixture-tested, so
      they may ship as separate `PATCH`/`MINOR` releases within the
      phase rather than as one large `MINOR` bump, mirroring the
      precedent `ROADMAP.md` section 3 already documents for Phase 21.
- [ ] Phase 31 (loop/control-flow golf) third — highest behavioral risk
      in this document (loop restructuring), gets the most Phase 15
      equivalence-net coverage before being enabled by default in any
      built-in profile.
- [ ] Phase 32 (search) fourth — depends on Phases 29–31 existing as
      independently toggleable passes to search over.
- [ ] Phase 33 (manual idiom panel) last, and independent of the
      Win32-shell migration timeline in `ROADMAP.md` Phases 22–28 —
      schedule it alongside whichever of those phases is current when
      Phase 33 starts, since it is pure UI work with no `rust-core`
      dependency on Phases 29–32 landing first.
