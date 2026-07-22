#include "budget_presets.h"

namespace
{
    constexpr BudgetPreset k_presets[] = {
        {"Shadertoy", 65536, -1},
        {"X/Twitter shader", 280, -1},
        {"JS13K-style 13KB", -1, 13312},
        {"4KB intro", -1, 4096},
        {"8KB intro", -1, 8192},
        {"64KB intro", -1, 65536},
    };
}

const BudgetPreset* budget_presets(std::size_t& count)
{
    count = sizeof(k_presets) / sizeof(k_presets[0]);
    return k_presets;
}
