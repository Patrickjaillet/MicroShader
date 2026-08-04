# µShader — Git / Build / Release procedure

Internal maintainer reference. This is not the user manual — see
`README.md` for that.

## 1. Project summary

**µShader** is a native Windows 10/11 application (Win32 +
Direct2D/DirectWrite/GDI+ chrome, WGL-hosted OpenGL viewport — no
Dear ImGui, no SDL3) that "golfs" (minifies) a Shadertoy-style GLSL
fragment shader (`mainImage`) and previews the result live to confirm
it renders identically to the source.

- **Current version**: see `VERSION` (single source of truth)
- **Publisher**: SANDEFJORD DEVELOPMENT (Patrick JAILLET)
- **Repository**: https://github.com/Patrickjaillet/MicroShader
- **License**: MIT — no third-party binaries bundled (see
  `THIRD_PARTY_NOTICES.md`)
- **Golfing engine**: Rust core (`rust-core/`), exposed to the C++
  shell through a generated C header (`include/ushader/golf_core.h`,
  via `cbindgen`)
- **Batch CLI**: the `golf` binary (`rust-core/src/bin/golf.rs`), for
  embedding µShader in an offline asset pipeline

## 2. Repository layout

```
uShader/
├── .gitignore
├── CHANGELOG.md
├── CMakeLists.txt        C++ build (reads VERSION, generates cmake/version.h.in)
├── LICENSE                MIT
├── README.md              user manual
├── ROADMAP.md             internal planning doc — gitignored, not published
├── THIRD_PARTY_NOTICES.md
├── VERSION                single source of truth, e.g. "4.0.0"
│
├── assets/
│   └── icons/             app.ico, app.rc, app_source.png, installer.ico, UI icon set
│
├── cmake/
│   └── version.h.in       template generated into USHADER_VERSION_*/USHADER_BUILD
│
├── docs/
│   ├── ushaderprofile.schema.json / ushaderprofile-schema.md
│   ├── logo.png
│   └── screenshot.png      README hero image — recapture on every visible UI change
│
├── fixtures/               .glsl regression fixtures for the golfing engine
│
├── include/ushader/
│   └── golf_core.h          C ABI surface generated from rust-core
│
├── installer/
│   └── ushader.iss           Inno Setup script
│
├── rust-core/                minification engine (Rust crate)
│   ├── Cargo.toml / Cargo.lock / cbindgen.toml
│   └── src/                  golfer.rs, aggressive.rs, twigl.rs, search.rs,
│                              deflate.rs, gif.rs, and friends
│       └── bin/golf.rs        batch CLI entry point
│
├── src/                       C++ application (Win32 + Direct2D/DirectWrite/GDI+ + WGL)
│   ├── main_win32.cpp
│   ├── platform/               file dialogs, paths, screenshot/GIF capture, accessibility
│   ├── render/                 framebuffer, GL function loading, shader runner
│   ├── report/                 self-contained HTML session report
│   └── ui/                     editor, panels, command palette, workspace, tab strip, ...
│
└── tests/                      C++ test executables (ctest)
```

## 3. Before committing

- `.gitignore` already excludes build output (`/build/`, `/out/`,
  `/dist/`, `*.obj`, `*.pdb`, `*.exe`, `*.dll`, `*.lib`), `.vs/`,
  `CMakeUserPresets.json`, `/rust-core/target/`, `Cargo.lock`,
  `/CLAUDE.md`, `/ROADMAP.md`, and `/uShader.zip`. Don't force-add any
  of these.
- The application version comes from a single place: the **`VERSION`**
  file, read by `CMakeLists.txt` to populate `cmake/version.h.in`
  (`USHADER_VERSION_MAJOR/MINOR/PATCH`, `USHADER_BUILD`). The Inno
  Setup installer takes its version as a command-line parameter
  (`/DMyAppVersion=...`), so the two must be kept in sync by hand at
  build time.
- Before tagging a release, make sure `CHANGELOG.md` has an entry for
  it and `README.md`'s screenshot (`docs/screenshot.png`) still
  matches the current UI.

## 4. Git — commit and push (PowerShell)

```powershell
git status
git add <files>
git commit -m "uShader X.Y.Z: <summary>"
git push -u origin main
```

For a tagged release (recommended, matching `VERSION`):

```powershell
git tag -a vX.Y.Z -m "uShader X.Y.Z"
git push origin vX.Y.Z
```

## 5. Building the software (Release) — PowerShell

Prerequisites: Visual Studio 2022 Build Tools (MSVC, C++20), CMake ≥
3.21, a Rust toolchain with the `x86_64-pc-windows-msvc` target, and
(for the installer) Inno Setup 7.

```powershell
# Configure + build the C++ app in Release mode
# (this also builds rust-core via Cargo as part of the CMake build)
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Optional: build the batch CLI binary separately
cargo build --release --manifest-path rust-core\Cargo.toml --bin golf
```

Run the test suite with:

```powershell
ctest --test-dir build -C Release
```

## 6. Building the installer (Inno Setup)

The version passed to Inno Setup must match `VERSION` (format
`X.Y.Z.Build`, e.g. `4.0.0.0`):

```powershell
& "C:\Program Files\Inno Setup 7\ISCC.exe" `
  /DMyAppVersion=4.0.0.0 `
  installer\ushader.iss
```

This produces `dist\uShader-Setup-4.0.0.0.exe` (see `OutputDir` and
`OutputBaseFilename` in `ushader.iss`). The installer bundles
`ushader.exe`, the icon/UI asset set, and `THIRD_PARTY_NOTICES.md` —
no third-party runtime or binary is bundled (see
`THIRD_PARTY_NOTICES.md`).

## 7. Publishing a release on GitHub

Via the GitHub web UI, or with the **GitHub CLI** (`gh`, install once
with `winget install GitHub.cli`):

```powershell
gh auth login    # once

gh release create vX.Y.Z `
  ".\dist\uShader-Setup-X.Y.Z.0.exe" `
  --title "uShader X.Y.Z" `
  --notes-file CHANGELOG.md
```

This creates the `vX.Y.Z` tag (if it doesn't already exist remotely),
the GitHub release, and attaches the installer `.exe` as a downloadable
asset — the exact file `README.md` points end users to on the
repository's **Releases** page.

## 8. One-shot release sequence

```powershell
git add <files>
git commit -m "uShader X.Y.Z"
git push -u origin main
git tag -a vX.Y.Z -m "uShader X.Y.Z"
git push origin vX.Y.Z

cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release
& "C:\Program Files\Inno Setup 7\ISCC.exe" /DMyAppVersion=X.Y.Z.0 installer\ushader.iss

gh release create vX.Y.Z ".\dist\uShader-Setup-X.Y.Z.0.exe" --title "uShader X.Y.Z" --notes-file CHANGELOG.md
```
