#include "texture.h"

#include <stb_image.h>

#include "gl_functions.h"

Texture load_texture_from_file(const char* path)
{
    Texture texture;

    int channels = 0;
    unsigned char* pixels = stbi_load(path, &texture.width, &texture.height, &channels, 4);
    if (pixels == nullptr)
    {
        return Texture{};
    }

    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);
    return texture;
}

void destroy_texture(Texture& texture)
{
    if (texture.id != 0)
    {
        GLuint id = texture.id;
        glDeleteTextures(1, &id);
        texture.id = 0;
    }
    texture.width = 0;
    texture.height = 0;
}
