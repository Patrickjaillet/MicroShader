#include "report.h"

#include <cctype>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>

#include <TextEditor.h>

#include "report_encoding.h"
#include "../platform/file_dialog.h"
#include "../platform/paths.h"
#include "../platform/utf8.h"
#include "../ui/budget_presets.h"
#include "../ui/glsl_format.h"
#include "../ui/glsl_language.h"
#include "ushader/version.h"

namespace
{
    bool is_word_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
    }

    const char* palette_css_class(TextEditor::PaletteIndex index)
    {
        switch (index)
        {
            case TextEditor::PaletteIndex::Keyword:
                return "tok-kw";
            case TextEditor::PaletteIndex::Number:
                return "tok-num";
            case TextEditor::PaletteIndex::String:
                return "tok-str";
            case TextEditor::PaletteIndex::CharLiteral:
                return "tok-str";
            case TextEditor::PaletteIndex::Preprocessor:
                return "tok-ppd";
            case TextEditor::PaletteIndex::Comment:
                return "tok-cmt";
            case TextEditor::PaletteIndex::KnownIdentifier:
                return "tok-known";
            case TextEditor::PaletteIndex::Identifier:
                return "tok-id";
            case TextEditor::PaletteIndex::Punctuation:
                return "tok-punct";
            default:
                return "tok-def";
        }
    }

    std::string highlight_glsl_html(const std::string& source)
    {
        std::string out;
        out.reserve(source.size() * 2);
        std::size_t i = 0;
        while (i < source.size())
        {
            char c = source[i];
            if (c == '\n' || c == ' ' || c == '\t' || c == '\r')
            {
                out += c;
                ++i;
                continue;
            }

            std::string token;
            if (is_word_char(c))
            {
                while (i < source.size() && is_word_char(source[i]))
                {
                    token += source[i];
                    ++i;
                }
            }
            else
            {
                token += c;
                ++i;
            }

            out += "<span class=\"";
            out += palette_css_class(classify_glsl_token(token));
            out += "\">";
            out += html_escape(token);
            out += "</span>";
        }
        return out;
    }

    std::string budget_badge_html(const char* label, long long limit, std::size_t actual)
    {
        if (limit < 0)
        {
            return "";
        }
        auto limit_u = static_cast<std::size_t>(limit);
        bool over = actual > limit_u;
        bool near_limit = !over && limit_u > 0 && actual * 10 >= limit_u * 9;
        const char* css_class = over ? "badge-err" : (near_limit ? "badge-warn" : "badge-ok");
        const char* glyph = over ? "&times;" : "&#10003;";
        std::ostringstream out;
        out << "<span class=\"badge " << css_class << "\">" << glyph << " " << html_escape(label)
            << ": " << actual << " / " << limit << "</span>";
        return out.str();
    }

    std::string current_timestamp()
    {
        std::time_t raw_time = std::time(nullptr);
        std::tm local_tm{};
        localtime_s(&local_tm, &raw_time);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_tm);
        return std::string(buffer);
    }

    std::string derive_report_filename(const std::string& document_name)
    {
        std::string base = document_name.empty() ? "shader" : document_name;
        std::size_t dot = base.find_last_of('.');
        if (dot != std::string::npos && dot != 0)
        {
            base = base.substr(0, dot);
        }
        return base + ".report.html";
    }

    const wchar_t kHtmlFilter[] = L"HTML report (*.html)\0*.html\0All files (*.*)\0*.*\0";
}

