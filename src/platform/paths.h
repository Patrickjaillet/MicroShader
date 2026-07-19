#pragma once

#include <string>

std::string asset_path(const std::string& relative_path);
std::string app_data_dir();
std::string read_utf8_file(const std::string& utf8_path);
bool write_utf8_file(const std::string& utf8_path, const std::string& content);
bool file_exists(const std::string& utf8_path);
