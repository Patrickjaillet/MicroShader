#pragma once

#include <string>
#include <vector>

class OffscreenFramebuffer;

bool save_framebuffer_png(const OffscreenFramebuffer& fb, const std::string& path);
bool encode_framebuffer_png_to_memory(const OffscreenFramebuffer& fb, std::vector<unsigned char>& out_png_bytes);

struct FramebufferDiffResult
{
    int max_delta = 0;
    double mean_delta = 0.0;
};

bool diff_framebuffers(const OffscreenFramebuffer& source_fb, const OffscreenFramebuffer& golfed_fb, FramebufferDiffResult& out_result);

bool framebuffer_diff_within_tolerance(const FramebufferDiffResult& diff, int max_delta_tolerance);
