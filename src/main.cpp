#include <SDL3/SDL.h>

#include <cstdio>
#include <ctime>

#include "render/default_shader.h"
#include "render/gl_functions.h"
#include "render/shader_runner.h"
#include "ushader/version.h"

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow(
        "uShader " USHADER_VERSION_STRING,
        1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if (!window)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    if (!gl_load_functions())
    {
        std::printf("failed to load required OpenGL 3.3 core functions\n");
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    ShaderRunner runner;
    std::string error_log;
    if (!runner.compile(kDefaultShaderSource, error_log))
    {
        std::printf("shader compile/link failed:\n%s\n", error_log.c_str());
    }

    Uint64 start_ticks = SDL_GetTicks();
    Uint64 last_ticks = start_ticks;
    int frame = 0;

    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    float mouse_click_x = 0.0f;
    float mouse_click_y = 0.0f;

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                int pixel_w = 0;
                int pixel_h = 0;
                SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);
                glViewport(0, 0, pixel_w, pixel_h);
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                mouse_click_x = event.button.x;
                mouse_click_y = event.button.y;
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                mouse_x = event.motion.x;
                mouse_y = event.motion.y;
            }
        }

        int pixel_w = 0;
        int pixel_h = 0;
        SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);

        Uint64 now_ticks = SDL_GetTicks();
        Uint64 delta_ms = now_ticks - last_ticks;
        last_ticks = now_ticks;

        std::time_t raw_time = std::time(nullptr);
        std::tm local_time{};
        localtime_s(&local_time, &raw_time);

        ShaderUniforms uniforms{};
        uniforms.time = static_cast<float>(now_ticks - start_ticks) / 1000.0f;
        uniforms.resolution_x = static_cast<float>(pixel_w);
        uniforms.resolution_y = static_cast<float>(pixel_h);
        uniforms.mouse_x = mouse_x;
        uniforms.mouse_y = static_cast<float>(pixel_h) - mouse_y;
        uniforms.mouse_click_x = mouse_click_x;
        uniforms.mouse_click_y = static_cast<float>(pixel_h) - mouse_click_y;
        uniforms.date_year = static_cast<float>(local_time.tm_year + 1900);
        uniforms.date_month = static_cast<float>(local_time.tm_mon + 1);
        uniforms.date_day = static_cast<float>(local_time.tm_mday);
        uniforms.date_seconds = static_cast<float>(local_time.tm_hour * 3600 + local_time.tm_min * 60 + local_time.tm_sec);
        uniforms.frame = frame;
        uniforms.frame_rate = delta_ms > 0 ? (1000.0f / static_cast<float>(delta_ms)) : 0.0f;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        runner.draw(uniforms);

        SDL_GL_SwapWindow(window);
        frame += 1;
    }

    runner.destroy();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
