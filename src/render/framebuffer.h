#pragma once

#include <vector>

class OffscreenFramebuffer
{
public:
    bool resize(int width, int height);
    void bind() const;
    void unbind(int window_width, int window_height) const;
    void destroy();
    bool read_pixels(std::vector<unsigned char>& out_rgba) const;

    unsigned int texture_id() const { return texture; }
    int width() const { return current_width; }
    int height() const { return current_height; }

private:
    unsigned int fbo = 0;
    unsigned int texture = 0;
    int current_width = 0;
    int current_height = 0;
};
