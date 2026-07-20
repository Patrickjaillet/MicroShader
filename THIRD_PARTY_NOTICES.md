# Third-party notices

µShader is MIT-licensed (see `LICENSE`). It bundles the following
third-party binary as a standalone, separately-invoked executable
(not linked into `ushader.exe`):

## FFmpeg (`ffmpeg.exe`)

Used to encode MP4 and WebM viewport recordings. Invoked as a
subprocess; µShader communicates with it only through a pipe of raw
pixel data and command-line arguments.

- Project: https://ffmpeg.org
- Build source: https://github.com/BtbN/FFmpeg-Builds
- License: GNU General Public License v3.0 (this build includes
  `libx264` and `libvpx`, which require the GPL variant rather than
  LGPL)
- Full license text: https://www.gnu.org/licenses/gpl-3.0.html
- Corresponding source code for the exact build used is published by
  the FFmpeg project and the BtbN/FFmpeg-Builds project at the links
  above.

GIF recording does not depend on FFmpeg; it uses the bundled
public-domain `gif-h` library (https://github.com/charlietangora/gif-h),
compiled directly into `ushader.exe`.

## ANGLE (`libEGL.dll`, `libGLESv2.dll`)

Used from Phase 22 onward to host the existing OpenGL ES 3 renderer
(`render/shader_runner.cpp`) inside WinUI 3's `SwapChainPanel`, which
has no native OpenGL context (GLES 3-over-D3D11). Bundled as
standalone DLLs shipped next to `ushader.exe` by the Inno Setup
installer — the same vendoring precedent Phase 9 set for `ffmpeg.exe`
— not statically linked.

- Project: https://github.com/google/angle
- License: BSD-style license (Google), see the project's `LICENSE` file
- Full license text: https://github.com/google/angle/blob/main/LICENSE
- ANGLE bundles several third-party components under their own
  licenses, notably Apache License 2.0 for SPIRV-Tools and
  SPIRV-Cross; their attributions are carried unmodified in ANGLE's
  own `LICENSE` file, referenced above rather than duplicated here.
- Corresponding source code for the exact build used is published by
  the ANGLE project at the link above.
