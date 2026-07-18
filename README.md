# µShader

Native Windows 10/11 GLSL shader golfer — minify a Shadertoy-style
`mainImage` fragment shader and preview the result live.

µShader pairs a tokenizer-based Rust minification engine with a
native SDL3 + OpenGL + Dear ImGui application shell: paste a shader,
golf it, and immediately verify it renders identically in the live
viewport. The UI is a dark, Adobe Premiere Pro–style editing
workspace with a custom borderless window frame.

![uShader screenshot: Source, Golfed, and Viewport panels](docs/screenshot.png)

## Features

- **Golfing engine**: identifier renaming, numeric literal shortening,
  and whitespace stripping always run; an aggressive mode adds 14
  further transformation passes (dead-code elimination, constant
  folding, declaration merging, function inlining, and more), each
  individually toggleable, plus a protected-names list for identifiers
  that must never be renamed.
- **Live viewport** with the standard Shadertoy uniform set (`iTime`,
  `iResolution`, `iMouse`, `iDate`, `iFrame`, `iFrameRate`), and a
  Compare mode that renders the source and golfed shaders side by
  side to confirm golfing didn't change the output.
- **GLSL-aware text editor** (syntax highlighting, error-line
  highlighting on compile failure) for both the Source and Golfed
  panels, with a "Formatted view" toggle for reading the golfed
  one-liner across multiple lines.
- **Reduction stats**: char/byte counts, reduction percentage,
  per-pass counters, and size-budget badges (280/512/1024 bytes).
- **Import/export**: open and save `.glsl` files, copy the golfed
  output to the clipboard, export in Shadertoy format, and capture
  the viewport to a PNG screenshot.
- **Viewport recording**: capture the running shader to an animated
  GIF, or to MP4/WebM via a bundled `ffmpeg.exe` — no separate
  install required.

## Installing

Download the latest `uShader-Setup-*.exe` from the
[Releases](https://github.com/Patrickjaillet/MicroShader/releases)
page and run it. The installer is not code-signed, so Windows
SmartScreen may show an "unknown publisher" warning on first run —
click "More info" -> "Run anyway" to proceed.

Requires Windows 10 or 11 (64-bit). Verified on Windows 10 (LTSC 2019,
build 17763); not yet independently verified on Windows 11.

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

To build the installer, install [Inno Setup 6](https://jrsoftware.org/isinfo.php)
and run:

```bash
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /DMyAppVersion=1.0.0.0 installer\ushader.iss
```

## License

[MIT](LICENSE) — free to reuse, modify, and redistribute. Bundles a
GPL-licensed `ffmpeg.exe` for MP4/WebM recording as a standalone
executable invoked as a subprocess; see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## About

**µShader**
Copyright © 2026 SANDEFJORD DEVELOPMENT (Patrick JAILLET) — All rights reserved
Email: contact.shaderstudio@gmail.com
Website: https://github.com/Patrickjaillet
Repository: https://github.com/Patrickjaillet/MicroShader
