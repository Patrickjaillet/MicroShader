#pragma once

#include <string>
#include <unordered_map>

struct ID2D1RenderTarget;

namespace Gdiplus
{
    class Bitmap;
}

class IconSet
{
public:
    bool load(const std::wstring& icon_directory);
    void release();

    void draw(ID2D1RenderTarget* render_target, const char* icon_name, int size,
        float x, float y, float red, float green, float blue, float alpha) const;

private:
    Gdiplus::Bitmap* find_bitmap(const char* icon_name, int size) const;

    std::unordered_map<std::string, Gdiplus::Bitmap*> bitmaps;
};
