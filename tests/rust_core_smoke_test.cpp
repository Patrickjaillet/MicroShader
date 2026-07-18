#include <cstdio>
#include <cstring>

#include "ushader/golf_core.h"

static int check(const char* label, const char* source, UshaderGolfOptions options, const char* expected)
{
    UshaderGolfStats stats{};
    char* golfed = ushader_golf(source, options, nullptr, &stats);
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
    else if (stats.output_chars != std::strlen(golfed))
    {
        std::fprintf(stderr, "%s: stats.output_chars (%zu) does not match golfed length (%zu)\n", label, stats.output_chars, std::strlen(golfed));
        failed = 1;
    }
    else
    {
        std::printf("%s: ok (%s) [renamed=%zu numbers_shortened=%zu reduction=%.1f%%]\n", label, golfed, stats.renamed_count, stats.numbers_shortened, stats.reduction_pct);
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

    UshaderGolfStats dead_store_stats{};
    char* dead_store_golfed = ushader_golf(
        "void mainImage(out vec4 fragColor,in vec2 fragCoord){float x=1.0;x=2.0;fragColor=vec4(x);}",
        all, nullptr, &dead_store_stats);
    if (dead_store_golfed == nullptr || dead_store_stats.dead_stores_removed != 1)
    {
        std::fprintf(stderr, "aggressive pipeline: expected dead_stores_removed == 1, got %zu\n", dead_store_stats.dead_stores_removed);
        failures += 1;
    }
    ushader_free_string(dead_store_golfed);

    char* null_result = ushader_golf(nullptr, none, nullptr, nullptr);
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
