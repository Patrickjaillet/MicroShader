#include <cassert>
#include <cstdio>
#include <string>

#include "../src/ui/export_wrappers.h"

int main()
{
    const std::string golfed = "void mainImage(out vec4 b,in vec2 d){b=vec4(d,0.,1.);}";

    assert(wrap_as_shadertoy_main_image(golfed) == golfed);
    assert(wrap_as_bonzomatic_source(golfed) == golfed);

    MainImageSignature signature = extract_main_image_signature(golfed);
    assert(signature.found);
    assert(signature.out_param == "b");
    assert(signature.coord_param == "d");

    std::string bare = wrap_as_bare_main(golfed);
    assert(bare.find("uniform float iTime;") != std::string::npos);
    assert(bare.find("uniform vec3 iResolution;") != std::string::npos);
    assert(bare.find(golfed) != std::string::npos);
    assert(bare.find("void main(){vec4 b;vec2 d=gl_FragCoord.xy;mainImage(b,d);gl_FragColor=b;}") != std::string::npos);

    const std::string spaced = "void mainImage(out vec4 fragColor, in vec2 fragCoord)\n{\n    fragColor=vec4(fragCoord,0.0,1.0);\n}\n";
    MainImageSignature spaced_signature = extract_main_image_signature(spaced);
    assert(spaced_signature.found);
    assert(spaced_signature.out_param == "fragColor");
    assert(spaced_signature.coord_param == "fragCoord");

    const std::string no_signature = "void main(){}";
    MainImageSignature missing = extract_main_image_signature(no_signature);
    assert(!missing.found);
    assert(wrap_as_bare_main(no_signature) == no_signature);

    std::printf("export_wrappers_test: all tests passed\n");
    return 0;
}
