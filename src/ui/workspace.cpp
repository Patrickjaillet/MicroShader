#include "workspace.h"

#include <cstddef>
#include <string>

#include "golf_profile.h"
#include "../platform/paths.h"

namespace
{
    std::string json_escape(const std::string& value)
    {
        std::string out;
        out.reserve(value.size());
        for (char c : value)
        {
            switch (c)
            {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    out += c;
                    break;
            }
        }
        return out;
    }

    std::string parse_string_field(const std::string& text, const std::string& key)
    {
        std::string needle = "\"" + key + "\"";
        std::size_t key_pos = text.find(needle);
        if (key_pos == std::string::npos)
        {
            return std::string();
        }
        std::size_t colon_pos = text.find(':', key_pos + needle.size());
        if (colon_pos == std::string::npos)
        {
            return std::string();
        }
        std::size_t quote_start = text.find('"', colon_pos + 1);
        if (quote_start == std::string::npos)
        {
            return std::string();
        }
        std::string out;
        std::size_t i = quote_start + 1;
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
                    case 'n':
                        out += '\n';
                        break;
                    case 'r':
                        out += '\r';
                        break;
                    case 't':
                        out += '\t';
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
        return out;
    }

    int parse_int_field(const std::string& text, const std::string& key, int default_value)
    {
        std::string needle = "\"" + key + "\"";
        std::size_t key_pos = text.find(needle);
        if (key_pos == std::string::npos)
        {
            return default_value;
        }
        std::size_t colon_pos = text.find(':', key_pos + needle.size());
        if (colon_pos == std::string::npos)
        {
            return default_value;
        }
        std::size_t value_start = colon_pos + 1;
        while (value_start < text.size() && (text[value_start] == ' ' || text[value_start] == '\t'))
        {
            ++value_start;
        }
        bool negative = false;
        if (value_start < text.size() && (text[value_start] == '-' || text[value_start] == '+'))
        {
            negative = text[value_start] == '-';
            ++value_start;
        }
        bool has_digit = false;
        int value = 0;
        while (value_start < text.size() && text[value_start] >= '0' && text[value_start] <= '9')
        {
            has_digit = true;
            value = value * 10 + (text[value_start] - '0');
            ++value_start;
        }
        if (!has_digit)
        {
            return default_value;
        }
        return negative ? -value : value;
    }

    float parse_float_field(const std::string& text, const std::string& key, float default_value)
    {
        std::string needle = "\"" + key + "\"";
        std::size_t key_pos = text.find(needle);
        if (key_pos == std::string::npos)
        {
            return default_value;
        }
        std::size_t colon_pos = text.find(':', key_pos + needle.size());
        if (colon_pos == std::string::npos)
        {
            return default_value;
        }
        std::size_t value_start = colon_pos + 1;
        while (value_start < text.size() && (text[value_start] == ' ' || text[value_start] == '\t'))
        {
            ++value_start;
        }
        std::size_t i = value_start;
        bool negative = false;
        if (i < text.size() && (text[i] == '-' || text[i] == '+'))
        {
            negative = text[i] == '-';
            ++i;
        }
        bool has_digit = false;
        double value = 0.0;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9')
        {
            has_digit = true;
            value = value * 10.0 + static_cast<double>(text[i] - '0');
            ++i;
        }
        if (i < text.size() && text[i] == '.')
        {
            ++i;
            double fraction = 0.1;
            while (i < text.size() && text[i] >= '0' && text[i] <= '9')
            {
                has_digit = true;
                value += static_cast<double>(text[i] - '0') * fraction;
                fraction *= 0.1;
                ++i;
            }
        }
        if (!has_digit)
        {
            return default_value;
        }
        return static_cast<float>(negative ? -value : value);
    }

    std::string trim_trailing_whitespace(std::string value)
    {
        while (!value.empty() && (value.back() == '\n' || value.back() == '\r' ||
                                  value.back() == ' ' || value.back() == '\t'))
        {
            value.pop_back();
        }
        return value;
    }
}

