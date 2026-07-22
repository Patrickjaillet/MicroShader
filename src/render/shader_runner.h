#pragma once

#include <functional>
#include <string>
#include <vector>

class OffscreenFramebuffer;

struct ShaderUniforms
{
    float time;
    float resolution_x;
    float resolution_y;
    float mouse_x;
    float mouse_y;
    float mouse_click_x;
    float mouse_click_y;
    float date_year;
    float date_month;
    float date_day;
    float date_seconds;
    int frame;
    float frame_rate;
};

class ShaderRunner
{
public:
    static int fragment_header_lines();

    bool compile(const std::string& fragment_source, std::string& error_log);
    void draw(const ShaderUniforms& uniforms) const;
    void destroy();

private:
    unsigned int program = 0;
    unsigned int vao = 0;
    int loc_time = -1;
    int loc_resolution = -1;
    int loc_mouse = -1;
    int loc_date = -1;
    int loc_frame = -1;
    int loc_frame_rate = -1;
};

struct EquivalenceSampleConfig
{
    std::vector<float> sample_times{0.0f, 0.5f, 1.0f, 2.5f, 5.0f};
    float resolution_x = 640.0f;
    float resolution_y = 360.0f;
    int max_delta_tolerance = 0;
};

using EquivalenceSampleCallback = std::function<void(
    float time, const OffscreenFramebuffer& source_fb, const OffscreenFramebuffer& golfed_fb)>;

bool run_equivalence_samples(
    const ShaderRunner& source_runner,
    const ShaderRunner& golfed_runner,
    OffscreenFramebuffer& source_fb,
    OffscreenFramebuffer& golfed_fb,
    const EquivalenceSampleConfig& config,
    int window_pixel_w,
    int window_pixel_h,
    const EquivalenceSampleCallback& on_sample);

struct EquivalenceRunResult
{
    bool valid = false;
    int samples_total = 0;
    int samples_failed = 0;
    int worst_max_delta = 0;
};

EquivalenceRunResult run_equivalence_check(
    const ShaderRunner& source_runner,
    const ShaderRunner& golfed_runner,
    OffscreenFramebuffer& source_fb,
    OffscreenFramebuffer& golfed_fb,
    const EquivalenceSampleConfig& config,
    int window_pixel_w,
    int window_pixel_h);
