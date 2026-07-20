#pragma once

#include <optional>
#include <string>

struct SDL_Window;

std::optional<std::string> show_open_file_dialog(SDL_Window* window, const wchar_t* filter, const wchar_t* default_ext, const wchar_t* default_path = nullptr);
std::optional<std::string> show_save_file_dialog(SDL_Window* window, const wchar_t* filter, const wchar_t* default_ext, const wchar_t* default_name);
