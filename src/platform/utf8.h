#pragma once

#include <string>

std::string wide_to_utf8(const wchar_t* wide);
std::wstring utf8_to_wide(const std::string& utf8);
