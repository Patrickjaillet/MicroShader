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
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false
    };
    failures += check(
        "aggressive pipeline",
        "void mainImage(out vec4 fragColor,in vec2 fragCoord){float x=1.0;x=2.0;fragColor=vec4(x);}",
        all,
        "void mainImage(out vec4 b,in vec2 c){float a;a=2.;b=vec4(a);}");

    UshaderGolfOptions freq_renaming = all;
    freq_renaming.frequency_aware_renaming = true;
    failures += check(
        "frequency-aware renaming",
        "void mainImage(out vec4 fragColor,in vec2 fragCoord){float floorField=floor(fragCoord.x)+floor(fragCoord.y);float fractField=fract(fragCoord.x)+fract(fragCoord.y);float finalField=floorField+fractField+floor(floorField)+fract(fractField);float filterField=finalField+floorField+fractField;fragColor=vec4(floorField+fractField+finalField+filterField);}",
        freq_renaming,
        "void mainImage(out vec4 e,in vec2 a){float b=floor(a.x)+floor(a.y),c=fract(a.x)+fract(a.y),d=b+c+floor(b)+fract(c),f=d+b+c;e=vec4(b+c+d+f);}");

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

    {
        const char* harder_source =
            "float f(vec2 p){return dot(p,p)+dot(p,p)+dot(p,p);}"
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(f(fragCoord));}";
        UshaderGolfStats harder_stats{};
        bool improved = false;
        char* applied_json = nullptr;
        char* harder_golfed = ushader_golf_harder(harder_source, all, nullptr, false, &harder_stats, &improved, &applied_json);
        if (harder_golfed == nullptr)
        {
            std::fprintf(stderr, "golf harder: ushader_golf_harder returned null\n");
            failures += 1;
        }
        else if (harder_stats.output_chars != std::strlen(harder_golfed))
        {
            std::fprintf(stderr, "golf harder: stats.output_chars (%zu) does not match golfed length (%zu)\n", harder_stats.output_chars, std::strlen(harder_golfed));
            failures += 1;
        }
        else if (applied_json == nullptr || applied_json[0] != '[')
        {
            std::fprintf(stderr, "golf harder: expected a JSON array in out_applied_json\n");
            failures += 1;
        }
        else
        {
            std::printf("golf harder: ok (%s) [improved=%d applied=%s]\n", harder_golfed, improved ? 1 : 0, applied_json);
        }
        if (harder_golfed != nullptr) { ushader_free_string(harder_golfed); }
        if (applied_json != nullptr) { ushader_free_string(applied_json); }
    }

    {
        const char* deep_source =
            "float f(vec2 p){return dot(p,p)+dot(p,p)+dot(p,p);}"
            "void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(f(fragCoord));}";
        UshaderGolfStats deep_stats{};
        bool improved = false;
        char* applied_json = nullptr;
        char* deep_golfed = ushader_golf_harder_deep(deep_source, all, nullptr, 1, 200, 2000, &deep_stats, &improved, &applied_json);
        if (deep_golfed == nullptr)
        {
            std::fprintf(stderr, "golf harder deep: ushader_golf_harder_deep returned null\n");
            failures += 1;
        }
        else if (deep_stats.output_chars != std::strlen(deep_golfed))
        {
            std::fprintf(stderr, "golf harder deep: stats.output_chars (%zu) does not match golfed length (%zu)\n", deep_stats.output_chars, std::strlen(deep_golfed));
            failures += 1;
        }
        else if (applied_json == nullptr || applied_json[0] != '[')
        {
            std::fprintf(stderr, "golf harder deep: expected a JSON array in out_applied_json\n");
            failures += 1;
        }
        else
        {
            std::printf("golf harder deep: ok (%s) [improved=%d applied=%s]\n", deep_golfed, improved ? 1 : 0, applied_json);
        }
        if (deep_golfed != nullptr) { ushader_free_string(deep_golfed); }
        if (applied_json != nullptr) { ushader_free_string(applied_json); }
    }

    if (failures == 0)
    {
        std::printf("all checks passed\n");
    }

    return failures;
}
