# µShader — Contenu du zip & procédure Git / Build / Release

## 1. Résumé du projet

**µShader** est une application native Windows 10/11 (SDL3 + OpenGL +
Dear ImGui, façon Adobe Premiere Pro sombre avec fenêtre borderless) qui
« golfe » (minifie) un fragment shader GLSL style Shadertoy (`mainImage`)
et prévisualise le rendu en direct pour vérifier que le résultat golfé
est visuellement identique à la source.

- **Éditeur**, version dans `VERSION` : `2.0.1`
- **Éditeur légal** : SANDEFJORD DEVELOPMENT (Patrick JAILLET)
- **Dépôt GitHub** : https://github.com/Patrickjaillet/MicroShader
- **Licence** : MIT (+ `ffmpeg.exe` sous GPL, embarqué en subprocess — voir `THIRD_PARTY_NOTICES.md`)
- **Moteur de golfing** : cœur en Rust (`rust-core/`, via `cbindgen` → C API `capi.rs`), consommé par l'app C++ (`src/`)
- **CLI batch** : binaire `golf` (rust-core), pour intégrer µShader dans un pipeline d'assets hors-ligne

## 2. Arborescence complète du zip (156 fichiers)

```
uShader.zip
├── .gitignore
├── CHANGELOG.md               (dernière entrée : 2.0.1 — 2026-07-20)
├── CLAUDE.md                  ⚠️ listé dans .gitignore → NE PAS committer
├── CMakeLists.txt             build C++ (lit VERSION, génère cmake/version.h.in)
├── LICENSE                    MIT
├── README.md
├── ROADMAP.md
├── THIRD_PARTY_NOTICES.md
├── VERSION                    → "2.0.1"
├── utf8_test.obj              ⚠️ artefact de build (.obj est dans .gitignore) — à supprimer avant commit
│
├── assets/
│   ├── fonts/                 Inter.ttf, lucide.ttf (+ licences)
│   └── icons/                 app.ico, app.rc, app_source.png, installer.ico
│
├── cmake/
│   └── version.h.in           template généré en USHADER_VERSION_* / USHADER_BUILD
│
├── docs/
│   ├── design/                color-palette.svg, icon-states.svg, ui-mockup-full.svg
│   ├── logo.png
│   └── screenshot.png
│
├── fixtures/                  ~25 fichiers .glsl de régression pour chaque pass de golfing
│                              + sample.ushaderprofile
│
├── include/ushader/
│   └── golf_core.h            interface C exposée par rust-core
│
├── installer/
│   └── ushader.iss            script Inno Setup 6 (voir §5)
│
├── rust-core/                 moteur de minification (crate Rust)
│   ├── Cargo.toml / Cargo.lock / cbindgen.toml
│   └── src/
│       ├── lib.rs, capi.rs, lexer.rs, expr.rs, callgraph.rs,
│       │   inline.rs, vocab.rs, budget.rs
│       ├── golfer.rs, aggressive.rs   (passes de golfing)
│       └── bin/golf.rs                (CLI batch)
│
├── src/                       application C++ (SDL3 + OpenGL + ImGui)
│   ├── main.cpp
│   ├── platform/               file_dialog, paths, recorder, screenshot, stb_impl, utf8
│   ├── render/                 framebuffer, gl_functions, shader_runner, texture, default_shader
│   ├── report/                 report.cpp/.h, report_encoding (export HTML autonome)
│   └── ui/                     ~20 fichiers : editor, thème, palette de commandes,
│                                diff view, minimap, workspace multi-onglets,
│                                golf_controls/profile/trace, keybindings, etc.
│
└── tests/
    ├── golf_profile_roundtrip_test.cpp
    ├── report_encoding_test.cpp
    ├── rust_core_smoke_test.cpp
    └── workspace_roundtrip_test.cpp
```

## 3. Points d'attention avant de committer

- **`CLAUDE.md` et `utf8_test.obj`** ne doivent pas être versionnés :
  le premier est explicitement exclu par `.gitignore` (`/CLAUDE.md`),
  le second correspond au motif `*.obj` déjà ignoré — c'est un résidu de
  compilation qui traîne dans le zip, à supprimer avant `git add`.
- `.gitignore` exclut déjà `/build/`, `/out/`, `/dist/`, `*.obj`, `*.pdb`,
  `*.ilk`, `*.exe`, `*.dll`, `*.lib`, `.vs/`, `CMakeUserPresets.json`,
  `/rust-core/target/`, `Cargo.lock`, `/ancien/`.
  → **`Cargo.lock` étant ignoré**, ne pas forcer son ajout au commit.
