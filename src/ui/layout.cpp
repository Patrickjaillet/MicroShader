#include "layout.h"

#include <imgui_internal.h>

void build_dock_layout(ImGuiID dockspace_id, bool narrow)
{
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

    if (narrow)
    {
        ImGui::DockBuilderDockWindow(kSourceWindowTitle, dockspace_id);
        ImGui::DockBuilderDockWindow(kGolfedWindowTitle, dockspace_id);
        ImGui::DockBuilderDockWindow(kViewportWindowTitle, dockspace_id);
    }
    else
    {
        ImGuiID center = dockspace_id;
        ImGuiID id_source = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.33f, nullptr, &center);
        ImGuiID id_golfed = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.5f, nullptr, &center);
        ImGuiID id_viewport = center;

        ImGui::DockBuilderDockWindow(kSourceWindowTitle, id_source);
        ImGui::DockBuilderDockWindow(kGolfedWindowTitle, id_golfed);
        ImGui::DockBuilderDockWindow(kViewportWindowTitle, id_viewport);
    }

    ImGui::DockBuilderFinish(dockspace_id);
}
