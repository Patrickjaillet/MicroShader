#pragma once

#include <imgui.h>

namespace tokens
{
    constexpr ImVec4 from_hex(unsigned int hex, float alpha = 1.0f)
    {
        return ImVec4(
            static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
            static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
            static_cast<float>(hex & 0xFF) / 255.0f,
            alpha);
    }

    constexpr ImVec4 bg_app = from_hex(0x1E1E1E);
    constexpr ImVec4 bg_panel = from_hex(0x232323);
    constexpr ImVec4 bg_panel_raised = from_hex(0x2B2B2B);
    constexpr ImVec4 bg_field_sunken = from_hex(0x1B1B1B);
    constexpr ImVec4 bg_field_deepest = from_hex(0x000000);
    constexpr ImVec4 bg_hover = from_hex(0x3D3D3D);
    constexpr ImVec4 bg_active = from_hex(0x4A4A4A);

    constexpr ImVec4 border_hairline = from_hex(0x000000);
    constexpr ImVec4 border_subtle = from_hex(0x454545);

    constexpr ImVec4 accent = from_hex(0x2680EB);
    constexpr ImVec4 accent_hover = from_hex(0x4B9EFF);
    constexpr ImVec4 accent_active = from_hex(0x1B5FBD);

    constexpr ImVec4 text_primary = from_hex(0xE6E6E6);
    constexpr ImVec4 text_secondary = from_hex(0x9E9E9E);
    constexpr ImVec4 text_disabled = from_hex(0x5C5C5C);

    constexpr ImVec4 status_ok = from_hex(0x4CAF50);
    constexpr ImVec4 status_warning = from_hex(0xE9A23B);
    constexpr ImVec4 status_error = from_hex(0xE5484D);
}
