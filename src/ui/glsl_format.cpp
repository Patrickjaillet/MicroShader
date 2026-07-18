#include "glsl_format.h"

namespace
{
    void append_indent(std::string& out, int depth)
    {
        for (int i = 0; i < depth; ++i)
        {
            out += "    ";
        }
    }

    void trim_trailing_spaces(std::string& out)
    {
        size_t end = out.size();
        while (end > 0 && out[end - 1] == ' ')
        {
            end -= 1;
        }
        out.erase(end);
    }
}

std::string format_glsl(const std::string& source)
{
    std::string result;
    int depth = 0;
    int paren_depth = 0;

    for (size_t i = 0; i < source.size(); ++i)
    {
        char c = source[i];

        if (c == '(')
        {
            paren_depth += 1;
            result += c;
            continue;
        }

        if (c == ')')
        {
            paren_depth = paren_depth > 0 ? paren_depth - 1 : 0;
            result += c;
            continue;
        }

        if (c == '{')
        {
            result += c;
            depth += 1;
            result += '\n';
            append_indent(result, depth);
            continue;
        }

        if (c == '}')
        {
            trim_trailing_spaces(result);
            if (!result.empty() && result.back() != '\n')
            {
                result += '\n';
            }
            depth = depth > 0 ? depth - 1 : 0;
            append_indent(result, depth);
            result += c;
            if (i + 1 < source.size() && source[i + 1] == ';')
            {
                result += ';';
                i += 1;
            }
            result += '\n';
            append_indent(result, depth);
            continue;
        }

        if (c == ';' && paren_depth == 0)
        {
            result += c;
            result += '\n';
            append_indent(result, depth);
            continue;
        }

        result += c;
    }

    trim_trailing_spaces(result);
    while (!result.empty() && result.back() == '\n')
    {
        result.pop_back();
    }
    return result;
}
