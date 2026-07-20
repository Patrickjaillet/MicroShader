#pragma once

#include <cstddef>

struct BudgetPreset
{
    const char* name;
    long long raw_limit;
    long long deflate_limit;
};

const BudgetPreset* budget_presets(std::size_t& count);
