#include "win32_icon_set.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d2d1.h>
#include <gdiplus.h>

#include <cstring>

namespace
{
    constexpr int kSizes[] = { 16, 20, 24, 32 };

    const char* kIconNames[] = {
        "play", "circle_alert", "code", "image", "folder_open", "save",
        "copy", "download", "camera", "info", "circle", "circle_stop",
        "minus", "square", "x",
    };

    std::string make_key(const char* icon_name, int size)
    {
        return std::string(icon_name) + "_" + std::to_string(size);
    }

    int nearest_size(int requested)
    {
        int best = kSizes[0];
        int best_diff = 1 << 30;
        for (int candidate : kSizes)
        {
            int diff = candidate - requested;
            if (diff < 0)
            {
                diff = -diff;
            }
            if (diff < best_diff)
            {
                best_diff = diff;
                best = candidate;
            }
        }
        return best;
    }
}

bool IconSet::load(const std::wstring& icon_directory)
{
    bool all_ok = true;
    for (const char* name : kIconNames)
    {
        for (int size : kSizes)
        {
            std::wstring path = icon_directory + L"\\" + std::wstring(name, name + strlen(name)) + L"_" + std::to_wstring(size) + L".png";
            auto* bitmap = Gdiplus::Bitmap::FromFile(path.c_str());
            if (bitmap == nullptr || bitmap->GetLastStatus() != Gdiplus::Ok)
            {
                delete bitmap;
                all_ok = false;
                continue;
            }
            bitmaps[make_key(name, size)] = bitmap;
        }
    }
    return all_ok;
}

void IconSet::release()
{
    for (auto& entry : bitmaps)
    {
        delete entry.second;
    }
    bitmaps.clear();
}

Gdiplus::Bitmap* IconSet::find_bitmap(const char* icon_name, int size) const
{
    auto it = bitmaps.find(make_key(icon_name, nearest_size(size)));
    if (it == bitmaps.end())
    {
        return nullptr;
    }
    return it->second;
}

void IconSet::draw(ID2D1RenderTarget* render_target, const char* icon_name, int size,
    float x, float y, float red, float green, float blue, float alpha) const
{
    Gdiplus::Bitmap* bitmap = find_bitmap(icon_name, size);
    if (bitmap == nullptr)
    {
        return;
    }

    ID2D1GdiInteropRenderTarget* interop = nullptr;
    if (FAILED(render_target->QueryInterface(&interop)) || interop == nullptr)
    {
        return;
    }

    HDC hdc = nullptr;
    if (SUCCEEDED(interop->GetDC(D2D1_DC_INITIALIZE_MODE_COPY, &hdc)))
    {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

        Gdiplus::ColorMatrix matrix = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, alpha, 0.0f,
            red, green, blue, 0.0f, 1.0f,
        };
        Gdiplus::ImageAttributes attributes;
        attributes.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

        Gdiplus::Rect dest(static_cast<INT>(x), static_cast<INT>(y), size, size);
        UINT source_width = bitmap->GetWidth();
        UINT source_height = bitmap->GetHeight();
        graphics.DrawImage(bitmap, dest, 0, 0, static_cast<INT>(source_width), static_cast<INT>(source_height),
            Gdiplus::UnitPixel, &attributes);

        RECT dirty{ static_cast<LONG>(x), static_cast<LONG>(y), static_cast<LONG>(x) + size, static_cast<LONG>(y) + size };
        interop->ReleaseDC(&dirty);
    }

    interop->Release();
}
