#include "screenshot.h"

#include <stb_image_write.h>
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
