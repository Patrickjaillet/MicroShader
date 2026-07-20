#pragma once

#include <imgui.h>

inline constexpr const char* kSourceWindowTitle = "Source";
inline constexpr const char* kGolfedWindowTitle = "Golfed";
inline constexpr const char* kTraceWindowTitle = "Trace";
inline constexpr const char* kDiffWindowTitle = "Diff";
inline constexpr const char* kViewportWindowTitle = "Viewport";

void build_dock_layout(ImGuiID dockspace_id, bool narrow);
