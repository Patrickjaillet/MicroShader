#include "glsl_language.h"

#include <cctype>

TextEditor::PaletteIndex classify_glsl_token(const std::string& token)
{
    if (token.empty())
    {
        return TextEditor::PaletteIndex::Default;
    }

    char first = token[0];

    if (first == '#')
    {
        return TextEditor::PaletteIndex::Preprocessor;
    }
    if (first == '"')
    {
        return TextEditor::PaletteIndex::String;
    }
    if (first == '\'')
    {
        return TextEditor::PaletteIndex::CharLiteral;
    }
    if (token.rfind("//", 0) == 0 || token.rfind("/*", 0) == 0)
    {
        return TextEditor::PaletteIndex::Comment;
    }
    if (std::isdigit(static_cast<unsigned char>(first)) ||
        (token.size() > 1 && (first == '+' || first == '-') && std::isdigit(static_cast<unsigned char>(token[1]))))
    {
        return TextEditor::PaletteIndex::Number;
    }
    if (std::isalpha(static_cast<unsigned char>(first)) || first == '_')
    {
        const TextEditor::LanguageDefinition& language = glsl_language_definition();
        if (language.mKeywords.find(token) != language.mKeywords.end())
        {
            return TextEditor::PaletteIndex::Keyword;
        }
        if (language.mIdentifiers.find(token) != language.mIdentifiers.end())
        {
            return TextEditor::PaletteIndex::KnownIdentifier;
        }
        return TextEditor::PaletteIndex::Identifier;
    }
    return TextEditor::PaletteIndex::Punctuation;
}

const TextEditor::LanguageDefinition& glsl_language_definition()
{
    static bool inited = false;
    static TextEditor::LanguageDefinition langDef;
    if (!inited)
    {
        static const char* const keywords[] = {
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
            "iimage2D", "uimage2D", "atomic_uint"
        };
        for (auto& k : keywords)
        {
            langDef.mKeywords.insert(k);
        }

        static const char* const builtinFunctions[] = {
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
            "imageLoad", "imageStore", "imageSize", "imageAtomicAdd", "mainImage"
        };
        for (auto& k : builtinFunctions)
        {
            TextEditor::Identifier id;
            id.mDeclaration = "Built-in function";
            langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
        }

        static const char* const builtinVariables[] = {
            "gl_Position", "gl_PointSize", "gl_ClipDistance", "gl_FragCoord", "gl_FrontFacing",
            "gl_PointCoord", "gl_SampleID", "gl_SamplePosition", "gl_FragDepth",
            "gl_VertexID", "gl_InstanceID", "gl_PrimitiveID", "gl_Layer", "gl_ViewportIndex",
            "iResolution", "iTime", "iTimeDelta", "iFrame", "iFrameRate",
            "iChannelTime", "iChannelResolution", "iMouse", "iDate", "iSampleRate"
        };
        for (auto& k : builtinVariables)
        {
            TextEditor::Identifier id;
            id.mDeclaration = "Built-in variable";
            langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
        }

        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", TextEditor::PaletteIndex::Preprocessor));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("\\'\\\\?[^\\']\\'", TextEditor::PaletteIndex::CharLiteral));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", TextEditor::PaletteIndex::Number));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", TextEditor::PaletteIndex::Identifier));
        langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", TextEditor::PaletteIndex::Punctuation));

        langDef.mCommentStart = "/*";
        langDef.mCommentEnd = "*/";
        langDef.mSingleLineComment = "//";

        langDef.mCaseSensitive = true;
        langDef.mAutoIndentation = true;
        langDef.mName = "GLSL";

        inited = true;
    }
    return langDef;
}
