#include "fuzzy_match.h"

#include <cctype>

bool fuzzy_match(const std::string& query, const std::string& label)
{
    if (query.empty())
    {
        return true;
    }

    std::size_t label_pos = 0;
    for (char query_char : query)
    {
        char query_lower = static_cast<char>(std::tolower(static_cast<unsigned char>(query_char)));
        bool found = false;
        while (label_pos < label.size())
        {
            char label_lower = static_cast<char>(std::tolower(static_cast<unsigned char>(label[label_pos])));
            ++label_pos;
            if (label_lower == query_lower)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            return false;
        }
    }
    return true;
}
