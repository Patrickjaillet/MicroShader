#include "command_palette.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <cctype>

namespace
{
    bool fuzzy_match(const std::string& query, const std::string& label)
    {
        if (query.empty())
        {
            return true;
        }

        std::size_t label_pos = 0;
        for (char query_char : query)
        {
            char query_lower = static_cast<char>(std::tolower(static_cast<unsigned char>(query_char)));
            bool found = false;
            while (label_pos < label.size())
            {
                char label_lower = static_cast<char>(std::tolower(static_cast<unsigned char>(label[label_pos])));
                ++label_pos;
                if (label_lower == query_lower)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return false;
            }
        }
        return true;
    }
}

void render_command_palette(bool& open, const std::vector<PaletteCommand>& commands)
{
    static bool was_open = false;
    static std::string query;

    if (open && !was_open)
    {
        ImGui::OpenPopup("##command_palette");
        query.clear();
    }
    was_open = open;

    if (!open)
    {
        return;
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.2f));
    ImGui::SetNextWindowSize(ImVec2(480.0f, 0.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopup("##command_palette"))
    {
        bool appearing = ImGui::IsWindowAppearing();
        if (appearing)
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##palette_query", "Type a command...", &query);

        ImGui::Separator();
        ImGui::BeginChild("##palette_results", ImVec2(0.0f, 240.0f));
        for (const PaletteCommand& command : commands)
        {
            if (!fuzzy_match(query, command.label))
            {
                continue;
            }
            if (ImGui::Selectable(command.label.c_str()))
            {
                command.execute();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    else
    {
        open = false;
    }
}
