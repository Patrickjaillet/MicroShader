#include "glsl_numeric_literals.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace
{
    bool is_ident_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }
}

std::vector<GlslNumericLiteral> find_glsl_float_literals(const std::string& source)
{
    std::vector<GlslNumericLiteral> result;
    const std::size_t n = source.size();
    std::size_t i = 0;

    while (i < n)
    {
        char c = source[i];

        if (c == '/' && i + 1 < n && source[i + 1] == '/')
        {
            i += 2;
            while (i < n && source[i] != '\n') { i++; }
            continue;
        }
        if (c == '/' && i + 1 < n && source[i + 1] == '*')
        {
            i += 2;
            while (i + 1 < n && !(source[i] == '*' && source[i + 1] == '/')) { i++; }
            i = (i + 1 < n) ? i + 2 : n;
            continue;
        }

        bool starts_number = std::isdigit(static_cast<unsigned char>(c))
            || (c == '.' && i + 1 < n && std::isdigit(static_cast<unsigned char>(source[i + 1])));
        if (!starts_number)
        {
            i++;
            continue;
        }

        bool prev_is_ident = i > 0 && is_ident_char(source[i - 1]);
        if (prev_is_ident)
        {
            i++;
            continue;
        }

        std::size_t start = i;
        bool has_dot = false;
        bool has_exp = false;

        while (i < n && std::isdigit(static_cast<unsigned char>(source[i]))) { i++; }
        if (i < n && source[i] == '.')
        {
            has_dot = true;
            i++;
            while (i < n && std::isdigit(static_cast<unsigned char>(source[i]))) { i++; }
        }
        if (i < n && (source[i] == 'e' || source[i] == 'E'))
        {
            std::size_t save = i;
            std::size_t j = i + 1;
            if (j < n && (source[j] == '+' || source[j] == '-')) { j++; }
            if (j < n && std::isdigit(static_cast<unsigned char>(source[j])))
            {
                has_exp = true;
                i = j;
                while (i < n && std::isdigit(static_cast<unsigned char>(source[i]))) { i++; }
            }
            else
            {
                i = save;
            }
        }

        if (!has_dot && !has_exp)
        {
            // A bare integer -- not in scope (loop bounds, array sizes,
            // indices) -- leave it unconsumed past its digits.
            continue;
        }

        std::size_t length = i - start;
        std::string text = source.substr(start, length);
        try
        {
            double value = std::stod(text);
            result.push_back(GlslNumericLiteral{ start, length, value });
        }
        catch (...)
        {
            // Malformed per std::stod (shouldn't happen given the grammar
            // above) -- skip rather than crash.
        }
    }

    return result;
}

std::string format_glsl_float_literal(double value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6f", value);
    std::string text(buffer);

    std::size_t dot = text.find('.');
    if (dot != std::string::npos)
    {
        std::size_t last_non_zero = text.find_last_not_of('0');
        if (last_non_zero == dot)
        {
            last_non_zero = dot + 1;
        }
        text.erase(last_non_zero + 1);
    }
    return text;
}

SliderRange compute_slider_range(double base_value)
{
    if (base_value == 0.0)
    {
        return SliderRange{ -1.0, 1.0 };
    }
    double magnitude = std::fabs(base_value) * 2.0;
    if (base_value > 0.0)
    {
        return SliderRange{ 0.0, magnitude };
    }
    return SliderRange{ -magnitude, magnitude };
}

std::string splice_source(const std::string& source, std::size_t offset, std::size_t length,
    const std::string& replacement)
{
    std::string result = source.substr(0, offset);
    result += replacement;
    result += source.substr(offset + length);
    return result;
}

std::string literal_context_snippet(const std::string& source, std::size_t offset, std::size_t length,
    std::size_t max_chars)
{
    (void)length;
    std::size_t line_start = (offset == 0) ? 0 : source.rfind('\n', offset - 1);
    line_start = (line_start == std::string::npos) ? 0 : line_start + 1;
    std::size_t line_end = source.find('\n', offset);
    if (line_end == std::string::npos)
    {
        line_end = source.size();
    }

    std::size_t trim_start = line_start;
    while (trim_start < line_end && std::isspace(static_cast<unsigned char>(source[trim_start])))
    {
        trim_start++;
    }
    std::string line = source.substr(trim_start, line_end - trim_start);
    if (max_chars < 8 || line.size() <= max_chars)
    {
        return line;
    }

    std::size_t literal_pos_in_line = offset - trim_start;
    std::size_t half = max_chars / 2;
    std::size_t window_start = literal_pos_in_line > half ? literal_pos_in_line - half : 0;
    if (window_start + max_chars > line.size())
    {
        window_start = line.size() - max_chars;
    }
    std::string snippet = line.substr(window_start, max_chars);
    if (window_start > 0)
    {
        snippet.replace(0, std::min<std::size_t>(3, snippet.size()), "...");
    }
    if (window_start + max_chars < line.size())
    {
        std::size_t tail = snippet.size();
        snippet.replace(tail >= 3 ? tail - 3 : 0, std::min<std::size_t>(3, tail), "...");
    }
    return snippet;
}
