#pragma once

#include <string>
#include <vector>

enum class DiffSpanKind
{
    Unchanged,
    Removed,
    Added,
};

struct DiffSpan
{
    std::string text;
    DiffSpanKind kind = DiffSpanKind::Unchanged;
};

std::vector<DiffSpan> compute_unified_diff(const std::string& before, const std::string& after);
void render_diff_view(const std::vector<DiffSpan>& spans);
