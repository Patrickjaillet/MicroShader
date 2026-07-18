#pragma once

#include <string>

class OffscreenFramebuffer;

bool save_framebuffer_png(const OffscreenFramebuffer& fb, const std::string& path);
