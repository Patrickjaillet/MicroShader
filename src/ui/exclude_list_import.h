#pragma once

#include <string>

struct SDL_Window;

std::string parse_exclude_name_list(const std::string& list_text);
std::string merge_protected_names(const std::string& existing_csv, const std::string& imported_csv);
void import_exclude_list_action(std::string& protected_names, SDL_Window* window);
