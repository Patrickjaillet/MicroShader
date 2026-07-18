#include <SDL3/SDL.h>
#include <TextEditor.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <cstdio>
#include <ctime>
#include <regex>
#include <string>

#include "render/default_shader.h"
#include "render/framebuffer.h"
#include "render/gl_functions.h"
#include "render/shader_runner.h"
#include "ui/glsl_format.h"
#include "ui/glsl_language.h"
#include "ui/icons.h"
#include "ui/layout.h"
#include "ui/theme.h"
#include "ushader/golf_core.h"
#include "ushader/version.h"

namespace
{
    int parse_error_line_number(const std::string& log)
    {
        static const std::regex nvidia_pattern(R"(0\((\d+)\))");
        static const std::regex mesa_pattern(R"(0:(\d+))");
        std::smatch match;
        if (std::regex_search(log, match, nvidia_pattern) && match.size() > 1)
        {
            return std::stoi(match[1].str());
        }
        if (std::regex_search(log, match, mesa_pattern) && match.size() > 1)
        {
            return std::stoi(match[1].str());
        }
        return -1;
    }

    std::string error_line_prefix(int line)
    {
        if (line > 0)
        {
            return "Line " + std::to_string(line) + ": ";
        }
        return std::string();
    }

    std::string golf_source(const std::string& source)
    {
        UshaderGolfOptions options{true, true, true, true, true, true, true, true, true, true, true, true, true, true};
        char* result = ushader_golf(source.c_str(), options, nullptr);
        if (result == nullptr)
        {
            return std::string();
        }
        std::string golfed(result);
        ushader_free_string(result);
        return golfed;
    }
}

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    load_fonts(io, 18.0f);
    apply_theme();

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    ShaderRunner runner;
    OffscreenFramebuffer viewport_fb;

    TextEditor source_editor;
    source_editor.SetLanguageDefinition(glsl_language_definition());
    source_editor.SetPalette(TextEditor::GetLightPalette());
    source_editor.SetText(kDefaultShaderSource);

    TextEditor golfed_editor;
    golfed_editor.SetLanguageDefinition(glsl_language_definition());
    golfed_editor.SetPalette(TextEditor::GetLightPalette());
    golfed_editor.SetReadOnly(true);

    std::string golfed_text;
    std::string compile_error;
    bool formatted_view = false;

    auto run_golf_action = [&]()
    {
        std::string source_text = source_editor.GetText();

        std::string source_error;
        if (!runner.compile(source_text, source_error))
        {
            compile_error = source_error;
            int line = parse_error_line_number(source_error) - ShaderRunner::fragment_header_lines();
            TextEditor::ErrorMarkers markers;
            if (line > 0)
            {
                markers[line] = source_error;
            }
            source_editor.SetErrorMarkers(markers);
            std::printf("shader compile/link failed:\n%s\n", source_error.c_str());
            std::fflush(stdout);
            return;
        }
        source_editor.SetErrorMarkers(TextEditor::ErrorMarkers());

        golfed_text = golf_source(source_text);
        const std::string& to_compile = golfed_text.empty() ? source_text : golfed_text;
        std::string golf_error;
        if (!runner.compile(to_compile, golf_error))
        {
            compile_error = golf_error;
            std::printf("shader compile/link failed:\n%s\n", golf_error.c_str());
            std::fflush(stdout);
        }
        else
        {
            compile_error.clear();
        }

        golfed_editor.SetText(formatted_view ? format_glsl(golfed_text) : golfed_text);
    };

    run_golf_action();

    Uint64 start_ticks = SDL_GetTicks();
    Uint64 last_ticks = start_ticks;
    int frame = 0;

    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    float mouse_click_x = 0.0f;
    float mouse_click_y = 0.0f;

    bool layout_built = false;
    bool last_narrow = false;

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(main_viewport->WorkPos);
        ImGui::SetNextWindowSize(main_viewport->WorkSize);
        ImGui::SetNextWindowViewport(main_viewport->ID);

        ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("##DockHost", nullptr, host_flags);
        ImGui::PopStyleVar();

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        bool narrow = main_viewport->WorkSize.x < 900.0f;
        if (!layout_built || narrow != last_narrow)
        {
            build_dock_layout(dockspace_id, narrow);
            layout_built = true;
            last_narrow = narrow;
        }
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
        ImGui::End();

        ImGui::Begin(kSourceWindowTitle);
        ImGui::PushFont(g_icon_font);
        ImGui::Text(ICON_PLAY);
        ImGui::PopFont();
        ImGui::SameLine(0.0f, 6.0f);
        if (ImGui::Button("Run golf"))
        {
            run_golf_action();
        }
        source_editor.Render("##source");
        ImGui::End();

        ImGui::Begin(kGolfedWindowTitle);
        if (ImGui::Checkbox("Formatted view", &formatted_view))
        {
            golfed_editor.SetText(formatted_view ? format_glsl(golfed_text) : golfed_text);
        }
        golfed_editor.Render("##golfed");
        ImGui::End();

        ImGui::Begin(kViewportWindowTitle);

        if (!compile_error.empty())
        {
            ImGui::PushFont(g_icon_font);
            ImGui::TextColored(ImVec4(0.80f, 0.20f, 0.20f, 1.0f), ICON_CIRCLE_ALERT);
            ImGui::PopFont();
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::TextColored(ImVec4(0.80f, 0.20f, 0.20f, 1.0f), "Shader error");
            int display_line = parse_error_line_number(compile_error) - ShaderRunner::fragment_header_lines();
            ImGui::TextWrapped("%s%s", error_line_prefix(display_line).c_str(), compile_error.c_str());
            ImGui::Separator();
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();

        int window_pixel_w = 0;
        int window_pixel_h = 0;
        SDL_GetWindowSizeInPixels(window, &window_pixel_w, &window_pixel_h);

        if (avail.x >= 1.0f && avail.y >= 1.0f && viewport_fb.resize(static_cast<int>(avail.x), static_cast<int>(avail.y)))
        {
            Uint64 now_ticks = SDL_GetTicks();
            Uint64 delta_ms = now_ticks - last_ticks;
            last_ticks = now_ticks;

            std::time_t raw_time = std::time(nullptr);
            std::tm local_time{};
            localtime_s(&local_time, &raw_time);

            ShaderUniforms uniforms{};
            uniforms.time = static_cast<float>(now_ticks - start_ticks) / 1000.0f;
            uniforms.resolution_x = avail.x;
            uniforms.resolution_y = avail.y;
            uniforms.mouse_x = mouse_x;
            uniforms.mouse_y = avail.y - mouse_y;
            uniforms.mouse_click_x = mouse_click_x;
            uniforms.mouse_click_y = avail.y - mouse_click_y;
            uniforms.date_year = static_cast<float>(local_time.tm_year + 1900);
            uniforms.date_month = static_cast<float>(local_time.tm_mon + 1);
            uniforms.date_day = static_cast<float>(local_time.tm_mday);
            uniforms.date_seconds = static_cast<float>(local_time.tm_hour * 3600 + local_time.tm_min * 60 + local_time.tm_sec);
            uniforms.frame = frame;
            uniforms.frame_rate = delta_ms > 0 ? (1000.0f / static_cast<float>(delta_ms)) : 0.0f;
            frame += 1;

            viewport_fb.bind();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            runner.draw(uniforms);
            viewport_fb.unbind(window_pixel_w, window_pixel_h);

            ImGui::Image(static_cast<ImTextureID>(viewport_fb.texture_id()), avail, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        }

        ImGui::End();

        ImGui::Render();

        glViewport(0, 0, window_pixel_w, window_pixel_h);
        glClearColor(0.90f, 0.91f, 0.94f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    viewport_fb.destroy();
    runner.destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
