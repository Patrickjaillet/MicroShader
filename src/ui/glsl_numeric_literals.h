#pragma once

#include <cstddef>
#include <string>
#include <vector>

// A single GLSL floating-point literal found in source text (a digit run
// containing a '.' and/or an exponent -- bare integers like loop bounds,
// array sizes, or swizzle/member-access indices are never included).
struct GlslNumericLiteral
{
    std::size_t byte_offset = 0;
    std::size_t byte_length = 0;
    double value = 0.0;
};

struct SliderRange
{
    double min_value = 0.0;
    double max_value = 1.0;
};

// Scans `source` in source order, skipping `//` and `/* */` comments (so
// numbers in commented-out code never grow a slider) and any digit run that
// is part of a longer identifier (e.g. the "3" in "vec3" or "mat2x2").
std::vector<GlslNumericLiteral> find_glsl_float_literals(const std::string& source);

// Formats `value` the way find_glsl_float_literals would recognize it as a
// single float literal: always includes a decimal point, and trims
// trailing zeros while keeping at least one digit after the point.
std::string format_glsl_float_literal(double value);

// A slider range scaled to a value's own magnitude: 0 maps to [-1, 1]; a
// positive value maps to [0, 2x itself]; a negative value maps to a
// symmetric [-2|x|, 2|x|] so the slider can still cross zero.
SliderRange compute_slider_range(double base_value);

// Replaces source[offset, offset + length) with `replacement`.
std::string splice_source(const std::string& source, std::size_t offset, std::size_t length,
    const std::string& replacement);

// A short, single-line, human-readable snippet of `source` around
// [offset, offset + length) -- the literal's own line, trimmed of leading
// whitespace and truncated (with a leading/trailing "..." as needed) to at
// most `max_chars` characters. Used as a slider row's label so otherwise-
// identical values (many shaders have several "1.0" literals) stay
// distinguishable.
std::string literal_context_snippet(const std::string& source, std::size_t offset, std::size_t length,
    std::size_t max_chars);
