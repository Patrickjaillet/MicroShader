#include "theme.h"

#include <imgui.h>

#include "../platform/paths.h"
#include "icons.h"
#include "theme_tokens.h"

ImFont* g_icon_font = nullptr;
ImFont* g_mono_font = nullptr;

void apply_theme()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 2.0f;
    style.ChildRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 2.0f;

    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 8.0f;
    style.GrabMinSize = 10.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.DockingSeparatorSize = 4.0f;

    using namespace tokens;
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = bg_app;
    colors[ImGuiCol_ChildBg] = bg_panel;
    colors[ImGuiCol_PopupBg] = bg_panel_raised;
    colors[ImGuiCol_Border] = border_hairline;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = bg_field_sunken;
    colors[ImGuiCol_FrameBgHovered] = bg_hover;
    colors[ImGuiCol_FrameBgActive] = bg_active;
    colors[ImGuiCol_TitleBg] = bg_panel_raised;
    colors[ImGuiCol_TitleBgActive] = bg_panel_raised;
    colors[ImGuiCol_TitleBgCollapsed] = bg_panel_raised;
    colors[ImGuiCol_MenuBarBg] = bg_panel_raised;
    colors[ImGuiCol_ScrollbarBg] = bg_panel;
    colors[ImGuiCol_ScrollbarGrab] = bg_hover;
    colors[ImGuiCol_ScrollbarGrabHovered] = bg_active;
    colors[ImGuiCol_ScrollbarGrabActive] = accent;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accent_active;
    colors[ImGuiCol_Button] = bg_panel_raised;
    colors[ImGuiCol_ButtonHovered] = bg_hover;
    colors[ImGuiCol_ButtonActive] = bg_active;
    colors[ImGuiCol_Header] = bg_hover;
    colors[ImGuiCol_HeaderHovered] = bg_hover;
    colors[ImGuiCol_HeaderActive] = bg_active;
    colors[ImGuiCol_Separator] = border_hairline;
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accent_active;
    colors[ImGuiCol_ResizeGrip] = ImVec4(border_subtle.x, border_subtle.y, border_subtle.z, 0.30f);
    colors[ImGuiCol_ResizeGripHovered] = accent_hover;
    colors[ImGuiCol_ResizeGripActive] = accent_active;
    colors[ImGuiCol_Tab] = bg_panel_raised;
    colors[ImGuiCol_TabHovered] = bg_hover;
    colors[ImGuiCol_TabSelected] = bg_panel_raised;
    colors[ImGuiCol_TabSelectedOverline] = accent;
    colors[ImGuiCol_TabDimmed] = bg_panel_raised;
    colors[ImGuiCol_TabDimmedSelected] = bg_panel_raised;
    colors[ImGuiCol_TabDimmedSelectedOverline] = border_subtle;
    colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.40f);
    colors[ImGuiCol_DockingEmptyBg] = bg_app;
    colors[ImGuiCol_PlotLines] = accent;
    colors[ImGuiCol_PlotHistogram] = accent;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_Text] = text_primary;
    colors[ImGuiCol_TextDisabled] = text_disabled;
    colors[ImGuiCol_NavCursor] = accent;
}

void load_fonts(ImGuiIO& io, float base_size)
{
    std::string inter_path = asset_path("fonts/Inter.ttf");
    std::string icon_path = asset_path("fonts/lucide.ttf");

    ImFontConfig main_config;
    main_config.OversampleH = 2;
    main_config.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF(inter_path.c_str(), base_size, &main_config);

    ImFontConfig mono_config;
    mono_config.OversampleH = 2;
    mono_config.OversampleV = 2;
    g_mono_font = io.Fonts->AddFontFromFileTTF(inter_path.c_str(), base_size * 0.85f, &mono_config);

    ImFontConfig icon_config;
    icon_config.OversampleH = 2;
    icon_config.OversampleV = 2;
    static const ImWchar icon_ranges[] = {
        0xe064, 0xe064,
        0xe076, 0xe077,
        0xe083, 0xe083,
        0xe093, 0xe093,
        0xe09e, 0xe09e,
        0xe0b2, 0xe0b2,
        0xe0f6, 0xe0f6,
        0xe0f9, 0xe0f9,
        0xe112, 0xe113,
        0xe11a, 0xe11c,
        0xe13c, 0xe13c,
        0xe14d, 0xe14d,
        0xe167, 0xe167,
        0xe1b2, 0xe1b2,
        0xe247, 0xe247,
        0};
    g_icon_font = io.Fonts->AddFontFromFileTTF(icon_path.c_str(), base_size, &icon_config, icon_ranges);
}
