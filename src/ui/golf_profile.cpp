#include "golf_profile.h"

#include <cstddef>
#include <string>

#include "budget_presets.h"
#include "../platform/paths.h"

namespace
{
    constexpr int kProfileSchemaVersion = 1;

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

    void append_bool_field(std::string& out, const char* key, bool value, bool trailing_comma)
    {
        out += "  \"";
        out += key;
        out += "\": ";
        out += value ? "true" : "false";
        if (trailing_comma)
        {
            out += ",";
        }
        out += "\n";
    }

    void append_int_field(std::string& out, const char* key, int value, bool trailing_comma)
    {
        out += "  \"";
        out += key;
        out += "\": ";
        out += std::to_string(value);
        if (trailing_comma)
        {
            out += ",";
        }
        out += "\n";
    }

    std::string find_field_slice(const std::string& text, const std::string& key)
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
        std::size_t value_start = colon_pos + 1;
        while (value_start < text.size() && (text[value_start] == ' ' || text[value_start] == '\t'))
        {
            ++value_start;
        }
        std::size_t value_end = text.find_first_of(",}\n", value_start);
        if (value_end == std::string::npos)
        {
            value_end = text.size();
        }
        return text.substr(value_start, value_end - value_start);
    }

    bool find_bool_field(const std::string& text, const std::string& key, bool default_value)
    {
        std::string slice = find_field_slice(text, key);
        if (slice.find("true") != std::string::npos)
        {
            return true;
        }
        if (slice.find("false") != std::string::npos)
        {
            return false;
        }
        return default_value;
    }

    std::string find_string_field(const std::string& text, const std::string& key)
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

    std::string budget_preset_name(int budget_preset_index)
    {
        std::size_t preset_count = 0;
        const BudgetPreset* presets = budget_presets(preset_count);
        if (budget_preset_index >= 0 && static_cast<std::size_t>(budget_preset_index) < preset_count)
        {
            return presets[budget_preset_index].name;
        }
        return std::string();
    }

    int budget_preset_index_from_name(const std::string& name)
    {
        if (name.empty())
        {
            return -1;
        }
        std::size_t preset_count = 0;
        const BudgetPreset* presets = budget_presets(preset_count);
        for (std::size_t i = 0; i < preset_count; ++i)
        {
            if (name == presets[i].name)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    bool toggles_equal(const GolfPassToggles& a, const GolfPassToggles& b)
    {
        return a.aggressive == b.aggressive
            && a.eliminate_dead_locals == b.eliminate_dead_locals
            && a.eliminate_dead_stores == b.eliminate_dead_stores
            && a.fold_constants == b.fold_constants
            && a.reduce_constant_vectors == b.reduce_constant_vectors
            && a.strip_trailing_void_return == b.strip_trailing_void_return
            && a.compound_assignments == b.compound_assignments
            && a.increment_decrement == b.increment_decrement
            && a.ternary_from_if_else == b.ternary_from_if_else
            && a.merge_declarations == b.merge_declarations
            && a.strip_redundant_braces == b.strip_redundant_braces
            && a.strip_redundant_parens == b.strip_redundant_parens
            && a.strip_duplicate_precision == b.strip_duplicate_precision
            && a.eliminate_dead_functions == b.eliminate_dead_functions
            && a.inline_single_call_functions == b.inline_single_call_functions
            && a.simplify_algebraic_identities == b.simplify_algebraic_identities
            && a.eliminate_common_subexpressions == b.eliminate_common_subexpressions;
    }

    std::string last_profile_path_file()
    {
        std::string dir = app_data_dir();
        if (dir.empty())
        {
            return std::string();
        }
        return dir + "last_profile_path.txt";
    }
}

std::string serialize_golf_profile(const GolfPassToggles& toggles, const std::string& protected_names, int budget_preset_index)
{
    std::string out = "{\n";
    append_int_field(out, "schema_version", kProfileSchemaVersion, true);
    append_bool_field(out, "aggressive", toggles.aggressive, true);
    append_bool_field(out, "eliminate_dead_locals", toggles.eliminate_dead_locals, true);
    append_bool_field(out, "eliminate_dead_stores", toggles.eliminate_dead_stores, true);
    append_bool_field(out, "fold_constants", toggles.fold_constants, true);
    append_bool_field(out, "reduce_constant_vectors", toggles.reduce_constant_vectors, true);
    append_bool_field(out, "strip_trailing_void_return", toggles.strip_trailing_void_return, true);
    append_bool_field(out, "compound_assignments", toggles.compound_assignments, true);
    append_bool_field(out, "increment_decrement", toggles.increment_decrement, true);
    append_bool_field(out, "ternary_from_if_else", toggles.ternary_from_if_else, true);
    append_bool_field(out, "merge_declarations", toggles.merge_declarations, true);
    append_bool_field(out, "strip_redundant_braces", toggles.strip_redundant_braces, true);
    append_bool_field(out, "strip_redundant_parens", toggles.strip_redundant_parens, true);
    append_bool_field(out, "strip_duplicate_precision", toggles.strip_duplicate_precision, true);
    append_bool_field(out, "eliminate_dead_functions", toggles.eliminate_dead_functions, true);
    append_bool_field(out, "inline_single_call_functions", toggles.inline_single_call_functions, true);
    append_bool_field(out, "simplify_algebraic_identities", toggles.simplify_algebraic_identities, true);
    append_bool_field(out, "eliminate_common_subexpressions", toggles.eliminate_common_subexpressions, true);
    out += "  \"protected_names\": \"" + json_escape(protected_names) + "\",\n";
    out += "  \"budget_preset\": \"" + json_escape(budget_preset_name(budget_preset_index)) + "\"\n";
    out += "}\n";
    return out;
}

bool deserialize_golf_profile(const std::string& text, GolfPassToggles& toggles, std::string& protected_names, int& budget_preset_index)
{
    if (text.find('{') == std::string::npos)
    {
        return false;
    }
    GolfPassToggles parsed;
    parsed.aggressive = find_bool_field(text, "aggressive", parsed.aggressive);
    parsed.eliminate_dead_locals = find_bool_field(text, "eliminate_dead_locals", parsed.eliminate_dead_locals);
    parsed.eliminate_dead_stores = find_bool_field(text, "eliminate_dead_stores", parsed.eliminate_dead_stores);
    parsed.fold_constants = find_bool_field(text, "fold_constants", parsed.fold_constants);
    parsed.reduce_constant_vectors = find_bool_field(text, "reduce_constant_vectors", parsed.reduce_constant_vectors);
    parsed.strip_trailing_void_return = find_bool_field(text, "strip_trailing_void_return", parsed.strip_trailing_void_return);
    parsed.compound_assignments = find_bool_field(text, "compound_assignments", parsed.compound_assignments);
    parsed.increment_decrement = find_bool_field(text, "increment_decrement", parsed.increment_decrement);
    parsed.ternary_from_if_else = find_bool_field(text, "ternary_from_if_else", parsed.ternary_from_if_else);
    parsed.merge_declarations = find_bool_field(text, "merge_declarations", parsed.merge_declarations);
    parsed.strip_redundant_braces = find_bool_field(text, "strip_redundant_braces", parsed.strip_redundant_braces);
    parsed.strip_redundant_parens = find_bool_field(text, "strip_redundant_parens", parsed.strip_redundant_parens);
    parsed.strip_duplicate_precision = find_bool_field(text, "strip_duplicate_precision", parsed.strip_duplicate_precision);
    parsed.eliminate_dead_functions = find_bool_field(text, "eliminate_dead_functions", parsed.eliminate_dead_functions);
    parsed.inline_single_call_functions = find_bool_field(text, "inline_single_call_functions", parsed.inline_single_call_functions);
    parsed.simplify_algebraic_identities = find_bool_field(text, "simplify_algebraic_identities", parsed.simplify_algebraic_identities);
    parsed.eliminate_common_subexpressions = find_bool_field(text, "eliminate_common_subexpressions", parsed.eliminate_common_subexpressions);

    toggles = parsed;
    protected_names = find_string_field(text, "protected_names");
    budget_preset_index = budget_preset_index_from_name(find_string_field(text, "budget_preset"));
    return true;
}

GolfPassToggles builtin_profile_maximum()
{
    return GolfPassToggles{};
}

GolfPassToggles builtin_profile_safe()
{
    GolfPassToggles toggles{};
    toggles.aggressive = true;
    toggles.eliminate_dead_locals = true;
    toggles.eliminate_dead_stores = true;
    toggles.fold_constants = false;
    toggles.reduce_constant_vectors = false;
    toggles.strip_trailing_void_return = false;
    toggles.compound_assignments = false;
    toggles.increment_decrement = false;
    toggles.ternary_from_if_else = false;
    toggles.merge_declarations = false;
    toggles.strip_redundant_braces = false;
    toggles.strip_redundant_parens = false;
    toggles.strip_duplicate_precision = false;
    toggles.eliminate_dead_functions = true;
    toggles.inline_single_call_functions = false;
    toggles.simplify_algebraic_identities = false;
    toggles.eliminate_common_subexpressions = false;
    return toggles;
}

GolfPassToggles builtin_profile_none()
{
    GolfPassToggles toggles{};
    toggles.aggressive = false;
    toggles.eliminate_dead_locals = false;
    toggles.eliminate_dead_stores = false;
    toggles.fold_constants = false;
    toggles.reduce_constant_vectors = false;
    toggles.strip_trailing_void_return = false;
    toggles.compound_assignments = false;
    toggles.increment_decrement = false;
    toggles.ternary_from_if_else = false;
    toggles.merge_declarations = false;
    toggles.strip_redundant_braces = false;
    toggles.strip_redundant_parens = false;
    toggles.strip_duplicate_precision = false;
    toggles.eliminate_dead_functions = false;
    toggles.inline_single_call_functions = false;
    toggles.simplify_algebraic_identities = false;
    toggles.eliminate_common_subexpressions = false;
    return toggles;
}

const char* builtin_profile_name(const GolfPassToggles& toggles)
{
    if (toggles_equal(toggles, builtin_profile_maximum()))
    {
        return "Maximum";
    }
    if (toggles_equal(toggles, builtin_profile_safe()))
    {
        return "Safe";
    }
    if (toggles_equal(toggles, builtin_profile_none()))
    {
        return "None";
    }
    return "Custom";
}

std::string load_last_profile_path()
{
    std::string file_path = last_profile_path_file();
    if (file_path.empty())
    {
        return std::string();
    }
    std::string content = read_utf8_file(file_path);
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r'))
    {
        content.pop_back();
    }
    return content;
}

void save_last_profile_path(const std::string& path)
{
    std::string file_path = last_profile_path_file();
    if (file_path.empty())
    {
        return;
    }
    write_utf8_file(file_path, path);
}
