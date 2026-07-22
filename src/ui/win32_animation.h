#pragma once

#include <chrono>

class AnimatedColor
{
public:
    void set_target(float red, float green, float blue, float alpha, float duration_seconds = 0.1f);
    void snap_to(float red, float green, float blue, float alpha);
    bool tick();
    void current(float& red, float& green, float& blue, float& alpha) const;
    bool is_animating() const { return animating; }

private:
    float start_r = 0.0f, start_g = 0.0f, start_b = 0.0f, start_a = 0.0f;
    float target_r = 0.0f, target_g = 0.0f, target_b = 0.0f, target_a = 0.0f;
    float cur_r = 0.0f, cur_g = 0.0f, cur_b = 0.0f, cur_a = 0.0f;
    float duration = 0.1f;
    std::chrono::steady_clock::time_point start_time;
    bool animating = false;
};
