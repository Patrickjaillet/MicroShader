#pragma once

struct Texture
{
    unsigned int id = 0;
    int width = 0;
    int height = 0;
};

Texture load_texture_from_file(const char* path);
void destroy_texture(Texture& texture);
