#include "golf_trace.h"

#include <cctype>
#include <cstdlib>

namespace
{
    std::size_t skip_ws(const std::string& text, std::size_t i)
    {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r'))
        {
            ++i;
        }
        return i;
    }

    bool parse_json_string(const std::string& text, std::size_t& i, std::string& out)
    {
        if (i >= text.size() || text[i] != '"')
        {
            return false;
        }
        ++i;
        out.clear();
        while (i < text.size() && text[i] != '"')
        {
            if (text[i] == '\\' && i + 1 < text.size())
            {
                char next = text[i + 1];
                switch (next)
                {
                    case '"':
                        out += '"';
                        break;
                    case '\\':
                        out += '\\';
                        break;
                    case '/':
                        out += '/';
                        break;
                    case 'n':
                        out += '\n';
                        break;
                    case 'r':
                        out += '\r';
                        break;
                    case 't':
                        out += '\t';
                        break;
                    case 'u':
                        if (i + 5 < text.size())
                        {
                            std::string hex = text.substr(i + 2, 4);
                            unsigned int code = static_cast<unsigned int>(std::strtoul(hex.c_str(), nullptr, 16));
                            out += static_cast<char>(code & 0xFF);
                            i += 4;
                        }
                        break;
                    default:
                        out += next;
                        break;
                }
                i += 2;
                continue;
            }
            out += text[i];
            ++i;
        }
        if (i < text.size() && text[i] == '"')
        {
            ++i;
        }
        return true;
    }

    bool parse_json_number(const std::string& text, std::size_t& i, std::size_t& out)
    {
        std::size_t start = i;
        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
        {
            ++i;
        }
        if (i == start)
        {
            return false;
        }
        out = static_cast<std::size_t>(std::strtoull(text.substr(start, i - start).c_str(), nullptr, 10));
        return true;
    }
}

std::vector<GolfTraceStep> parse_golf_trace(const std::string& json)
{
    std::vector<GolfTraceStep> steps;
    std::size_t i = skip_ws(json, 0);
    if (i >= json.size() || json[i] != '[')
    {
        return steps;
    }
    ++i;

    for (;;)
    {
        i = skip_ws(json, i);
        if (i >= json.size() || json[i] == ']')
        {
            break;
        }
        if (json[i] != '{')
        {
            break;
        }
        ++i;

        GolfTraceStep step;
        for (;;)
        {
            i = skip_ws(json, i);
            if (i >= json.size() || json[i] == '}')
            {
                break;
            }
            std::string key;
            if (!parse_json_string(json, i, key))
            {
                break;
            }
            i = skip_ws(json, i);
            if (i >= json.size() || json[i] != ':')
            {
                break;
            }
            ++i;
            i = skip_ws(json, i);

            if (key == "pass_name")
            {
                parse_json_string(json, i, step.pass_name);
            }
            else if (key == "before")
            {
                parse_json_string(json, i, step.before);
            }
            else if (key == "after")
            {
                parse_json_string(json, i, step.after);
            }
            else if (key == "count")
            {
                parse_json_number(json, i, step.count);
            }

            i = skip_ws(json, i);
            if (i < json.size() && json[i] == ',')
            {
                ++i;
                continue;
            }
            break;
        }

        i = skip_ws(json, i);
        if (i < json.size() && json[i] == '}')
        {
            ++i;
        }
        steps.push_back(std::move(step));

        i = skip_ws(json, i);
        if (i < json.size() && json[i] == ',')
        {
            ++i;
            continue;
        }
        break;
    }

    return steps;
}
