#pragma once

#include <string>

#include "golf_controls.h"

std::string serialize_golf_profile(const GolfPassToggles& toggles, const std::string& protected_names, int budget_preset_index);
bool deserialize_golf_profile(const std::string& text, GolfPassToggles& toggles, std::string& protected_names, int& budget_preset_index);

GolfPassToggles builtin_profile_maximum();
GolfPassToggles builtin_profile_safe();
GolfPassToggles builtin_profile_none();
const char* builtin_profile_name(const GolfPassToggles& toggles);

std::string load_last_profile_path();
void save_last_profile_path(const std::string& path);
