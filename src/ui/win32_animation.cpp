#include "win32_animation.h"

namespace
{
    float lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    float ease_out_cubic(float t)
    {
        float inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }
}

void AnimatedColor::set_target(float red, float green, float blue, float alpha, float duration_seconds)
{
    if (animating)
    {
        current(start_r, start_g, start_b, start_a);
    }
    else
    {
        start_r = cur_r;
        start_g = cur_g;
        start_b = cur_b;
        start_a = cur_a;
    }

    target_r = red;
    target_g = green;
    target_b = blue;
    target_a = alpha;
    duration = duration_seconds;
    start_time = std::chrono::steady_clock::now();
    animating = true;
}

void AnimatedColor::snap_to(float red, float green, float blue, float alpha)
{
    start_r = cur_r = target_r = red;
    start_g = cur_g = target_g = green;
    start_b = cur_b = target_b = blue;
    start_a = cur_a = target_a = alpha;
    animating = false;
}

bool AnimatedColor::tick()
{
    if (!animating)
    {
        return false;
    }

    float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start_time).count();
    float t = duration > 0.0f ? elapsed / duration : 1.0f;
    if (t >= 1.0f)
    {
        t = 1.0f;
        animating = false;
    }

    float eased = ease_out_cubic(t);
    cur_r = lerp(start_r, target_r, eased);
    cur_g = lerp(start_g, target_g, eased);
    cur_b = lerp(start_b, target_b, eased);
    cur_a = lerp(start_a, target_a, eased);
    return animating;
}

void AnimatedColor::current(float& red, float& green, float& blue, float& alpha) const
{
    red = cur_r;
    green = cur_g;
    blue = cur_b;
    alpha = cur_a;
}
