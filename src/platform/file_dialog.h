#pragma once

#include <optional>
#include <string>

struct HWND__;
typedef HWND__* HWND;

std::optional<std::string> show_open_file_dialog_hwnd(HWND owner, const wchar_t* filter, const wchar_t* default_ext, const wchar_t* default_path = nullptr);
std::optional<std::string> show_save_file_dialog_hwnd(HWND owner, const wchar_t* filter, const wchar_t* default_ext, const wchar_t* default_name);
