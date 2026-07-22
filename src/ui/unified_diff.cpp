#include "unified_diff.h"

#include <cctype>

namespace
{
    bool is_word_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
    }

    std::vector<std::string> tokenize(const std::string& text)
    {
        std::vector<std::string> tokens;
        std::size_t i = 0;
        while (i < text.size())
        {
            char c = text[i];
            if (c == '\n')
            {
                tokens.push_back("\n");
                ++i;
                continue;
            }
            if (std::isspace(static_cast<unsigned char>(c)) != 0)
            {
                std::string run;
                while (i < text.size() && text[i] != '\n' && std::isspace(static_cast<unsigned char>(text[i])) != 0)
                {
                    run += text[i];
                    ++i;
                }
                tokens.push_back(run);
                continue;
            }
            if (is_word_char(c))
            {
                std::string run;
                while (i < text.size() && is_word_char(text[i]))
                {
                    run += text[i];
                    ++i;
                }
                tokens.push_back(run);
                continue;
            }
            tokens.push_back(std::string(1, c));
            ++i;
        }
        return tokens;
    }

    std::vector<std::string> tokenize_lines(const std::string& text)
    {
        std::vector<std::string> lines;
        std::string current;
        for (char c : text)
        {
            current += c;
            if (c == '\n')
            {
                lines.push_back(current);
                current.clear();
            }
        }
        if (!current.empty())
        {
            lines.push_back(current);
        }
        return lines;
    }

    std::vector<DiffSpan> diff_tokens(const std::vector<std::string>& before, const std::vector<std::string>& after)
    {
        std::size_t n = before.size();
        std::size_t m = after.size();

        std::vector<std::vector<int>> lcs(n + 1, std::vector<int>(m + 1, 0));
        for (std::size_t i = n; i-- > 0;)
        {
            for (std::size_t j = m; j-- > 0;)
            {
                if (before[i] == after[j])
                {
                    lcs[i][j] = lcs[i + 1][j + 1] + 1;
                }
                else
                {
                    lcs[i][j] = lcs[i + 1][j] > lcs[i][j + 1] ? lcs[i + 1][j] : lcs[i][j + 1];
                }
            }
        }

        std::vector<DiffSpan> spans;
        std::size_t i = 0;
        std::size_t j = 0;
        while (i < n && j < m)
        {
            if (before[i] == after[j])
            {
                spans.push_back({before[i], DiffSpanKind::Unchanged});
                ++i;
                ++j;
            }
            else if (lcs[i + 1][j] >= lcs[i][j + 1])
            {
                spans.push_back({before[i], DiffSpanKind::Removed});
                ++i;
            }
            else
            {
                spans.push_back({after[j], DiffSpanKind::Added});
                ++j;
            }
        }
        while (i < n)
        {
            spans.push_back({before[i], DiffSpanKind::Removed});
            ++i;
        }
        while (j < m)
        {
            spans.push_back({after[j], DiffSpanKind::Added});
            ++j;
        }
        return spans;
    }
}

std::vector<DiffSpan> compute_unified_diff(const std::string& before, const std::string& after)
{
    std::vector<std::string> before_tokens = tokenize(before);
    std::vector<std::string> after_tokens = tokenize(after);

    double cell_count = static_cast<double>(before_tokens.size()) * static_cast<double>(after_tokens.size());
    if (cell_count > 2000000.0)
    {
        before_tokens = tokenize_lines(before);
        after_tokens = tokenize_lines(after);
    }

    return diff_tokens(before_tokens, after_tokens);
}
