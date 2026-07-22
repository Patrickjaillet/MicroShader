#include "../src/render/wgl_viewport_host.h"
#include "../src/render/gl_functions.h"
#include "../src/render/shader_runner.h"
#include "../src/render/framebuffer.h"
#include "../src/render/default_shader.h"

#include <windows.h>

#include <cstdio>
#include <string>

namespace
{
    const wchar_t* kHiddenClassName = L"uShaderWglEquivalenceTestHost";

    LRESULT CALLBACK hidden_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

int main()
{
    HINSTANCE instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = hidden_wndproc;
    wc.hInstance = instance;
    wc.lpszClassName = kHiddenClassName;
    RegisterClassExW(&wc);

    HWND hidden_hwnd = CreateWindowExW(
        0, kHiddenClassName, L"", WS_POPUP,
        0, 0, 640, 360, nullptr, nullptr, instance, nullptr);
    if (hidden_hwnd == nullptr)
    {
        std::fprintf(stderr, "could not create hidden host window\n");
        return 1;
    }

    WglViewportHost viewport;
    if (!viewport.create(hidden_hwnd, 0, 0, 640, 360))
    {
        std::fprintf(stderr, "could not create WGL viewport host\n");
        return 1;
    }
    viewport.make_current();

    if (!gl_load_functions_wgl())
    {
        std::fprintf(stderr, "could not load OpenGL 3.3 core functions under WGL\n");
        return 1;
    }

    ShaderRunner source_runner;
    ShaderRunner golfed_runner;
    std::string compile_error;
    if (!source_runner.compile(kDefaultShaderSource, compile_error))
    {
        std::fprintf(stderr, "source shader compile failed: %s\n", compile_error.c_str());
        return 1;
    }
    if (!golfed_runner.compile(kDefaultShaderSource, compile_error))
    {
        std::fprintf(stderr, "golfed shader compile failed: %s\n", compile_error.c_str());
        return 1;
    }

    OffscreenFramebuffer source_fb;
    OffscreenFramebuffer golfed_fb;

    EquivalenceSampleConfig config;
    EquivalenceRunResult result = run_equivalence_check(
        source_runner, golfed_runner, source_fb, golfed_fb, config, 640, 360);

    source_fb.destroy();
    golfed_fb.destroy();
    source_runner.destroy();
    golfed_runner.destroy();
    viewport.destroy();

    int failures = 0;
    if (!result.valid)
    {
        std::fprintf(stderr, "equivalence run did not complete under the WGL-hosted context\n");
        failures += 1;
    }
    else if (result.samples_failed != 0)
    {
        std::fprintf(stderr, "%d/%d samples differ under WGL hosting, max delta %d\n",
            result.samples_failed, result.samples_total, result.worst_max_delta);
        failures += 1;
    }
    else
    {
        std::printf("%d/%d samples bit-exact under the WGL-hosted context, matching the Phase 15 SDL-hosted safety net\n",
            result.samples_total, result.samples_total);
    }

    return failures;
}
