#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class WglViewportHost
{
public:
    bool create(HWND parent_hwnd, int x, int y, int width, int height);
    void make_current() const;
    void swap_buffers() const;
    void resize(int width, int height);
    void destroy();

    HWND hwnd() const { return viewport_hwnd; }
    int width() const { return current_width; }
    int height() const { return current_height; }
    bool is_core_profile() const { return core_profile; }

private:
    HWND viewport_hwnd = nullptr;
    HDC device_context = nullptr;
    HGLRC gl_context = nullptr;
    int current_width = 0;
    int current_height = 0;
    bool core_profile = false;
};
