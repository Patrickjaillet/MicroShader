#include "glsl_syntax_colors.h"

#include <d2d1.h>

#include "theme_tokens.h"

D2D1_COLOR_F glsl_syntax_color(GlslTokenKind kind)
{
    switch (kind)
    {
    case GlslTokenKind::Keyword: return D2D1::ColorF(0x569CD6);
    case GlslTokenKind::BuiltinIdentifier: return D2D1::ColorF(0x4EC9B0);
    case GlslTokenKind::Number: return D2D1::ColorF(0xB5CEA8);
    case GlslTokenKind::String: return D2D1::ColorF(0xCE9178);
    case GlslTokenKind::CharLiteral: return D2D1::ColorF(0xCE9178);
    case GlslTokenKind::Comment: return D2D1::ColorF(0x6A9955);
    case GlslTokenKind::Preprocessor: return D2D1::ColorF(0xC586C0);
    case GlslTokenKind::Punctuation: return D2D1::ColorF(tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z);
    case GlslTokenKind::Identifier:
    case GlslTokenKind::Default:
    default:
        return D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z);
    }
}
