#pragma once

inline constexpr const char* kDefaultShaderSource =
    "void mainImage(out vec4 fragColor, in vec2 fragCoord)\n"
    "{\n"
    "    vec2 uv = fragCoord / iResolution.xy;\n"
    "    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx + vec3(0.0, 2.0, 4.0));\n"
    "    fragColor = vec4(col, 1.0);\n"
    "}\n";
