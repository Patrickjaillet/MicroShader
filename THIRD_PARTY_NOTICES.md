# Third-party notices

µShader is MIT-licensed (see `LICENSE`). As of v3.0.0, it bundles no
third-party binaries or fonts: the Win32/Direct2D/DirectWrite/GDI+
shell uses only APIs and fonts already in-box on Windows 10/11
(Segoe UI, Consolas), and the viewport recording feature that used to
bundle FFmpeg and the `gif-h` library has been removed. `stb_image`
and `stb_image_write` (public domain / MIT dual-license,
https://github.com/nothings/stb) are compiled directly into
`ushader.exe` for PNG screenshot encoding.
