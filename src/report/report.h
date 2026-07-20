#pragma once

#include <string>
#include <vector>

#include "ushader/golf_core.h"
#include "../render/shader_runner.h"

struct SDL_Window;

struct ReportOptions
{
    bool include_screenshot = false;
};

std::string html_escape(const std::string& text);
std::string base64_encode(const std::vector<unsigned char>& bytes);

std::string render_session_report_html(
    const std::string& document_name,
    const std::string& source,
    const std::string& golfed,
    const UshaderGolfStats& stats,
    const UshaderBudgetResult& budget,
    int budget_preset_index,
    const EquivalenceRunResult& equivalence,
    const ReportOptions& options,
    const std::vector<unsigned char>& screenshot_png);

void export_report_action(
    const std::string& document_name,
    const std::string& source,
    const std::string& golfed,
    const UshaderGolfStats& stats,
    const UshaderBudgetResult& budget,
    int budget_preset_index,
    const EquivalenceRunResult& equivalence,
    const ReportOptions& options,
    const std::vector<unsigned char>& screenshot_png,
    SDL_Window* window);
