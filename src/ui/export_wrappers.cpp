#include "export_wrappers.h"

#include <regex>

MainImageSignature extract_main_image_signature(const std::string& golfed_source)
{
    static const std::regex kSignature(R"(void\s+mainImage\s*\(\s*out\s+vec4\s+(\w+)\s*,\s*in\s+vec2\s+(\w+)\s*\))");
    std::smatch match;
    MainImageSignature signature;
    if (std::regex_search(golfed_source, match, kSignature))
    {
        signature.found = true;
        signature.out_param = match[1].str();
        signature.coord_param = match[2].str();
    }
    return signature;
}

std::string wrap_as_shadertoy_main_image(const std::string& golfed_source)
{
    return golfed_source;
}

std::string wrap_as_bonzomatic_source(const std::string& golfed_source)
{
    return golfed_source;
}

std::string wrap_as_bare_main(const std::string& golfed_source)
{
    MainImageSignature signature = extract_main_image_signature(golfed_source);
    if (!signature.found)
    {
        return golfed_source;
    }

    std::string result;
    result += "uniform float iTime;\n";
    result += "uniform vec3 iResolution;\n";
    result += "uniform vec4 iMouse;\n";
    result += "uniform vec4 iDate;\n";
    result += "uniform int iFrame;\n";
    result += "uniform float iFrameRate;\n";
    result += golfed_source;
    result += "\nvoid main(){vec4 ";
    result += signature.out_param;
    result += ";vec2 ";
    result += signature.coord_param;
    result += "=gl_FragCoord.xy;mainImage(";
    result += signature.out_param;
    result += ",";
    result += signature.coord_param;
    result += ");gl_FragColor=";
    result += signature.out_param;
    result += ";}\n";
    return result;
}
