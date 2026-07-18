#include "about_panel.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include "../render/texture.h"
#include "ushader/version.h"

namespace
{
    void link_text(const char* label, const char* url)
    {
        ImGui::TextColored(ImVec4(0.30f, 0.42f, 0.90f, 1.0f), "%s", label);
        if (ImGui::IsItemClicked())
        {
            SDL_OpenURL(url);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
    }
}

void render_about_popup(bool& show_about, const Texture& logo)
{
    if (show_about)
    {
        ImGui::OpenPopup("About uShader");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("About uShader", &show_about, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (logo.id != 0)
        {
            float display_width = 420.0f;
            float display_height = display_width * (static_cast<float>(logo.height) / static_cast<float>(logo.width));
            ImGui::Image(static_cast<ImTextureID>(logo.id), ImVec2(display_width, display_height));
        }

        ImGui::Text("uShader %s", USHADER_VERSION_STRING);
        ImGui::Separator();
        ImGui::TextUnformatted("Copyright (c) 2026 SANDEFJORD DEVELOPMENT (Patrick JAILLET)");
        ImGui::TextUnformatted("All rights reserved");
        ImGui::Spacing();

        link_text("contact.shaderstudio@gmail.com", "mailto:contact.shaderstudio@gmail.com");
        link_text("https://github.com/Patrickjaillet", "https://github.com/Patrickjaillet");
        link_text("https://github.com/Patrickjaillet/MicroShader", "https://github.com/Patrickjaillet/MicroShader");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("MIT License");

        ImGui::Spacing();
        if (ImGui::Button("Close"))
        {
            show_about = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
