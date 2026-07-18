# µShader

Native Windows 10/11 GLSL shader golfer — minify a Shadertoy-style
`mainImage` fragment shader and preview the result live.

µShader pairs a tokenizer-based Rust minification engine with a
native SDL3 + OpenGL + Dear ImGui application shell: paste a shader,
golf it, and immediately verify it renders identically in the live
viewport.

![uShader screenshot: Source, Golfed, and Viewport panels](docs/screenshot.png)

**Status: early development (Phase 5 — golfing controls & stats).
Aggressive-pass toggles, protected names, reduction stats, and
Compare mode are all in place; file import/export is not implemented
yet.** See `ROADMAP.md` for the full phase-by-phase plan.

## Building

Requirements:

- Windows 10 or 11
- Visual Studio 2022 Build Tools (MSVC, C++20)
- CMake ≥ 3.21
- Rust toolchain with the `x86_64-pc-windows-msvc` target

```bash
cmake -S . -B build
cmake --build build
```

## License

[MIT](LICENSE) — free to reuse, modify, and redistribute.

## About

**µShader**
Copyright © 2026 SANDEFJORD DEVELOPMENT (Patrick JAILLET) — All rights reserved
Email: contact.shaderstudio@gmail.com
Website: https://github.com/Patrickjaillet
Repository: https://github.com/Patrickjaillet/MicroShader
