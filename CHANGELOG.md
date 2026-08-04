# Changelog

All notable changes to µShader, from the point of view of using the
application.

## [Unreleased]

## [4.1.0] - 2026-08-04

### Added

- New **Capture GIF** action for the live viewport, with three
  built-in size/duration presets, animated and looping.
- The **Twigl Export** tab (mode, ES 3.00, MRT, backbuffer, sound) now
  keeps its own state per document, the same way the golfing profile
  already did.
- New **Import twigl shader** button: pastes twigl.app-formatted code
  and converts it back into a normal, editable shader.
- The "Copy for twigl.app" button (and the matching command-palette
  action) now shows a confirmation message after copying.
- Snippet names from the twigl snippet library are now automatically
  protected from renaming, avoiding a naming conflict after inserting
  a snippet.
- The "Sound" toggle's accessibility label now states clearly that it
  refers to a separate program from the one shown, specific to
  twigl.app.

### Fixed

- Twigl.app exports in **ES 3.00** mode now use current texture
  functions (the old ones no longer compiled in a real browser).
- Multi-target (**MRT**) exports in ES 3.00 now include the layout
  information WebGL2 requires.
- The **Backbuffer** and **Sound** toggles in the Twigl export panel
  now genuinely update the live preview.
- The command palette's "Copy as twigl" action now always matches
  what's shown on screen (including in MRT mode).
- Restoring a session on launch now correctly reopens the document and
  tab that were active when the app was last closed.
- A document that was intentionally cleared (but not saved) is no
  longer replaced with the default shader on the next launch.
- The stats panel now correctly reports whether frequency-aware
  renaming actually ran on the last pass.
- Improved the performance of the frequency-aware renaming option used
  by "Golf harder".

## [4.0.0] - 2026-08-03

Major release: UI overhaul and twigl.app export.

### Added

- Tabs are now grouped (Author, Analyze, Export, Settings) for easier
  navigation.
- Multi-document editing is back: every open shader gets its own tab,
  and the whole session (every open document) is restored
  automatically on the next launch.
- The command palette now offers one-click shortcuts: copy as
  Shadertoy / Bonzomatic / bare `main()` / for twigl.app, save/load a
  golfing profile, import an exclude-name list, capture a PNG
  screenshot of the live viewport, and export a self-contained HTML
  session report.
- New **Twigl Export** tab: converts the golfed shader into the format
  expected by twigl.app, with a mode picker (Classic/Geek/Geeker/
  Geekest), an ES 3.00 toggle, MRT/Backbuffer/Sound toggles, a live
  preview and a byte-budget gauge, opened via `Ctrl+Alt+T`.

## [3.1.0] - 2026-08-03

### Added

- New **Golf harder** button, which automatically searches for an even
  smaller combination of golfing options than the current settings,
  with a "Deep search" option.
- The stats panel gained a new section summarizing the reduction
  achieved and which passes actually fired.
- New **Golf Tips** tab: a searchable catalogue of manual reduction
  techniques (rotation constants, common trigonometric identities,
  compact noise generators, 3D shape primitives, color gradients,
  gamma/tonemap corrections) — every entry can be copied or inserted
  straight into the editor with one click.
- The "Trace" view now shows a banner whenever the golfed result
  exceeds the chosen size budget, linking directly to the Golf Tips
  tab.
- The golfing controls panel is now an always-visible side panel
  instead of a full-screen tab, with a "Golf" button and a "Formatted
  view" button directly accessible.

## [3.0.0] - 2026-07-22

Major change: the application was entirely rebuilt on a native Windows
interface — lighter, with no external runtime to install.

### Changed

The look and shortcuts stay the same, but a few features from the
previous version were not yet ported to this new interface: golfing
profiles (save/load), multi-document editing and session restore,
"Copy as Shadertoy/Bonzomatic/main()" shortcuts, exclude-name-list
import, HTML session report, PNG screenshot, and live-viewport
video/GIF recording. These were reintroduced in versions 3.1.0/4.0.0
above, except video/GIF recording (GIF returned in the Unreleased
section above).

## [2.1.0] through [2.21.0] - 2026-07-22

Groundwork for the new native interface (see 3.0.0): no visible change
in the shipped application during this period, except for the points
below.

### Added

- Support for Windows screen readers (Narrator and similar) on the
  title bar buttons and checkboxes.
- An internal color-contrast check ensures text stays readable
  according to accessibility standards (WCAG AA).
- **Colorblind-safe status indicators** checkbox (different shapes in
  addition to colors) in the Appearance tab.
- **UI text size** slider (13 to 28 pt) in the Appearance tab, with
  automatic scaling based on the display.
- Import of a Shader Minifier–style exclude-name list.
- Three one-click copy shortcuts: "Copy as Shadertoy", "Copy as
  Bonzomatic", "Copy as bare main()".
- The golfing profile format (`.ushaderprofile`) is now documented and
  versioned, for anyone who wants to generate or read one outside the
  application.

### Fixed

- Fixed a bug preventing screen readers from correctly detecting UI
  elements.

## [2.0.0] and [2.0.1] - 2026-07-19 / 2026-07-20

### Added

