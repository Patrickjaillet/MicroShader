#include "shader_error_state.h"

#include <regex>

namespace
{
    int parse_compiler_line_number(const std::string& log)
    {
        static const std::regex nvidia_pattern(R"(0\((\d+)\))");
        static const std::regex mesa_pattern(R"(0:(\d+))");
        std::smatch match;
        if (std::regex_search(log, match, nvidia_pattern) && match.size() > 1)
        {
            return std::stoi(match[1].str());
        }
        if (std::regex_search(log, match, mesa_pattern) && match.size() > 1)
        {
            return std::stoi(match[1].str());
        }
        return -1;
    }

    std::string format_line_prefix(int line)
    {
        if (line > 0)
        {
            return "Line " + std::to_string(line) + ": ";
        }
        return std::string();
    }
}

ShaderErrorState make_shader_error_state(const std::string& raw_log, int fragment_header_lines)
{
    ShaderErrorState state;
    if (raw_log.empty())
    {
        return state;
    }

    state.has_error = true;
    state.raw_log = raw_log;
    state.source_line = parse_compiler_line_number(raw_log) - fragment_header_lines;
    state.display_message = format_line_prefix(state.source_line) + raw_log;
    return state;
}