std::string serialize_workspace(const WorkspaceState& state)
{
    std::string out = "{\n";
    out += "  \"version\": 1,\n";
    out += "  \"active_tab\": " + std::to_string(state.active_tab) + ",\n";
    out += "  \"ui_font_size\": " + std::to_string(state.ui_font_size) + ",\n";
    out += "  \"layout_ini\": \"" + json_escape(state.layout_ini) + "\",\n";
    out += "  \"documents\": [";
    for (std::size_t i = 0; i < state.documents.size(); ++i)
    {
        const WorkspaceDocument& doc = state.documents[i];
        std::string profile = serialize_golf_profile(doc.pass_toggles, doc.protected_names, doc.budget_preset_index);
        std::size_t brace = profile.find('{');
        std::string doc_json;
        if (brace != std::string::npos)
        {
            doc_json = profile.substr(0, brace + 1)
                + "\n    \"path\": \"" + json_escape(doc.file_path) + "\","
                + profile.substr(brace + 1);
        }
        else
        {
            doc_json = "{ \"path\": \"" + json_escape(doc.file_path) + "\" }";
        }
        doc_json = trim_trailing_whitespace(doc_json);
        out += i == 0 ? "\n" : ",\n";
        out += doc_json;
    }
    if (!state.documents.empty())
    {
        out += "\n  ";
    }
    out += "]\n";
    out += "}\n";
    return out;
}

bool deserialize_workspace(const std::string& text, WorkspaceState& state)
{
    if (text.find('{') == std::string::npos)
    {
        return false;
    }

    state.active_tab = parse_int_field(text, "active_tab", 0);
    state.layout_ini = parse_string_field(text, "layout_ini");
    state.ui_font_size = parse_float_field(text, "ui_font_size", kDefaultBaseFontSize);
    if (!(state.ui_font_size >= kMinBaseFontSize) || !(state.ui_font_size <= kMaxBaseFontSize))
    {
        state.ui_font_size = kDefaultBaseFontSize;
    }
    state.documents.clear();

    std::size_t docs_key = text.find("\"documents\"");
    if (docs_key == std::string::npos)
    {
        return true;
    }
    std::size_t bracket = text.find('[', docs_key);
    if (bracket == std::string::npos)
    {
        return true;
    }

    int depth = 0;
    bool in_string = false;
    std::size_t obj_start = std::string::npos;
    for (std::size_t i = bracket + 1; i < text.size(); ++i)
    {
        char c = text[i];
        if (in_string)
        {
            if (c == '\\')
            {
                ++i;
                continue;
            }
            if (c == '"')
            {
                in_string = false;
            }
            continue;
        }
        if (c == '"')
        {
            in_string = true;
            continue;
        }
        if (c == ']' && depth == 0)
        {
            break;
        }
        if (c == '{')
        {
            if (depth == 0)
            {
                obj_start = i;
            }
            ++depth;
        }
        else if (c == '}')
        {
            if (depth > 0)
            {
                --depth;
            }
            if (depth == 0 && obj_start != std::string::npos)
            {
                std::string object = text.substr(obj_start, i - obj_start + 1);
                WorkspaceDocument doc;
                deserialize_golf_profile(object, doc.pass_toggles, doc.protected_names, doc.budget_preset_index);
                doc.file_path = parse_string_field(object, "path");
                state.documents.push_back(doc);
                obj_start = std::string::npos;
            }
        }
    }

    return true;
}

std::string workspace_session_path()
{
    std::string dir = app_data_dir();
    if (dir.empty())
    {
        return std::string();
    }
    return dir + "last_session.ushaderworkspace";
}

std::string workspace_layout_path()
{
    std::string dir = app_data_dir();
    if (dir.empty())
    {
        return std::string();
    }
    return dir + workspace_layout_name();
}

const char* workspace_layout_name()
{
    return "layout.ini";
}

bool workspace_session_exists()
{
    std::string path = workspace_session_path();
    if (path.empty())
    {
        return false;
    }
    return !read_utf8_file(path).empty();
}
