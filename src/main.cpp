#include <SDL3/SDL.h>
#include <TextEditor.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <cstdio>
#include <ctime>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

#include "platform/file_dialog.h"
#include "platform/screenshot.h"
#include "render/default_shader.h"
#include "render/framebuffer.h"
#include "render/gl_functions.h"
#include "render/shader_runner.h"
#include "ui/glsl_format.h"
#include "ui/glsl_language.h"
#include "ui/golf_controls.h"
#include "ui/icons.h"
#include "ui/layout.h"
#include "ui/stats_panel.h"
#include "ui/theme.h"
#include "ushader/golf_core.h"
#include "ushader/version.h"

namespace
{
    const wchar_t kGlslFilter[] = L"GLSL shaders (*.glsl)\0*.glsl\0All files (*.*)\0*.*\0";
    const wchar_t kPngFilter[] = L"PNG image (*.png)\0*.png\0";

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

    std::string read_text_file(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return std::string();
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool write_text_file(const std::string& path, const std::string& content)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file)
        {
            return false;
        }
        file << content;
        return static_cast<bool>(file);
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

    ShaderRunner source_runner;
    ShaderRunner golfed_runner;
    OffscreenFramebuffer source_viewport_fb;
    OffscreenFramebuffer golfed_viewport_fb;

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
    bool compare_mode = false;
    GolfPassToggles pass_toggles;
    std::string protected_names;
    UshaderGolfStats golf_stats{};

    auto icon_button = [&](const char* icon, const char* label)
    {
        ImGui::PushFont(g_icon_font);
        ImGui::Text("%s", icon);
        ImGui::PopFont();
        ImGui::SameLine(0.0f, 6.0f);
        return ImGui::Button(label);
    };

