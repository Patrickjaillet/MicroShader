#pragma once

namespace tokens
{
    struct Color4
    {
        float x;
        float y;
        float z;
        float w;
    };

    constexpr Color4 from_hex(unsigned int hex, float alpha = 1.0f)
    {
        return Color4{
            static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
            static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
            static_cast<float>(hex & 0xFF) / 255.0f,
            alpha};
    }

    constexpr Color4 bg_app = from_hex(0x1E1E1E);
    constexpr Color4 bg_panel = from_hex(0x232323);
    constexpr Color4 bg_panel_raised = from_hex(0x2B2B2B);
    constexpr Color4 bg_field_sunken = from_hex(0x1B1B1B);
    constexpr Color4 bg_field_deepest = from_hex(0x000000);
    constexpr Color4 bg_hover = from_hex(0x3D3D3D);
    constexpr Color4 bg_active = from_hex(0x4A4A4A);

    constexpr Color4 border_hairline = from_hex(0x000000);
    constexpr Color4 border_subtle = from_hex(0x454545);

    constexpr Color4 accent = from_hex(0x2680EB);
    constexpr Color4 accent_hover = from_hex(0x4B9EFF);
    constexpr Color4 accent_active = from_hex(0x1B5FBD);

    constexpr Color4 text_primary = from_hex(0xE6E6E6);
    constexpr Color4 text_secondary = from_hex(0x9E9E9E);
    constexpr Color4 text_disabled = from_hex(0x5C5C5C);

    constexpr Color4 status_ok = from_hex(0x4CAF50);
    constexpr Color4 status_warning = from_hex(0xE9A23B);
    constexpr Color4 status_error = from_hex(0xE5484D);
}
