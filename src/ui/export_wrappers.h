#pragma once

#include <string>

struct MainImageSignature
{
    bool found = false;
    std::string out_param;
    std::string coord_param;
};

MainImageSignature extract_main_image_signature(const std::string& golfed_source);

std::string wrap_as_shadertoy_main_image(const std::string& golfed_source);
std::string wrap_as_bonzomatic_source(const std::string& golfed_source);
std::string wrap_as_bare_main(const std::string& golfed_source);