- La version applicative vient d'un seul endroit : le fichier **`VERSION`**
  (actuellement `2.0.1`), lu par `CMakeLists.txt` pour peupler
  `cmake/version.h.in` (`USHADER_VERSION_MAJOR/MINOR/PATCH`, `USHADER_BUILD`).
  L'installeur Inno Setup (`installer/ushader.iss`), lui, prend sa version
  en paramètre de ligne de commande (`/DMyAppVersion=...`), donc les deux
  doivent être tenus synchronisés manuellement au moment du build.

## 4. Procédure Git — commit + push (VS Code / PowerShell)

Ouvrir un terminal **PowerShell** dans VS Code (`` Ctrl+` ``), se placer à la
racine du projet extrait, puis :

```powershell
# 0) Nettoyage des fichiers qui ne doivent pas être committés
Remove-Item .\utf8_test.obj -ErrorAction SilentlyContinue
# CLAUDE.md est déjà ignoré par .gitignore : rien à faire, git ne le verra pas

# 1) Initialiser le dépôt (uniquement si ce n'est pas déjà fait)
git init
git branch -M main
git remote add origin https://github.com/Patrickjaillet/MicroShader.git

# Si le remote existe déjà (dépôt déjà initialisé auparavant) :
git remote set-url origin https://github.com/Patrickjaillet/MicroShader.git

# 2) Vérifier ce que git va suivre (CLAUDE.md et *.obj ne doivent PAS apparaître)
git status

# 3) Ajouter et committer
git add .
git commit -m "uShader 2.0.1 : recent files + drag-and-drop (Phase 18/19)"

# 4) Récupérer l'historique distant s'il existe déjà, puis pousser
git pull origin main --rebase   # seulement si le repo distant n'est pas vide
git push -u origin main
```

Pour une release taguée (recommandé, en cohérence avec `VERSION`) :

```powershell
git tag -a v2.0.1 -m "uShader 2.0.1"
git push origin v2.0.1
```

## 5. Build du logiciel (Release) — PowerShell

Pré-requis : Visual Studio 2022 Build Tools (MSVC, C++20), CMake ≥ 3.21,
toolchain Rust avec la cible `x86_64-pc-windows-msvc`, Inno Setup 6.

```powershell
# Configuration + build C++ (mode Release)
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Build du binaire CLI Rust "golf" (optionnel, pipeline batch)
cargo build --release --manifest-path rust-core\Cargo.toml --bin golf
```

## 6. Génération de l'installeur (Inno Setup) — PowerShell

Le numéro de version passé à Inno Setup doit correspondre au contenu de
`VERSION` (format `X.Y.Z.Build`, ex. `2.0.1.0`) :

```powershell
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" `
  /DMyAppVersion=2.0.1.0 `
  installer\ushader.iss
```

Cela produit `dist\uShader-Setup-2.0.1.0.exe` (voir `OutputDir=..\dist`
et `OutputBaseFilename=uShader-Setup-{#MyAppVersion}` dans `ushader.iss`).
L'installeur embarque `ushader.exe`, `ffmpeg.exe`, les polices, les
assets de branding et `THIRD_PARTY_NOTICES.md`.

## 7. Publier la release avec l'installeur sur GitHub

Via l'interface GitHub, ou en ligne de commande avec le **GitHub CLI**
(`gh`, à installer une fois : `winget install GitHub.cli`) :

```powershell
gh auth login    # une seule fois

gh release create v2.0.1 `
  ".\dist\uShader-Setup-2.0.1.0.exe" `
  --title "uShader 2.0.1" `
  --notes-file CHANGELOG.md
```

Cela crée le tag `v2.0.1` (s'il n'existe pas déjà côté distant), la
release GitHub, et y attache l'installeur `.exe` en tant qu'asset
téléchargeable — c'est exactement le fichier que le README pointe pour
les utilisateurs finaux (page **Releases** du dépôt
`Patrickjaillet/MicroShader`).

## 8. Récapitulatif en une seule séquence

```powershell
Remove-Item .\utf8_test.obj -ErrorAction SilentlyContinue
git add .
git commit -m "uShader 2.0.1"
git push -u origin main
git tag -a v2.0.1 -m "uShader 2.0.1"
git push origin v2.0.1

cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build --config Release
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /DMyAppVersion=2.0.1.0 installer\ushader.iss

gh release create v2.0.1 ".\dist\uShader-Setup-2.0.1.0.exe" --title "uShader 2.0.1" --notes-file CHANGELOG.md
```
