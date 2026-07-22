#include "glsl_token_rules.h"

#include <cctype>

const std::unordered_set<std::string>& glsl_keywords()
{
    static const std::unordered_set<std::string> keywords = {
        "if", "else", "for", "while", "do", "break", "continue", "return", "discard",
        "switch", "case", "default", "true", "false", "struct",
        "in", "out", "inout", "uniform", "const", "varying", "attribute", "layout",
        "flat", "smooth", "noperspective", "centroid", "invariant", "precise",
        "coherent", "volatile", "restrict", "readonly", "writeonly", "buffer",
        "shared", "patch", "sample", "precision", "highp", "mediump", "lowp",
        "void", "bool", "int", "uint", "float", "double",
        "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "uvec2", "uvec3", "uvec4",
        "bvec2", "bvec3", "bvec4", "dvec2", "dvec3", "dvec4",
        "mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4",
        "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4",
        "sampler1D", "sampler2D", "sampler3D", "samplerCube",
        "sampler1DShadow", "sampler2DShadow", "samplerCubeShadow",
        "sampler1DArray", "sampler2DArray", "sampler1DArrayShadow", "sampler2DArrayShadow",
        "samplerCubeArray", "samplerCubeArrayShadow",
        "isampler1D", "isampler2D", "isampler3D", "isamplerCube",
        "isampler1DArray", "isampler2DArray",
        "usampler1D", "usampler2D", "usampler3D", "usamplerCube",
        "usampler1DArray", "usampler2DArray",
        "sampler2DMS", "isampler2DMS", "usampler2DMS",
        "sampler2DMSArray", "isampler2DMSArray", "usampler2DMSArray",
        "samplerBuffer", "isamplerBuffer", "usamplerBuffer",
        "sampler2DRect", "sampler2DRectShadow",
        "image1D", "image2D", "image3D", "imageCube", "image2DArray",
        "iimage2D", "uimage2D", "atomic_uint",
    };
    return keywords;
}

const std::unordered_set<std::string>& glsl_builtin_functions()
{
    static const std::unordered_set<std::string> functions = {
        "radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan",
        "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
        "pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt",
        "abs", "sign", "floor", "trunc", "round", "roundEven", "ceil", "fract", "mod", "modf",
        "min", "max", "clamp", "mix", "step", "smoothstep", "isnan", "isinf",
        "floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat",
        "fma", "frexp", "ldexp",
        "length", "distance", "dot", "cross", "normalize", "faceforward", "reflect", "refract",
        "matrixCompMult", "outerProduct", "transpose", "determinant", "inverse",
        "lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual", "equal", "notEqual",
        "any", "all", "not",
        "texture", "textureProj", "textureLod", "textureOffset", "texelFetch", "texelFetchOffset",
        "textureProjLod", "textureLodOffset", "textureProjLodOffset",
        "textureGrad", "textureGradOffset", "textureProjGrad", "textureProjGradOffset",
        "textureGather", "textureGatherOffset", "texture2D", "textureCube",
        "dFdx", "dFdy", "fwidth", "dFdxCoarse", "dFdyCoarse", "dFdxFine", "dFdyFine",
        "fwidthCoarse", "fwidthFine",
        "noise1", "noise2", "noise3", "noise4",
        "EmitVertex", "EndPrimitive", "barrier", "memoryBarrier", "groupMemoryBarrier",
        "imageLoad", "imageStore", "imageSize", "imageAtomicAdd", "mainImage",
    };
    return functions;
}

const std::unordered_set<std::string>& glsl_builtin_variables()
{
    static const std::unordered_set<std::string> variables = {
        "gl_Position", "gl_PointSize", "gl_ClipDistance", "gl_FragCoord", "gl_FrontFacing",
        "gl_PointCoord", "gl_SampleID", "gl_SamplePosition", "gl_FragDepth",
        "gl_VertexID", "gl_InstanceID", "gl_PrimitiveID", "gl_Layer", "gl_ViewportIndex",
        "iResolution", "iTime", "iTimeDelta", "iFrame", "iFrameRate",
        "iChannelTime", "iChannelResolution", "iMouse", "iDate", "iSampleRate",
    };
    return variables;
}

const std::unordered_set<std::string>& glsl_builtin_identifiers()
{
    static const std::unordered_set<std::string> builtins = [] {
        std::unordered_set<std::string> combined = glsl_builtin_functions();
        combined.insert(glsl_builtin_variables().begin(), glsl_builtin_variables().end());
        return combined;
    }();
    return builtins;
}

