#pragma once

#include <string>

struct ShaderErrorState
{
    bool has_error = false;
    std::string raw_log;
    std::string display_message;
    int source_line = -1;
};

ShaderErrorState make_shader_error_state(const std::string& raw_log, int fragment_header_lines);
