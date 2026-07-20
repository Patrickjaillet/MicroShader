#include "exclude_list_import.h"

#include <optional>
#include <sstream>
#include <vector>

#include "../platform/file_dialog.h"
#include "../platform/paths.h"

namespace
{
    const wchar_t kExcludeListFilter[] = L"Exclude-name list (*.txt)\0*.txt\0All files (*.*)\0*.*\0";

    std::string trim(const std::string& text)
    {
        std::size_t begin = text.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
        {
            return "";
        }
        std::size_t end = text.find_last_not_of(" \t\r\n");
        return text.substr(begin, end - begin + 1);
    }

    std::vector<std::string> split_csv(const std::string& csv)
    {
        std::vector<std::string> names;
        std::stringstream stream(csv);
        std::string entry;
        while (std::getline(stream, entry, ','))
        {
            std::string trimmed = trim(entry);
            if (!trimmed.empty())
            {
                names.push_back(trimmed);
            }
        }
        return names;
    }
}

std::string parse_exclude_name_list(const std::string& list_text)
{
    std::vector<std::string> names;
    std::stringstream stream(list_text);
    std::string line;
    while (std::getline(stream, line))
    {
        std::string trimmed = trim(line);
        if (trimmed.empty())
        {
            continue;
        }
        if (trimmed.rfind("//", 0) == 0 || trimmed.rfind('#', 0) == 0)
        {
            continue;
        }
        names.push_back(trimmed);
    }

    std::string joined;
    for (const std::string& name : names)
    {
        if (!joined.empty())
        {
            joined += ", ";
        }
        joined += name;
    }
    return joined;
}

std::string merge_protected_names(const std::string& existing_csv, const std::string& imported_csv)
{
    std::vector<std::string> merged = split_csv(existing_csv);
    std::vector<std::string> imported = split_csv(imported_csv);

    for (const std::string& name : imported)
    {
        bool already_present = false;
        for (const std::string& existing_name : merged)
        {
            if (existing_name == name)
            {
                already_present = true;
                break;
            }
        }
        if (!already_present)
        {
            merged.push_back(name);
        }
    }

    std::string joined;
    for (const std::string& name : merged)
    {
        if (!joined.empty())
        {
            joined += ", ";
        }
        joined += name;
    }
    return joined;
}

void import_exclude_list_action(std::string& protected_names, SDL_Window* window)
{
    std::optional<std::string> path = show_open_file_dialog(window, kExcludeListFilter, L"txt");
    if (path.has_value())
    {
        std::string imported = parse_exclude_name_list(read_utf8_file(*path));
        protected_names = merge_protected_names(protected_names, imported);
    }
}