std::string render_session_report_html(
    const std::string& document_name,
    const std::string& source,
    const std::string& golfed,
    const UshaderGolfStats& stats,
    const UshaderBudgetResult& budget,
    int budget_preset_index,
    const EquivalenceRunResult& equivalence,
    const ReportOptions& options,
    const std::vector<unsigned char>& screenshot_png)
{
    std::size_t preset_count = 0;
    const BudgetPreset* presets = budget_presets(preset_count);
    const BudgetPreset* preset =
        (budget_preset_index >= 0 && static_cast<std::size_t>(budget_preset_index) < preset_count)
            ? &presets[budget_preset_index]
            : nullptr;

    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n";
    html << "<title>uShader Report - " << html_escape(document_name) << "</title>\n";
    html << "<style>\n"
        "body{background:#1e1e1e;color:#e6e6e6;font-family:Consolas,'Cascadia Code','Courier New',monospace;"
        "margin:0;padding:24px;}"
        "h1{font-size:20px;margin:0 0 4px 0;}"
        "h2{font-size:14px;margin:0 0 8px 0;color:#9e9e9e;text-transform:uppercase;letter-spacing:0.05em;}"
        ".meta{color:#9e9e9e;font-size:13px;margin-bottom:20px;}"
        ".card{background:#232323;border:1px solid #454545;border-radius:6px;padding:16px;margin-bottom:16px;}"
        ".grid{display:grid;grid-template-columns:1fr 1fr;gap:4px 24px;font-size:13px;}"
        "pre.code{background:#1b1b1b;border:1px solid #454545;border-radius:4px;padding:12px;overflow:auto;"
        "font-size:13px;line-height:1.45;white-space:pre;}"
        ".badge{display:inline-block;padding:2px 8px;border-radius:4px;font-size:12px;margin-right:8px;}"
        ".badge-ok{background:rgba(76,175,80,0.15);color:#4caf50;}"
        ".badge-warn{background:rgba(233,162,59,0.15);color:#e9a23b;}"
        ".badge-err{background:rgba(229,72,77,0.15);color:#e5484d;}"
        ".tok-kw{color:#569cd6;}"
        ".tok-num{color:#b5cea8;}"
        ".tok-str{color:#ce9178;}"
        ".tok-ppd{color:#c586c0;}"
        ".tok-cmt{color:#6a9955;font-style:italic;}"
        ".tok-known{color:#4ec9b0;}"
        ".tok-id{color:#e6e6e6;}"
        ".tok-punct{color:#9e9e9e;}"
        ".tok-def{color:#e6e6e6;}"
        "footer{color:#5c5c5c;font-size:12px;margin-top:24px;}"
        "\n</style>\n</head>\n<body>\n";

    html << "<h1>uShader Session Report</h1>\n";
    html << "<div class=\"meta\">" << html_escape(document_name) << " &middot; generated " << current_timestamp() << "</div>\n";

    html << "<div class=\"card\">\n<h2>Size &amp; budget</h2>\n";
    html << "<div>" << stats.input_chars << " -&gt; " << stats.output_chars << " chars ("
        << std::fixed << std::setprecision(1) << stats.reduction_pct << std::defaultfloat << "% reduction)</div>\n";
    html << "<div>Renamed: " << stats.renamed_count << "  Numbers shortened: " << stats.numbers_shortened << "</div>\n";
    html << "<div>Compressed estimate: " << budget.deflate_bytes << " bytes (deflate, fixed Huffman)</div>\n";
    if (preset != nullptr)
    {
        html << "<div style=\"margin-top:8px;\">";
        html << budget_badge_html(preset->name, preset->raw_limit, static_cast<std::size_t>(budget.raw_bytes));
        html << budget_badge_html(preset->name, preset->deflate_limit, static_cast<std::size_t>(budget.deflate_bytes));
        html << "</div>\n";
    }
    html << "</div>\n";

    html << "<div class=\"card\">\n<h2>Per-pass counters</h2>\n<div class=\"grid\">\n";
    html << "<div>Dead locals: " << stats.dead_locals_removed << "</div>";
    html << "<div>Ternaries: " << stats.ternaries_from_if_else << "</div>\n";
    html << "<div>Dead stores: " << stats.dead_stores_removed << "</div>";
    html << "<div>Declarations merged: " << stats.declarations_merged << "</div>\n";
    html << "<div>Constants folded: " << stats.constants_folded << "</div>";
    html << "<div>Braces removed: " << stats.braces_removed << "</div>\n";
    html << "<div>Constant vectors: " << stats.constant_vectors_reduced << "</div>";
    html << "<div>Parens removed: " << stats.redundant_parens_removed << "</div>\n";
    html << "<div>Trailing returns: " << stats.trailing_void_returns_removed << "</div>";
    html << "<div>Duplicate precision: " << stats.duplicate_precision_removed << "</div>\n";
    html << "<div>Compound assigns: " << stats.compound_assignments << "</div>";
    html << "<div>Dead functions: " << stats.dead_functions_removed << "</div>\n";
    html << "<div>Inc/dec: " << stats.increments_decrements << "</div>";
    html << "<div>Functions inlined: " << stats.functions_inlined << "</div>\n";
    html << "<div>Algebraic identities: " << stats.algebraic_identities_simplified << "</div>";
    html << "<div>Common subexpressions: " << stats.common_subexpressions_eliminated << "</div>\n";
    html << "</div>\n</div>\n";

    html << "<div class=\"card\">\n<h2>Equivalence check</h2>\n";
    if (!equivalence.valid)
    {
        html << "<div style=\"color:#9e9e9e;\">Not run for this session.</div>\n";
    }
    else if (equivalence.samples_failed == 0)
    {
        html << "<span class=\"badge badge-ok\">&#10003; Equivalent</span> ";
        html << equivalence.samples_total << "/" << equivalence.samples_total << " samples passed\n";
    }
    else
    {
        html << "<span class=\"badge badge-err\">&times; Differs</span> ";
        html << equivalence.samples_failed << "/" << equivalence.samples_total
            << " samples differ, max delta " << equivalence.worst_max_delta << "\n";
    }
    html << "</div>\n";

    if (options.include_screenshot && !screenshot_png.empty())
    {
        html << "<div class=\"card\">\n<h2>Viewport screenshot</h2>\n";
        html << "<img alt=\"Golfed shader viewport\" style=\"max-width:100%;border-radius:4px;\" src=\"data:image/png;base64,"
            << base64_encode(screenshot_png) << "\">\n</div>\n";
    }

    html << "<div class=\"card\">\n<h2>Source</h2>\n<pre class=\"code\">" << highlight_glsl_html(source) << "</pre>\n</div>\n";
    html << "<div class=\"card\">\n<h2>Golfed</h2>\n<pre class=\"code\">" << highlight_glsl_html(format_glsl(golfed)) << "</pre>\n</div>\n";

    html << "<footer>Generated by uShader " << USHADER_VERSION_STRING
        << " &middot; Copyright (c) 2026 SANDEFJORD DEVELOPMENT (Patrick JAILLET) &middot; opens offline, no network access required</footer>\n";
    html << "</body>\n</html>\n";

    return html.str();
}

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
    SDL_Window* window)
{
    std::wstring default_name = utf8_to_wide(derive_report_filename(document_name));
    std::optional<std::string> path = show_save_file_dialog(window, kHtmlFilter, L"html", default_name.c_str());
    if (!path.has_value())
    {
        return;
    }

    std::string html = render_session_report_html(
        document_name, source, golfed, stats, budget, budget_preset_index, equivalence, options, screenshot_png);
    write_utf8_file(*path, html);
}
