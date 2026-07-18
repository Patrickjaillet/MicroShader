# Changelog

All notable changes to µShader are documented in this file.

## [0.4.0] - Unreleased

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

## [0.3.0] - Unreleased

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

## [0.2.0] - Unreleased

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

## [0.1.0] - Unreleased

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
