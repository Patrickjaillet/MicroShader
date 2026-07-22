#pragma once

#include <string>
#include <vector>

#include "golf_controls.h"

struct WorkspaceDocument
{
    std::string file_path;
    GolfPassToggles pass_toggles;
    std::string protected_names;
    int budget_preset_index = 3;
};

struct WorkspaceState
{
    int active_tab = 0;
    std::string layout_ini;
    std::vector<WorkspaceDocument> documents;
    float ui_font_size = 18.0f;
};

std::string serialize_workspace(const WorkspaceState& state);
bool deserialize_workspace(const std::string& text, WorkspaceState& state);

std::string workspace_session_path();
std::string workspace_layout_path();
const char* workspace_layout_name();
bool workspace_session_exists();
