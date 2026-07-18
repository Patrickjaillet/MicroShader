# Changelog

All notable changes to µShader are documented in this file.

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
