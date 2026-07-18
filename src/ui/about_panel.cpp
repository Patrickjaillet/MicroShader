#include "about_panel.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include "../render/texture.h"
#include "theme_tokens.h"
#include "ushader/version.h"

namespace
{
    void link_text(const char* label, const char* url)
    {
        ImGui::TextColored(tokens::accent, "%s", label);
        if (ImGui::IsItemClicked())
        {
            SDL_OpenURL(url);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
    }

    void hairline()
    {
        ImGui::PushStyleColor(ImGuiCol_Separator, tokens::border_subtle);
        ImGui::Separator();
        ImGui::PopStyleColor();
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

    ImGui::PushStyleColor(ImGuiCol_PopupBg, tokens::bg_panel_raised);
    ImGui::PushStyleColor(ImGuiCol_Border, tokens::border_subtle);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 24.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);

    if (ImGui::BeginPopupModal("About uShader", &show_about, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
    {
        float card_width = 380.0f;

        if (logo.id != 0)
        {
            float display_width = card_width;
            float display_height = display_width * (static_cast<float>(logo.height) / static_cast<float>(logo.width));
            ImGui::Image(static_cast<ImTextureID>(logo.id), ImVec2(display_width, display_height));
        }

        ImGui::Spacing();
        ImGui::TextColored(tokens::text_primary, "uShader %s", USHADER_VERSION_STRING);
        hairline();

        ImGui::TextColored(tokens::text_secondary, "Copyright (c) 2026 SANDEFJORD DEVELOPMENT (Patrick JAILLET)");
        ImGui::TextColored(tokens::text_secondary, "All rights reserved");
        ImGui::Spacing();

        link_text("contact.shaderstudio@gmail.com", "mailto:contact.shaderstudio@gmail.com");
        link_text("https://github.com/Patrickjaillet", "https://github.com/Patrickjaillet");
        link_text("https://github.com/Patrickjaillet/MicroShader", "https://github.com/Patrickjaillet/MicroShader");

        ImGui::Spacing();
        hairline();
        ImGui::TextColored(tokens::text_secondary, "MIT License");
        ImGui::TextColored(tokens::text_secondary, "Includes FFmpeg (GPL) for MP4/WebM recording");
        ImGui::TextColored(tokens::text_secondary, "see THIRD_PARTY_NOTICES.md");

        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(card_width, 0.0f)))
        {
            show_about = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}
