#include "wgl_viewport_host.h"
#include "gl_functions.h"
#include "shader_runner.h"
#include "default_shader.h"

#include <windows.h>
#include <GL/gl.h>

#include <chrono>
#include <cstdio>
#include <string>

namespace
{
    const wchar_t* kMainClassName = L"uShaderWglSpikeMain";
    WglViewportHost* g_viewport = nullptr;

    LRESULT CALLBACK main_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
        case WM_SIZE:
            if (g_viewport != nullptr && g_viewport->hwnd() != nullptr)
            {
                RECT client{};
                GetClientRect(hwnd, &client);
                MoveWindow(g_viewport->hwnd(), 0, 0, client.right - client.left, client.bottom - client.top, TRUE);
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
    }
}

int main()
{
    HINSTANCE instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = main_wndproc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kMainClassName;
    RegisterClassExW(&wc);

    HWND main_hwnd = CreateWindowExW(
        0, kMainClassName, L"uShader WGL viewport spike", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 960, 540,
        nullptr, nullptr, instance, nullptr);
    if (main_hwnd == nullptr)
    {
        return 1;
    }

    RECT client{};
    GetClientRect(main_hwnd, &client);

    WglViewportHost viewport;
    if (!viewport.create(main_hwnd, 0, 0, client.right - client.left, client.bottom - client.top))
    {
        std::printf("failed to create WGL viewport host\n");
        return 1;
    }
    g_viewport = &viewport;

    viewport.make_current();
    if (!gl_load_functions_wgl())
    {
        std::printf("failed to load required OpenGL 3.3 core functions\n");
        return 1;
    }

    ShaderRunner runner;
    std::string compile_error;
    if (!runner.compile(kDefaultShaderSource, compile_error))
    {
        std::printf("shader compile/link failed:\n%s\n", compile_error.c_str());
        return 1;
    }

    ShowWindow(main_hwnd, SW_SHOW);
    UpdateWindow(main_hwnd);

    auto start_time = std::chrono::steady_clock::now();
    int frame_index = 0;
    bool running = true;
    while (running)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running)
        {
            break;
        }

        viewport.make_current();

        ShaderUniforms uniforms{};
        uniforms.time = std::chrono::duration<float>(std::chrono::steady_clock::now() - start_time).count();
        uniforms.resolution_x = static_cast<float>(viewport.width());
        uniforms.resolution_y = static_cast<float>(viewport.height());
        uniforms.frame = frame_index++;
        uniforms.frame_rate = 60.0f;

        glViewport(0, 0, viewport.width(), viewport.height());
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        runner.draw(uniforms);
        viewport.swap_buffers();
    }

    runner.destroy();
    viewport.destroy();
    return 0;
}
