#include <cstdio>
#include <cstring>

#include "ushader/golf_core.h"

static int check(const char* label, const char* source, UshaderGolfOptions options, const char* expected)
{
    char* golfed = ushader_golf(source, options, nullptr);
    if (golfed == nullptr)
    {
        std::fprintf(stderr, "%s: ushader_golf returned null\n", label);
        return 1;
    }

    int failed = 0;
    if (std::strcmp(golfed, expected) != 0)
    {
        std::fprintf(stderr, "%s: mismatch\n  got:      %s\n  expected: %s\n", label, golfed, expected);
        failed = 1;
    }
    else
    {
        std::printf("%s: ok (%s)\n", label, golfed);
    }

    ushader_free_string(golfed);
    return failed;
}

int main()
{
    int failures = 0;

    UshaderGolfOptions none{};
    failures += check(
        "safe pipeline",
        "void mainImage(out vec4 fragColor,in vec2 fragCoord){float x=1.0;fragColor=vec4(x);}",
        none,
        "void mainImage(out vec4 a,in vec2 c){float b=1.;a=vec4(b);}");

    UshaderGolfOptions all{
        true, true, true, true, true, true, true, true, true, true, true, true, true, true
    };
    failures += check(
        "aggressive pipeline",
        "void mainImage(out vec4 fragColor,in vec2 fragCoord){float x=1.0;x=2.0;fragColor=vec4(x);}",
        all,
        "void mainImage(out vec4 b,in vec2 c){float a;a=2.;b=vec4(a);}");

    char* null_result = ushader_golf(nullptr, none, nullptr);
    if (null_result != nullptr)
    {
        std::fprintf(stderr, "null source: expected null result\n");
        failures += 1;
    }

    if (failures == 0)
    {
        std::printf("all checks passed\n");
    }

    return failures;
}
