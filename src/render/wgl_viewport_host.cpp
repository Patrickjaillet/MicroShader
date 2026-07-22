#include "wgl_viewport_host.h"

#include <GL/gl.h>

namespace
{
    typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC device_context, HGLRC share_context, const int* attrib_list);

    constexpr int kWglContextMajorVersionArb = 0x2091;
    constexpr int kWglContextMinorVersionArb = 0x2092;
    constexpr int kWglContextProfileMaskArb = 0x9126;
    constexpr int kWglContextCoreProfileBitArb = 0x00000001;

    const wchar_t* kViewportClassName = L"uShaderWglViewport";

    LRESULT CALLBACK viewport_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if (msg == WM_SIZE)
        {
            auto* host = reinterpret_cast<WglViewportHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (host != nullptr)
            {
                host->resize(LOWORD(lparam), HIWORD(lparam));
            }
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    void ensure_class_registered(HINSTANCE instance)
    {
        static bool registered = false;
        if (registered)
        {
            return;
        }

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = viewport_wndproc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = kViewportClassName;
        RegisterClassExW(&wc);
        registered = true;
    }
}

bool WglViewportHost::create(HWND parent_hwnd, int x, int y, int width, int height)
{
    HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent_hwnd, GWLP_HINSTANCE));
    ensure_class_registered(instance);

    viewport_hwnd = CreateWindowExW(
        0, kViewportClassName, L"", WS_CHILD | WS_VISIBLE,
        x, y, width, height, parent_hwnd, nullptr, instance, nullptr);
    if (viewport_hwnd == nullptr)
    {
        return false;
    }

    SetWindowLongPtrW(viewport_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    device_context = GetDC(viewport_hwnd);
    if (device_context == nullptr)
    {
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(device_context, &pfd);
    if (pixel_format == 0 || !SetPixelFormat(device_context, pixel_format, &pfd))
    {
        return false;
    }

    HGLRC temp_context = wglCreateContext(device_context);
    if (temp_context == nullptr)
    {
        return false;
    }
    wglMakeCurrent(device_context, temp_context);

    auto create_context_attribs_arb = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
        wglGetProcAddress("wglCreateContextAttribsARB"));

    if (create_context_attribs_arb != nullptr)
    {
        const int attribs[] = {
            kWglContextMajorVersionArb, 3,
            kWglContextMinorVersionArb, 3,
            kWglContextProfileMaskArb, kWglContextCoreProfileBitArb,
            0
        };
        HGLRC core_context = create_context_attribs_arb(device_context, nullptr, attribs);
        if (core_context != nullptr)
        {
            wglMakeCurrent(device_context, nullptr);
            wglDeleteContext(temp_context);
            gl_context = core_context;
            core_profile = true;
            wglMakeCurrent(device_context, gl_context);
        }
        else
        {
            gl_context = temp_context;
        }
    }
    else
    {
        gl_context = temp_context;
    }

    current_width = width;
    current_height = height;
    return true;
}

void WglViewportHost::make_current() const
{
    if (device_context != nullptr && gl_context != nullptr)
    {
        wglMakeCurrent(device_context, gl_context);
    }
}

void WglViewportHost::swap_buffers() const
{
    if (device_context != nullptr)
    {
        SwapBuffers(device_context);
    }
}

void WglViewportHost::resize(int width, int height)
{
    current_width = width;
    current_height = height;
    if (gl_context != nullptr && wglGetCurrentContext() == gl_context)
    {
        glViewport(0, 0, width, height);
    }
}

void WglViewportHost::destroy()
{
    if (gl_context != nullptr)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(gl_context);
        gl_context = nullptr;
    }
    if (device_context != nullptr && viewport_hwnd != nullptr)
    {
        ReleaseDC(viewport_hwnd, device_context);
        device_context = nullptr;
    }
    if (viewport_hwnd != nullptr)
    {
        DestroyWindow(viewport_hwnd);
        viewport_hwnd = nullptr;
    }
}
