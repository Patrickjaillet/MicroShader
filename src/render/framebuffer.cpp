#include "framebuffer.h"

#include "gl_functions.h"

bool OffscreenFramebuffer::resize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return false;
    }

    if (width == current_width && height == current_height && fbo != 0)
    {
        return true;
    }

    destroy();

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (!complete)
    {
        destroy();
        return false;
    }

    current_width = width;
    current_height = height;
    return true;
}

void OffscreenFramebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, current_width, current_height);
}

void OffscreenFramebuffer::unbind(int window_width, int window_height) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_width, window_height);
}

bool OffscreenFramebuffer::read_pixels(std::vector<unsigned char>& out_rgba) const
{
    if (fbo == 0 || current_width <= 0 || current_height <= 0)
    {
        return false;
    }

    out_rgba.resize(static_cast<size_t>(current_width) * static_cast<size_t>(current_height) * 4);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glReadPixels(0, 0, current_width, current_height, GL_RGBA, GL_UNSIGNED_BYTE, out_rgba.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void OffscreenFramebuffer::destroy()
{
    if (fbo != 0)
    {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (texture != 0)
    {
        GLuint tex = texture;
        glDeleteTextures(1, &tex);
        texture = 0;
    }
    current_width = 0;
    current_height = 0;
}
