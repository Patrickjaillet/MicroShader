#include "screenshot.h"

#include <stb_image_write.h>
#include <cstdlib>
#include <vector>

#include "../render/framebuffer.h"

bool save_framebuffer_png(const OffscreenFramebuffer& fb, const std::string& path)
{
    std::vector<unsigned char> pixels;
    if (!fb.read_pixels(pixels))
    {
        return false;
    }

    stbi_flip_vertically_on_write(1);
    int stride = fb.width() * 4;
    return stbi_write_png(path.c_str(), fb.width(), fb.height(), 4, pixels.data(), stride) != 0;
}

bool encode_framebuffer_png_to_memory(const OffscreenFramebuffer& fb, std::vector<unsigned char>& out_png_bytes)
{
    std::vector<unsigned char> pixels;
    if (!fb.read_pixels(pixels))
    {
        return false;
    }

    stbi_flip_vertically_on_write(1);
    int stride = fb.width() * 4;

    out_png_bytes.clear();
    auto write_cb = [](void* context, void* data, int size) {
        auto* out = static_cast<std::vector<unsigned char>*>(context);
        auto* bytes = static_cast<unsigned char*>(data);
        out->insert(out->end(), bytes, bytes + size);
    };

    int ok = stbi_write_png_to_func(write_cb, &out_png_bytes, fb.width(), fb.height(), 4, pixels.data(), stride);
    return ok != 0 && !out_png_bytes.empty();
}

bool diff_framebuffers(const OffscreenFramebuffer& source_fb, const OffscreenFramebuffer& golfed_fb, FramebufferDiffResult& out_result)
{
    if (source_fb.width() != golfed_fb.width() || source_fb.height() != golfed_fb.height())
    {
        return false;
    }

    std::vector<unsigned char> source_pixels;
    std::vector<unsigned char> golfed_pixels;
    if (!source_fb.read_pixels(source_pixels) || !golfed_fb.read_pixels(golfed_pixels))
    {
        return false;
    }

    if (source_pixels.size() != golfed_pixels.size())
    {
        return false;
    }

    int max_delta = 0;
    unsigned long long sum_delta = 0;
    size_t pixel_count = source_pixels.size() / 4;

    for (size_t i = 0; i < source_pixels.size(); i += 4)
    {
        int pixel_delta = 0;
        for (size_t channel = 0; channel < 4; ++channel)
        {
            int diff = std::abs(static_cast<int>(source_pixels[i + channel]) - static_cast<int>(golfed_pixels[i + channel]));
            pixel_delta = diff > pixel_delta ? diff : pixel_delta;
        }
        max_delta = pixel_delta > max_delta ? pixel_delta : max_delta;
        sum_delta += static_cast<unsigned long long>(pixel_delta);
    }

    out_result.max_delta = max_delta;
    out_result.mean_delta = pixel_count > 0 ? static_cast<double>(sum_delta) / static_cast<double>(pixel_count) : 0.0;
    return true;
}

bool framebuffer_diff_within_tolerance(const FramebufferDiffResult& diff, int max_delta_tolerance)
{
    return diff.max_delta <= max_delta_tolerance;
}