- Export a **session report** as a self-contained HTML file (works
  offline, no connection needed), including the source code, the
  golfed code, the reduction statistics, and optionally a screenshot
  of the live viewport.
- Drag-and-drop a `.glsl` file onto the window to open it directly in
  a new tab.
- **Recent files** list, available from the File menu; entries
  pointing at a deleted file are removed automatically.

## [1.9.0] and [1.9.1] - 2026-07-19

### Added

- **Command palette** (`Ctrl+Shift+P`): fuzzy search across every
  action in the app (run golf, switch tabs, toggle a pass, load/save a
  profile, toggle Compare mode, export...).
- **Customizable keyboard shortcuts** for the command palette, new
  tab, open, save, and close-tab, from a new "Keyboard Shortcuts" tab.
- **Minimap** for the Source and Golfed editors: a compact, colored
  overview of the file, useful on longer shaders.
- New **Diff** tab: shows at a glance what changed between the source
  code and the golfed code, removed text struck through in red, added
  text in green.

## [1.8.0] - 2026-07-19

### Added

- **Command-line tool** for automatically golfing a whole folder of
  shaders (useful for plugging into a game or demo build pipeline),
  with size-budget options, a detailed report, and a preview of the
  differences before applying.

## [1.7.0] - 2026-07-19

### Added

- **Multi-document editing**: one tab per open `.glsl` file, each with
  its own golfing settings.
- The session (every open document) is now **saved and restored
  automatically** on launch, with a confirmation before reopening
  files.
- A dot appears on the tab of a document with unsaved changes, and a
  confirmation is asked before closing it or quitting the app with
  unsaved changes.

## [1.6.0] - 2026-07-19

Internal groundwork for the automatic verification feature (Compare
mode): no visible change yet.

## [1.5.0] - 2026-07-19

### Added

- New **Trace** tab: lists, pass by pass, everything the golfing
  engine changed in the shader, with a before/after view per pass —
  including passes that made no change, shown grayed out.

## [1.4.0] and [1.4.1] - 2026-07-19

### Added

- **Golfing profiles**: save and reload a set of golfing settings
  (`.ushaderprofile`), plus three ready-to-use built-in profiles
  (`Maximum`, `Safe`, `None`).

## [1.3.0] and [1.3.1] - 2026-07-19

### Added

- **Compressed-size budgets**: the stats panel now also estimates the
  compressed size (what actually matters for code-golf/demoscene
  competitions), with presets (Shadertoy, X/Twitter, 4KB, 8KB, 64KB...)
  and a green/amber/red badge showing whether the budget is met.
- The application now launches maximized.

## [1.2.0] through [1.2.5] - 2026-07-18 / 2026-07-19

### Added

- New **dark interface**, inspired by a professional video-editing
  application, with a custom title bar.
- Live-viewport video recording to MP4/WebM now works out of the box
  right after installing, with nothing extra to install.
- Two new golfing passes (algebraic identity simplification like
  `x*1` → `x`, and elimination of redundant computations), each with
  its own checkbox and counter.

### Fixed

- Fixed two cases where golfing produced an incorrect result with the
  new redundant-computation-elimination pass.

## [1.1.0] - 2026-07-18

### Added

- **Live-viewport recording** to an animated GIF, and to MP4/WebM if
  `ffmpeg` is installed on the machine.

### Fixed

- Opening/saving files whose path contains accented or special
  characters (for example the "µShader" folder itself).
- A console window briefly flashed behind the main window on launch;
  it no longer appears.

## [1.0.0] - 2026-07-18

First stable release, with a dedicated Windows installer
(`uShader-Setup-*.exe`).

## [0.8.0] - 2026-07-18

### Added

- **About** window (version, copyright, contact links).
- Application and installer icons.

### Fixed

- Files whose path contains non-ASCII characters failed to load/save
  correctly.

## [0.7.0] - 2026-07-18

### Added

- **Open/Save** buttons for the source code.
- **Copy** button for the golfed code.
- **Export (Shadertoy)** button.
- **Screenshot** button for the live viewport, exported as PNG.

## [0.6.0] - 2026-07-18

### Added

- First golfing controls: an "Aggressive golfing" toggle and
  individual checkboxes for every transformation pass, plus a field to
  protect certain names.
- Stats panel: before/after size, reduction percentage, per-pass
  counters, budget badges.
- **Compare** mode: shows the source and golfed shaders side by side
  to visually confirm they render the same image.

## [0.5.0] - 2026-07-18

### Added

- A real text editor for the source and golfed code, with GLSL syntax
  highlighting.
- **Formatted view** toggle to read the golfed code across multiple
  lines.
- The offending line is highlighted directly in the editor on a
  compile error.

## [0.4.0] - 2026-07-18

### Added

- First complete graphical interface: Source / Golfed / Live viewport
  panels, resizable.
- The "Run golf" action actually golfs the code and shows the result.
- The live viewport shows the golfed shader running, with compile
  errors visible on screen.

## [0.3.0] - 2026-07-18

First preview: a window displays and animates a demo shader (no
editing yet).

## [0.1.0] and [0.2.0] - 2026-07-18

Project kickoff: initial repository and golfing-engine structure, no
usable interface yet.
