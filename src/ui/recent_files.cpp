#include "recent_files.h"

#include <cstddef>

#include "../platform/paths.h"

namespace
{
    const std::size_t kMaxRecentFiles = 10;

    std::string recent_files_path()
    {
        std::string dir = app_data_dir();
        if (dir.empty())
        {
            return std::string();
        }
        return dir + "recent_files.json";
    }

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

    std::vector<std::string> parse_string_array(const std::string& text)
    {
        std::vector<std::string> out;
        std::size_t bracket = text.find('[');
        if (bracket == std::string::npos)
        {
            return out;
        }
        bool in_string = false;
        std::string current;
        for (std::size_t i = bracket + 1; i < text.size(); ++i)
        {
            char c = text[i];
            if (in_string)
            {
                if (c == '\\' && i + 1 < text.size())
                {
                    char next = text[i + 1];
                    switch (next)
                    {
                        case '"':
                            current += '"';
                            break;
                        case '\\':
                            current += '\\';
                            break;
                        case 'n':
                            current += '\n';
                            break;
                        case 'r':
                            current += '\r';
                            break;
                        case 't':
                            current += '\t';
                            break;
                        default:
                            current += next;
                            break;
                    }
                    ++i;
                    continue;
                }
                if (c == '"')
                {
                    in_string = false;
                    out.push_back(current);
                    current.clear();
                    continue;
                }
                current += c;
                continue;
            }
            if (c == '"')
            {
                in_string = true;
                continue;
            }
            if (c == ']')
            {
                break;
            }
        }
        return out;
    }

    void save_recent_files(const std::vector<std::string>& files)
    {
        std::string file_path = recent_files_path();
        if (file_path.empty())
        {
            return;
        }
        std::string out = "{\n  \"files\": [";
        for (std::size_t i = 0; i < files.size(); ++i)
        {
            out += i == 0 ? "\n    \"" : ",\n    \"";
            out += json_escape(files[i]);
            out += "\"";
        }
        if (!files.empty())
        {
            out += "\n  ";
        }
        out += "]\n}\n";
        write_utf8_file(file_path, out);
    }
}

std::vector<std::string> load_recent_files()
{
    std::string file_path = recent_files_path();
    if (file_path.empty())
    {
        return std::vector<std::string>();
    }
    std::string text = read_utf8_file(file_path);
    if (text.find('[') == std::string::npos)
    {
        return std::vector<std::string>();
    }
    return parse_string_array(text);
}

void add_recent_file(const std::string& file_path)
{
    if (file_path.empty())
    {
        return;
    }
    std::vector<std::string> files = load_recent_files();
    for (std::size_t i = 0; i < files.size(); ++i)
    {
        if (files[i] == file_path)
        {
            files.erase(files.begin() + i);
            break;
        }
    }
    files.insert(files.begin(), file_path);
    if (files.size() > kMaxRecentFiles)
    {
        files.resize(kMaxRecentFiles);
    }
    save_recent_files(files);
}

void remove_recent_file(const std::string& file_path)
{
    std::vector<std::string> files = load_recent_files();
    for (std::size_t i = 0; i < files.size(); ++i)
    {
        if (files[i] == file_path)
        {
            files.erase(files.begin() + i);
            break;
        }
    }
    save_recent_files(files);
}

void clear_recent_files()
{
    save_recent_files(std::vector<std::string>());
}
