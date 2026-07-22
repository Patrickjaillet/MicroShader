#pragma once

#include <string>
#include <unordered_set>
#include <vector>

enum class GlslTokenKind
{
    Default,
    Keyword,
    BuiltinIdentifier,
    Identifier,
    Number,
    String,
    CharLiteral,
    Comment,
    Preprocessor,
    Punctuation,
};

const std::unordered_set<std::string>& glsl_keywords();
const std::unordered_set<std::string>& glsl_builtin_functions();
const std::unordered_set<std::string>& glsl_builtin_variables();
const std::unordered_set<std::string>& glsl_builtin_identifiers();
GlslTokenKind classify_glsl_token_kind(const std::string& token);

struct GlslTokenSpan
{
    int start = 0;
    int length = 0;
    GlslTokenKind kind = GlslTokenKind::Default;
};

std::vector<GlslTokenSpan> tokenize_glsl_line(const std::wstring& line, bool& in_block_comment);
