#pragma once

#include <string>
#include <vector>

std::string html_escape(const std::string& text);
std::string base64_encode(const std::vector<unsigned char>& bytes);
