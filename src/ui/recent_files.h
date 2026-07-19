#pragma once

#include <string>
#include <vector>

std::vector<std::string> load_recent_files();
void add_recent_file(const std::string& file_path);
void remove_recent_file(const std::string& file_path);
void clear_recent_files();