    auto run_golf_action = [&]()
    {
        std::string source_text = source_editor.GetText();

        std::string source_error;
        if (!source_runner.compile(source_text, source_error))
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

        UshaderGolfOptions options = to_golf_options(pass_toggles);
        UshaderGolfStats stats{};
        char* result = ushader_golf(
            source_text.c_str(), options,
            protected_names.empty() ? nullptr : protected_names.c_str(),
            &stats);
        golfed_text = result != nullptr ? std::string(result) : std::string();
        if (result != nullptr)
        {
            ushader_free_string(result);
        }
        golf_stats = stats;

        const std::string& to_compile = golfed_text.empty() ? source_text : golfed_text;
        std::string golf_error;
        if (!golfed_runner.compile(to_compile, golf_error))
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
        if (icon_button(ICON_PLAY, "Run golf"))
        {
            run_golf_action();
        }
        ImGui::SameLine();
        if (icon_button(ICON_FOLDER_OPEN, "Open"))
        {
            std::optional<std::string> path = show_open_file_dialog(window, kGlslFilter, L"glsl");
            if (path.has_value())
            {
                source_editor.SetText(read_text_file(*path));
                run_golf_action();
            }
        }
        ImGui::SameLine();
        if (icon_button(ICON_SAVE, "Save"))
        {
            std::optional<std::string> path = show_save_file_dialog(window, kGlslFilter, L"glsl", L"shader.glsl");
            if (path.has_value())
            {
                write_text_file(*path, source_editor.GetText());
            }
        }
        render_golf_controls(pass_toggles, protected_names);
        ImGui::Separator();
        source_editor.Render("##source");
        ImGui::End();

        ImGui::Begin(kGolfedWindowTitle);
        if (ImGui::Checkbox("Formatted view", &formatted_view))
        {
            golfed_editor.SetText(formatted_view ? format_glsl(golfed_text) : golfed_text);
        }
        ImGui::SameLine();
        if (icon_button(ICON_COPY, "Copy"))
        {
            SDL_SetClipboardText(golfed_text.c_str());
        }
        ImGui::SameLine();
        if (icon_button(ICON_DOWNLOAD, "Export (Shadertoy)"))
        {
            std::optional<std::string> path = show_save_file_dialog(window, kGlslFilter, L"glsl", L"shader.glsl");
            if (path.has_value())
            {
                write_text_file(*path, golfed_text);
            }
        }
        render_stats_panel(golf_stats, golfed_text.size());
        ImGui::Separator();
        golfed_editor.Render("##golfed");
        ImGui::End();

        ImGui::Begin(kViewportWindowTitle);
        ImGui::Checkbox("Compare", &compare_mode);
        ImGui::SameLine();
        if (icon_button(ICON_CAMERA, "Screenshot"))
        {
            std::optional<std::string> path = show_save_file_dialog(window, kPngFilter, L"png", L"screenshot.png");
            if (path.has_value())
            {
                save_framebuffer_png(golfed_viewport_fb, *path);
            }
        }

        if (!compile_error.empty())
        {
            ImGui::PushFont(g_icon_font);
            ImGui::TextColored(ImVec4(0.80f, 0.20f, 0.20f, 1.0f), ICON_CIRCLE_ALERT);
            ImGui::PopFont();
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::TextColored(ImVec4(0.80f, 0.20f, 0.20f, 1.0f), "Shader error");
            int display_line = parse_error_line_number(compile_error) - ShaderRunner::fragment_header_lines();
            ImGui::TextWrapped("%s%s", error_line_prefix(display_line).c_str(), compile_error.c_str());
        }

        ImGui::Separator();

        ImVec2 avail = ImGui::GetContentRegionAvail();

        int window_pixel_w = 0;
        int window_pixel_h = 0;
        SDL_GetWindowSizeInPixels(window, &window_pixel_w, &window_pixel_h);

        Uint64 now_ticks = SDL_GetTicks();
        Uint64 delta_ms = now_ticks - last_ticks;
        last_ticks = now_ticks;

        std::time_t raw_time = std::time(nullptr);
        std::tm local_time{};
        localtime_s(&local_time, &raw_time);

        float frame_rate = delta_ms > 0 ? (1000.0f / static_cast<float>(delta_ms)) : 0.0f;
        frame += 1;

        auto make_uniforms = [&](float width, float height)
        {
            ShaderUniforms uniforms{};
            uniforms.time = static_cast<float>(now_ticks - start_ticks) / 1000.0f;
            uniforms.resolution_x = width;
            uniforms.resolution_y = height;
            uniforms.mouse_x = mouse_x;
            uniforms.mouse_y = height - mouse_y;
            uniforms.mouse_click_x = mouse_click_x;
            uniforms.mouse_click_y = height - mouse_click_y;
            uniforms.date_year = static_cast<float>(local_time.tm_year + 1900);
            uniforms.date_month = static_cast<float>(local_time.tm_mon + 1);
            uniforms.date_day = static_cast<float>(local_time.tm_mday);
            uniforms.date_seconds = static_cast<float>(local_time.tm_hour * 3600 + local_time.tm_min * 60 + local_time.tm_sec);
            uniforms.frame = frame;
            uniforms.frame_rate = frame_rate;
            return uniforms;
        };

        auto draw_panel = [&](ShaderRunner& runner, OffscreenFramebuffer& fb, ImVec2 size)
        {
            if (size.x >= 1.0f && size.y >= 1.0f && fb.resize(static_cast<int>(size.x), static_cast<int>(size.y)))
            {
                fb.bind();
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                runner.draw(make_uniforms(size.x, size.y));
                fb.unbind(window_pixel_w, window_pixel_h);
                ImGui::Image(static_cast<ImTextureID>(fb.texture_id()), size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            }
        };

        if (compare_mode)
        {
            float half_w = (avail.x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            ImVec2 half_size(half_w, avail.y);
            draw_panel(source_runner, source_viewport_fb, half_size);
            ImGui::SameLine();
            draw_panel(golfed_runner, golfed_viewport_fb, half_size);
        }
        else
        {
            draw_panel(golfed_runner, golfed_viewport_fb, avail);
        }

        ImGui::End();

        ImGui::Render();

        glViewport(0, 0, window_pixel_w, window_pixel_h);
        glClearColor(0.90f, 0.91f, 0.94f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    source_viewport_fb.destroy();
    golfed_viewport_fb.destroy();
    source_runner.destroy();
    golfed_runner.destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
