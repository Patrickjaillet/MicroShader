#include "theme.h"

#include <imgui.h>

#include "../platform/paths.h"
#include "icons.h"

ImFont* g_icon_font = nullptr;

void apply_theme()
{
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 6.0f;

    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    const ImVec4 accent = ImVec4(0.30f, 0.42f, 0.90f, 1.00f);
    const ImVec4 accent_hover = ImVec4(0.36f, 0.48f, 0.94f, 1.00f);
    const ImVec4 accent_active = ImVec4(0.24f, 0.34f, 0.80f, 1.00f);
    const ImVec4 background = ImVec4(0.98f, 0.98f, 0.99f, 1.00f);
    const ImVec4 surface = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    const ImVec4 border = ImVec4(0.85f, 0.86f, 0.89f, 1.00f);

    colors[ImGuiCol_WindowBg] = background;
    colors[ImGuiCol_ChildBg] = surface;
    colors[ImGuiCol_PopupBg] = surface;
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_FrameBg] = surface;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.94f, 0.95f, 0.98f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.92f, 0.97f, 1.00f);
    colors[ImGuiCol_TitleBg] = background;
    colors[ImGuiCol_TitleBgActive] = background;
    colors[ImGuiCol_MenuBarBg] = background;
    colors[ImGuiCol_ScrollbarBg] = background;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accent_active;
    colors[ImGuiCol_Button] = ImVec4(0.93f, 0.94f, 0.97f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = accent_hover;
    colors[ImGuiCol_ButtonActive] = accent_active;
    colors[ImGuiCol_Header] = ImVec4(0.90f, 0.92f, 0.98f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = accent_hover;
    colors[ImGuiCol_HeaderActive] = accent_active;
    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accent_active;
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.85f, 0.86f, 0.89f, 0.60f);
    colors[ImGuiCol_ResizeGripHovered] = accent_hover;
    colors[ImGuiCol_ResizeGripActive] = accent_active;
    colors[ImGuiCol_Tab] = background;
    colors[ImGuiCol_TabHovered] = accent_hover;
    colors[ImGuiCol_TabSelected] = accent;
    colors[ImGuiCol_TabSelectedOverline] = accent;
    colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.40f);
    colors[ImGuiCol_PlotLines] = accent;
    colors[ImGuiCol_PlotHistogram] = accent;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.30f);
}

void load_fonts(ImGuiIO& io, float base_size)
{
    std::string inter_path = asset_path("fonts/Inter.ttf");
    std::string icon_path = asset_path("fonts/lucide.ttf");

    ImFontConfig main_config;
    main_config.OversampleH = 2;
    main_config.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF(inter_path.c_str(), base_size, &main_config);

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
        0xe13c, 0xe13c,
        0xe14d, 0xe14d,
        0xe247, 0xe247,
        0};
    g_icon_font = io.Fonts->AddFontFromFileTTF(icon_path.c_str(), base_size, &icon_config, icon_ranges);
}