GlslTokenKind classify_glsl_token_kind(const std::string& token)
{
    if (token.empty())
    {
        return GlslTokenKind::Default;
    }

    char first = token[0];

    if (first == '#')
    {
        return GlslTokenKind::Preprocessor;
    }
    if (first == '"')
    {
        return GlslTokenKind::String;
    }
    if (first == '\'')
    {
        return GlslTokenKind::CharLiteral;
    }
    if (token.rfind("//", 0) == 0 || token.rfind("/*", 0) == 0)
    {
        return GlslTokenKind::Comment;
    }
    if (std::isdigit(static_cast<unsigned char>(first)) ||
        (token.size() > 1 && (first == '+' || first == '-') && std::isdigit(static_cast<unsigned char>(token[1]))))
    {
        return GlslTokenKind::Number;
    }
    if (std::isalpha(static_cast<unsigned char>(first)) || first == '_')
    {
        if (glsl_keywords().find(token) != glsl_keywords().end())
        {
            return GlslTokenKind::Keyword;
        }
        if (glsl_builtin_identifiers().find(token) != glsl_builtin_identifiers().end())
        {
            return GlslTokenKind::BuiltinIdentifier;
        }
        return GlslTokenKind::Identifier;
    }
    return GlslTokenKind::Punctuation;
}

namespace
{
    bool is_ident_start(wchar_t c)
    {
        return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || c == L'_';
    }

    bool is_ident_char(wchar_t c)
    {
        return is_ident_start(c) || (c >= L'0' && c <= L'9');
    }

    bool is_digit(wchar_t c)
    {
        return c >= L'0' && c <= L'9';
    }

    std::string narrow_ascii(const std::wstring& text, int start, int length)
    {
        std::string out;
        out.reserve(static_cast<size_t>(length));
        for (int i = 0; i < length; ++i)
        {
            wchar_t c = text[static_cast<size_t>(start + i)];
            out.push_back(c < 128 ? static_cast<char>(c) : '_');
        }
        return out;
    }
}

std::vector<GlslTokenSpan> tokenize_glsl_line(const std::wstring& line, bool& in_block_comment)
{
    std::vector<GlslTokenSpan> spans;
    int i = 0;
    int n = static_cast<int>(line.size());

    if (in_block_comment)
    {
        int start = 0;
        while (i < n)
        {
            if (line[i] == L'*' && i + 1 < n && line[i + 1] == L'/')
            {
                i += 2;
                in_block_comment = false;
                break;
            }
            ++i;
        }
        spans.push_back(GlslTokenSpan{ start, i - start, GlslTokenKind::Comment });
        if (in_block_comment)
        {
            return spans;
        }
    }

    while (i < n)
    {
        wchar_t c = line[i];

        if (c == L' ' || c == L'\t')
        {
            ++i;
            continue;
        }

        if (c == L'/' && i + 1 < n && line[i + 1] == L'/')
        {
            spans.push_back(GlslTokenSpan{ i, n - i, GlslTokenKind::Comment });
            break;
        }

        if (c == L'/' && i + 1 < n && line[i + 1] == L'*')
        {
            int start = i;
            i += 2;
            in_block_comment = true;
            while (i < n)
            {
                if (line[i] == L'*' && i + 1 < n && line[i + 1] == L'/')
                {
                    i += 2;
                    in_block_comment = false;
                    break;
                }
                ++i;
            }
            spans.push_back(GlslTokenSpan{ start, i - start, GlslTokenKind::Comment });
            continue;
        }

        if (c == L'#')
        {
            int start = i;
            ++i;
            while (i < n && (line[i] == L' ' || line[i] == L'\t'))
            {
                ++i;
            }
            while (i < n && is_ident_char(line[i]))
            {
                ++i;
            }
            spans.push_back(GlslTokenSpan{ start, i - start, GlslTokenKind::Preprocessor });
            continue;
        }

        if (c == L'"')
        {
            int start = i;
            ++i;
            while (i < n && line[i] != L'"')
            {
                if (line[i] == L'\\' && i + 1 < n)
                {
                    ++i;
                }
                ++i;
            }
            if (i < n)
            {
                ++i;
            }
            spans.push_back(GlslTokenSpan{ start, i - start, GlslTokenKind::String });
            continue;
        }

        if (c == L'\'')
        {
            int start = i;
            ++i;
            while (i < n && line[i] != L'\'')
            {
                if (line[i] == L'\\' && i + 1 < n)
                {
                    ++i;
                }
                ++i;
            }
            if (i < n)
            {
                ++i;
            }
            spans.push_back(GlslTokenSpan{ start, i - start, GlslTokenKind::CharLiteral });
            continue;
        }

        if (is_digit(c) || (c == L'.' && i + 1 < n && is_digit(line[i + 1])))
        {
            int start = i;
            while (i < n && (is_ident_char(line[i]) || line[i] == L'.'))
            {
                ++i;
            }
            spans.push_back(GlslTokenSpan{ start, i - start, GlslTokenKind::Number });
            continue;
        }

        if (is_ident_start(c))
        {
            int start = i;
            while (i < n && is_ident_char(line[i]))
            {
                ++i;
            }
            std::string token = narrow_ascii(line, start, i - start);
            spans.push_back(GlslTokenSpan{ start, i - start, classify_glsl_token_kind(token) });
            continue;
        }

        spans.push_back(GlslTokenSpan{ i, 1, GlslTokenKind::Punctuation });
        ++i;
    }

    return spans;
}
