#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct GolfTraceStep
{
    std::string pass_name;
    std::string before;
    std::string after;
    std::size_t count = 0;
};

std::vector<GolfTraceStep> parse_golf_trace(const std::string& json);
