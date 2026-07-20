#include <SDL3/SDL.h>
#include <TextEditor.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <cstdio>
#include <cstddef>
#include <ctime>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "platform/accessibility.h"
#include "platform/file_dialog.h"
#include "platform/paths.h"
#include "platform/recorder.h"
#include "platform/screenshot.h"
#include "platform/utf8.h"
#include "render/default_shader.h"
#include "render/framebuffer.h"
#include "render/gl_functions.h"
#include "render/shader_runner.h"
#include "render/texture.h"
#include "report/report.h"
#include "ui/about_panel.h"
#include "ui/command_palette.h"
#include "ui/diff_view.h"
#include "ui/document.h"
#include "ui/equivalence_controls.h"
#include "ui/export_wrappers.h"
#include "ui/glsl_format.h"
#include "ui/glsl_language.h"
#include "ui/golf_controls.h"
#include "ui/golf_profile.h"
#include "ui/golf_trace.h"
#include "ui/icons.h"
#include "ui/keybindings.h"
#include "ui/layout.h"
#include "ui/minimap.h"
#include "ui/recent_files.h"
#include "ui/stats_panel.h"
#include "ui/theme.h"
#include "ui/theme_tokens.h"
#include "ui/workspace.h"
#include "ushader/golf_core.h"
#include "ushader/version.h"

namespace
{
    const wchar_t kGlslFilter[] = L"GLSL shaders (*.glsl)\0*.glsl\0All files (*.*)\0*.*\0";
    const wchar_t kPngFilter[] = L"PNG image (*.png)\0*.png\0";
    const wchar_t kGifFilter[] = L"GIF animation (*.gif)\0*.gif\0";
    const wchar_t kMp4Filter[] = L"MP4 video (*.mp4)\0*.mp4\0";
    const wchar_t kWebMFilter[] = L"WebM video (*.webm)\0*.webm\0";
    const int kRecordFps = 24;
    const float kTitleBarHeight = 32.0f;

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

    std::string format_timecode(float seconds)
    {
        int total_seconds = static_cast<int>(seconds);
        int minutes = total_seconds / 60;
        int secs = total_seconds % 60;
        int frames_frac = static_cast<int>((seconds - static_cast<float>(total_seconds)) * 100.0f);
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d.%02d", minutes, secs, frames_frac);
        return std::string(buffer);
    }

    struct TitleBarHitTestState
    {
        SDL_Rect minimize_rect{};
        SDL_Rect maximize_rect{};
        SDL_Rect close_rect{};
        int title_bar_height = 0;
    };

    bool point_in_rect(int x, int y, const SDL_Rect& rect)
    {
        return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
    }

    SDL_HitTestResult SDLCALL title_bar_hit_test(SDL_Window* win, const SDL_Point* area, void* data)
    {
        const TitleBarHitTestState* state = static_cast<const TitleBarHitTestState*>(data);
        int width = 0;
        int height = 0;
        SDL_GetWindowSize(win, &width, &height);

        const int edge = 6;
        bool at_left = area->x < edge;
        bool at_right = area->x >= width - edge;
        bool at_top = area->y < edge;
        bool at_bottom = area->y >= height - edge;

        if (at_top && at_left) return SDL_HITTEST_RESIZE_TOPLEFT;
        if (at_top && at_right) return SDL_HITTEST_RESIZE_TOPRIGHT;
        if (at_bottom && at_left) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        if (at_bottom && at_right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        if (at_left) return SDL_HITTEST_RESIZE_LEFT;
        if (at_right) return SDL_HITTEST_RESIZE_RIGHT;
        if (at_top) return SDL_HITTEST_RESIZE_TOP;
        if (at_bottom) return SDL_HITTEST_RESIZE_BOTTOM;

        if (area->y < state->title_bar_height)
        {
            if (point_in_rect(area->x, area->y, state->minimize_rect) ||
                point_in_rect(area->x, area->y, state->maximize_rect) ||
                point_in_rect(area->x, area->y, state->close_rect))
            {
                return SDL_HITTEST_NORMAL;
            }
            return SDL_HITTEST_DRAGGABLE;
        }

        return SDL_HITTEST_NORMAL;
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
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS
            | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!window)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    TitleBarHitTestState hit_test_state{};
    hit_test_state.title_bar_height = static_cast<int>(kTitleBarHeight);
    SDL_SetWindowHitTest(window, title_bar_hit_test, &hit_test_state);

    accessibility_init(window);

    SDL_MaximizeWindow(window);

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

    std::string layout_ini_path = workspace_layout_path();
    bool had_layout_ini = !layout_ini_path.empty() && !read_utf8_file(layout_ini_path).empty();
    io.IniFilename = layout_ini_path.empty() ? nullptr : layout_ini_path.c_str();

    WorkspaceState restore_state;
    bool restore_prompt_open = false;
    if (workspace_session_exists() &&
        deserialize_workspace(read_utf8_file(workspace_session_path()), restore_state))
    {
        restore_prompt_open = !restore_state.documents.empty();
    }
    float ui_font_size = restore_state.ui_font_size;

    load_fonts(io, 18.0f);
    apply_theme();

    float dpi_scale = SDL_GetWindowDisplayScale(window);
    if (dpi_scale <= 0.0f)
    {
        dpi_scale = 1.0f;
    }
    apply_dpi_scale(io, dpi_scale * (ui_font_size / kDefaultBaseFontSize));

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    Texture logo_texture = load_texture_from_file(asset_path("branding/logo.png").c_str());
    Texture app_icon_texture = load_texture_from_file(asset_path("branding/app_icon.png").c_str());
    bool show_about = false;

    ShaderRunner source_runner;
    ShaderRunner golfed_runner;
    OffscreenFramebuffer source_viewport_fb;
    OffscreenFramebuffer golfed_viewport_fb;
    OffscreenFramebuffer equivalence_source_fb;
    OffscreenFramebuffer equivalence_golfed_fb;

    TextEditor trace_before_editor;
    trace_before_editor.SetLanguageDefinition(glsl_language_definition());
    trace_before_editor.SetPalette(TextEditor::GetDarkPalette());
    trace_before_editor.SetReadOnly(true);

    TextEditor trace_after_editor;
    trace_after_editor.SetLanguageDefinition(glsl_language_definition());
    trace_after_editor.SetPalette(TextEditor::GetDarkPalette());
    trace_after_editor.SetReadOnly(true);

    bool compare_mode = false;
    bool report_include_screenshot = false;
    std::string last_profile_path = load_last_profile_path();
    EquivalenceSampleConfig equivalence_config;
    bool command_palette_open = false;
    Keybindings keybindings = load_keybindings();
    MinimapSettings minimap_settings;
    std::vector<std::string> recent_files = load_recent_files();

    ViewportRecorder recorder;
    RecordingFormat record_format = RecordingFormat::Gif;
    bool start_recording_requested = false;
    Uint64 last_capture_ticks = 0;

    std::vector<std::unique_ptr<Document>> documents;
    int active_index = 0;
    int next_tab_id = 1;
    int pending_select_id = 0;
    int last_synced_tab_id = 0;
    int close_confirm_tab_id = -1;
    bool exit_requested = false;
    bool exit_confirm_open = false;

    auto icon_button_ex = [&](const char* icon, const char* label, bool primary)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 start = ImGui::GetCursorScreenPos();

        ImGui::PushFont(g_icon_font);
        ImVec2 icon_size = ImGui::CalcTextSize(icon);
        ImGui::PopFont();
        ImVec2 label_size = ImGui::CalcTextSize(label);
        const float gap = 6.0f;
        ImVec2 button_size(
            style.FramePadding.x * 2.0f + icon_size.x + gap + label_size.x,
            style.FramePadding.y * 2.0f + ImGui::GetFontSize());

        if (primary)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, tokens::accent);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens::accent_hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, tokens::accent_active);
        }

        ImGui::PushID(label);
        bool pressed = ImGui::Button("##icon_button", button_size);
        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();
        ImGui::PopID();

        if (primary)
        {
            ImGui::PopStyleColor(3);
        }

        ImVec4 icon_color = primary ? tokens::text_primary
            : (active ? tokens::accent : (hovered ? tokens::text_primary : tokens::text_secondary));
        ImU32 icon_color_u32 = ImGui::GetColorU32(icon_color);
        ImU32 text_color_u32 = primary ? ImGui::GetColorU32(tokens::text_primary) : ImGui::GetColorU32(ImGuiCol_Text);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 icon_pos(start.x + style.FramePadding.x, start.y + (button_size.y - icon_size.y) * 0.5f);
        ImGui::PushFont(g_icon_font);
        draw_list->AddText(icon_pos, icon_color_u32, icon);
        ImGui::PopFont();
        ImVec2 label_pos(icon_pos.x + icon_size.x + gap, start.y + (button_size.y - label_size.y) * 0.5f);
        draw_list->AddText(label_pos, text_color_u32, label);

        accessibility_register(label, AccessibleRole::Button, start.x, start.y, button_size.x, button_size.y, true);

        return pressed;
    };

    auto icon_button = [&](const char* icon, const char* label)
    {
        return icon_button_ex(icon, label, false);
    };

    auto compute_golf_engine = [&](Document& document)
    {
        std::string source_text = document.source_editor.GetText();

        UshaderGolfOptions options = to_golf_options(document.pass_toggles);
        UshaderGolfStats stats{};
        char* trace_json = nullptr;
        char* result = ushader_golf_traced(
            source_text.c_str(), options,
            document.protected_names.empty() ? nullptr : document.protected_names.c_str(),
            &stats, &trace_json);
        document.golfed_text = result != nullptr ? std::string(result) : std::string();
        if (result != nullptr)
        {
            ushader_free_string(result);
        }
        document.golf_stats = stats;

        document.golf_trace = trace_json != nullptr ? parse_golf_trace(std::string(trace_json)) : std::vector<GolfTraceStep>();
        if (trace_json != nullptr)
        {
            ushader_free_string(trace_json);
        }

        const std::string& diff_before = document.golf_trace.empty() ? source_text : document.golf_trace.front().before;
        const std::string& diff_after = document.golf_trace.empty() ? document.golfed_text : document.golf_trace.back().after;
        document.diff_spans = compute_unified_diff(diff_before, diff_after);

        const std::string& budget_source = document.golfed_text.empty() ? source_text : document.golfed_text;
        document.budget_result = ushader_estimate_budget(budget_source.c_str());

        document.golfed_editor.SetText(document.formatted_view ? format_glsl(document.golfed_text) : document.golfed_text);
    };

    auto run_full_golf = [&](Document& document)
    {
        std::string source_text = document.source_editor.GetText();

        std::string source_error;
        if (!source_runner.compile(source_text, source_error))
        {
            document.compile_error = source_error;
            int line = parse_error_line_number(source_error) - ShaderRunner::fragment_header_lines();
            TextEditor::ErrorMarkers markers;
            if (line > 0)
            {
                markers[line] = source_error;
            }
            document.source_editor.SetErrorMarkers(markers);
            std::printf("shader compile/link failed:\n%s\n", source_error.c_str());
            std::fflush(stdout);
            document.equivalence_result = EquivalenceRunResult{};
            return;
        }
        document.source_editor.SetErrorMarkers(TextEditor::ErrorMarkers());

        compute_golf_engine(document);

        const std::string& to_compile = document.golfed_text.empty() ? source_text : document.golfed_text;
        std::string golf_error;
        if (!golfed_runner.compile(to_compile, golf_error))
        {
            document.compile_error = golf_error;
            std::printf("shader compile/link failed:\n%s\n", golf_error.c_str());
            std::fflush(stdout);
            document.equivalence_result = EquivalenceRunResult{};
        }
        else
        {
            document.compile_error.clear();
            int window_pixel_w = 0;
            int window_pixel_h = 0;
            SDL_GetWindowSizeInPixels(window, &window_pixel_w, &window_pixel_h);
            document.equivalence_result = run_equivalence_check(
                source_runner, golfed_runner, equivalence_source_fb, equivalence_golfed_fb,
                equivalence_config, window_pixel_w, window_pixel_h);
        }
    };

    auto sync_active_gl = [&](Document& document)
    {
        std::string source_text = document.source_editor.GetText();
        std::string source_error;
        if (!source_runner.compile(source_text, source_error))
        {
            document.compile_error = source_error;
            int line = parse_error_line_number(source_error) - ShaderRunner::fragment_header_lines();
            TextEditor::ErrorMarkers markers;
            if (line > 0)
            {
                markers[line] = source_error;
            }
            document.source_editor.SetErrorMarkers(markers);
            return;
        }
        document.source_editor.SetErrorMarkers(TextEditor::ErrorMarkers());

        const std::string& to_compile = document.golfed_text.empty() ? source_text : document.golfed_text;
        std::string golf_error;
        if (!golfed_runner.compile(to_compile, golf_error))
        {
            document.compile_error = golf_error;
        }
        else
        {
            document.compile_error.clear();
        }
    };

    auto add_document = [&](std::unique_ptr<Document> document)
    {
        document->tab_id = next_tab_id++;
        Document& reference = *document;
        documents.push_back(std::move(document));
        active_index = static_cast<int>(documents.size()) - 1;
        pending_select_id = reference.tab_id;
        run_full_golf(reference);
        last_synced_tab_id = reference.tab_id;
    };

    auto new_document = [&]()
    {
        add_document(make_default_document());
    };

    auto open_document = [&]()
    {
        std::optional<std::string> path = show_open_file_dialog(window, kGlslFilter, L"glsl");
        if (path.has_value())
        {
            add_document(make_document(read_utf8_file(*path), *path));
            add_recent_file(*path);
            recent_files = load_recent_files();
        }
    };

    auto open_document_at_path = [&](const std::string& path)
    {
        if (!file_exists(path))
        {
            remove_recent_file(path);
            recent_files = load_recent_files();
            return;
        }
        add_document(make_document(read_utf8_file(path), path));
        add_recent_file(path);
        recent_files = load_recent_files();
    };

    auto has_glsl_extension = [](const std::string& path) -> bool
    {
        const std::string suffix = ".glsl";
        if (path.size() < suffix.size())
        {
            return false;
        }
        std::string tail = path.substr(path.size() - suffix.size());
        for (char& c : tail)
        {
            if (c >= 'A' && c <= 'Z')
            {
                c = static_cast<char>(c - 'A' + 'a');
            }
        }
        return tail == suffix;
    };

    auto save_document = [&](Document& document) -> bool
    {
        std::string path = document.file_path;
        if (path.empty())
        {
            std::optional<std::string> chosen = show_save_file_dialog(window, kGlslFilter, L"glsl", L"shader.glsl");
            if (!chosen.has_value())
            {
                return false;
            }
            path = *chosen;
        }
        std::string text = document.source_editor.GetText();
        if (!write_utf8_file(path, text))
        {
            return false;
        }
        document.file_path = path;
        document.display_name = document_display_name(path);
        document.saved_source = text;
        document.dirty = false;
        add_recent_file(path);
        recent_files = load_recent_files();
        return true;
    };

    auto save_document_as = [&](Document& document) -> bool
    {
        std::wstring default_name = document.file_path.empty()
            ? std::wstring(L"shader.glsl")
            : utf8_to_wide(document_display_name(document.file_path));
        std::optional<std::string> chosen = show_save_file_dialog(window, kGlslFilter, L"glsl", default_name.c_str());
        if (!chosen.has_value())
        {
            return false;
        }
        std::string text = document.source_editor.GetText();
        if (!write_utf8_file(*chosen, text))
        {
            return false;
        }
        document.file_path = *chosen;
        document.display_name = document_display_name(*chosen);
        document.saved_source = text;
        document.dirty = false;
        add_recent_file(*chosen);
        recent_files = load_recent_files();
        return true;
    };

    auto index_of_tab = [&](int tab_id) -> int
    {
        for (int i = 0; i < static_cast<int>(documents.size()); ++i)
        {
            if (documents[i]->tab_id == tab_id)
            {
                return i;
            }
        }
        return -1;
    };

    auto remove_document = [&](int index)
    {
        if (index < 0 || index >= static_cast<int>(documents.size()))
        {
            return;
        }
        documents.erase(documents.begin() + index);
        if (documents.empty())
        {
            new_document();
            return;
        }
        if (active_index >= static_cast<int>(documents.size()))
        {
            active_index = static_cast<int>(documents.size()) - 1;
        }
        pending_select_id = documents[active_index]->tab_id;
    };

    documents.push_back(make_default_document());
    documents[0]->tab_id = next_tab_id++;
    active_index = 0;
    run_full_golf(*documents[0]);
    last_synced_tab_id = documents[0]->tab_id;

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
        std::vector<std::string> dropped_file_paths;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                exit_requested = true;
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
            else if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr)
            {
                dropped_file_paths.push_back(event.drop.data);
            }
            else if (event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED)
            {
                float new_dpi_scale = SDL_GetWindowDisplayScale(window);
                if (new_dpi_scale > 0.0f)
                {
                    dpi_scale = new_dpi_scale;
                    apply_dpi_scale(io, dpi_scale * (ui_font_size / kDefaultBaseFontSize));
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        accessibility_begin_frame();

        bool window_minimized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0;

        if (exit_requested && window_minimized)
        {
            running = false;
            exit_requested = false;
        }

        int window_pixel_w = 0;
        int window_pixel_h = 0;
        SDL_GetWindowSizeInPixels(window, &window_pixel_w, &window_pixel_h);

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (!window_minimized && main_viewport->WorkSize.x >= 1.0f && main_viewport->WorkSize.y >= 1.0f)
        {

        ImGuiWindowFlags chrome_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                         ImGuiWindowFlags_NoNavFocus;

        ImGui::SetNextWindowPos(main_viewport->WorkPos);
        ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x, kTitleBarHeight));
        ImGui::SetNextWindowViewport(main_viewport->ID);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, tokens::bg_panel_raised);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 0.0f));
        ImGui::Begin("##TitleBar", nullptr, chrome_flags);

        if (app_icon_texture.id != 0)
        {
            ImGui::SetCursorPosY((kTitleBarHeight - 16.0f) * 0.5f);
            ImGui::Image(static_cast<ImTextureID>(app_icon_texture.id), ImVec2(16.0f, 16.0f));
            ImGui::SameLine(0.0f, 8.0f);
        }
        ImGui::SetCursorPosY((kTitleBarHeight - ImGui::GetFontSize()) * 0.5f);
        ImGui::TextColored(tokens::text_secondary, "uShader " USHADER_VERSION_STRING);

        const float button_width = 46.0f;
        const float button_height = kTitleBarHeight;
        ImGui::SameLine(main_viewport->WorkSize.x - button_width * 3.0f - 8.0f);
        ImGui::SetCursorPosY(0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

        ImVec2 minimize_screen_pos = ImGui::GetCursorScreenPos();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens::bg_hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, tokens::bg_active);
        ImGui::PushFont(g_icon_font);
        bool minimize_clicked = ImGui::Button(ICON_MINUS, ImVec2(button_width, button_height));
        ImGui::PopFont();
        hit_test_state.minimize_rect = SDL_Rect{
            static_cast<int>(minimize_screen_pos.x), static_cast<int>(minimize_screen_pos.y),
            static_cast<int>(button_width), static_cast<int>(button_height)};

        ImGui::SameLine(0.0f, 0.0f);
        ImVec2 maximize_screen_pos = ImGui::GetCursorScreenPos();
        ImGui::PushFont(g_icon_font);
        bool maximize_clicked = ImGui::Button(ICON_SQUARE, ImVec2(button_width, button_height));
        ImGui::PopFont();
        hit_test_state.maximize_rect = SDL_Rect{
            static_cast<int>(maximize_screen_pos.x), static_cast<int>(maximize_screen_pos.y),
            static_cast<int>(button_width), static_cast<int>(button_height)};
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0.0f, 0.0f);
        ImVec2 close_screen_pos = ImGui::GetCursorScreenPos();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tokens::status_error);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, tokens::status_error);
        ImGui::PushFont(g_icon_font);
        bool close_clicked = ImGui::Button(ICON_X, ImVec2(button_width, button_height));
        ImGui::PopFont();
        ImGui::PopStyleColor(3);
        hit_test_state.close_rect = SDL_Rect{
            static_cast<int>(close_screen_pos.x), static_cast<int>(close_screen_pos.y),
            static_cast<int>(button_width), static_cast<int>(button_height)};

        ImGui::PopStyleVar();

        bool window_is_maximized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != 0;
        accessibility_register("Minimize", AccessibleRole::Button, minimize_screen_pos.x, minimize_screen_pos.y, button_width, button_height, true);
        accessibility_register(window_is_maximized ? "Restore" : "Maximize", AccessibleRole::Button, maximize_screen_pos.x, maximize_screen_pos.y, button_width, button_height, true);
        accessibility_register("Close", AccessibleRole::Button, close_screen_pos.x, close_screen_pos.y, button_width, button_height, true);

        if (minimize_clicked)
        {
            SDL_MinimizeWindow(window);
        }
        if (maximize_clicked)
        {
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED)
            {
                SDL_RestoreWindow(window);
            }
            else
            {
                SDL_MaximizeWindow(window);
            }
        }
        if (close_clicked)
        {
            exit_requested = true;
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImVec2 host_pos(main_viewport->WorkPos.x, main_viewport->WorkPos.y + kTitleBarHeight);
        ImVec2 host_size(main_viewport->WorkSize.x, main_viewport->WorkSize.y - kTitleBarHeight);
        ImGui::SetNextWindowPos(host_pos);
        ImGui::SetNextWindowSize(host_size);
        ImGui::SetNextWindowViewport(main_viewport->ID);

        ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                       ImGuiWindowFlags_MenuBar;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("##DockHost", nullptr, host_flags);
        ImGui::PopStyleVar();

        bool menu_new = false;
        bool menu_open = false;
        bool menu_save = false;
        bool menu_save_as = false;
        bool menu_export_report = false;
        bool menu_close = false;
        std::string menu_open_recent_path;
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                menu_new = ImGui::MenuItem("New tab", "Ctrl+N");
                menu_open = ImGui::MenuItem("Open...", "Ctrl+O");
                if (ImGui::BeginMenu("Recent Files", !recent_files.empty()))
                {
                    for (const std::string& recent_path : recent_files)
                    {
                        if (ImGui::MenuItem(document_display_name(recent_path).c_str()))
                        {
                            menu_open_recent_path = recent_path;
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("%s", recent_path.c_str());
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Clear Recent Files"))
                    {
                        clear_recent_files();
                        recent_files.clear();
                    }
                    ImGui::EndMenu();
                }
                menu_save = ImGui::MenuItem("Save", "Ctrl+S");
                menu_save_as = ImGui::MenuItem("Save as...");
                ImGui::Separator();
                menu_export_report = ImGui::MenuItem("Export report...");
                ImGui::Separator();
                menu_close = ImGui::MenuItem("Close tab", "Ctrl+W");
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    exit_requested = true;
                }
                ImGui::EndMenu();
            }
            ImGui::PushFont(g_icon_font);
            ImGui::Text(ICON_INFO);
            ImGui::PopFont();
            ImGui::SameLine(0.0f, 4.0f);
            if (ImGui::MenuItem("About"))
            {
                show_about = true;
            }
            ImGui::EndMenuBar();
        }

        int selected_index = active_index;
        int close_request_index = -1;
        bool new_tab_requested = false;
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_TabListPopupButton;
        if (ImGui::BeginTabBar("##document_tabs", tab_bar_flags))
        {
            for (int i = 0; i < static_cast<int>(documents.size()); ++i)
            {
                Document& tab_document = *documents[i];
                ImGuiTabItemFlags item_flags = 0;
                if (tab_document.dirty)
                {
                    item_flags |= ImGuiTabItemFlags_UnsavedDocument;
                }
                if (pending_select_id == tab_document.tab_id)
                {
                    item_flags |= ImGuiTabItemFlags_SetSelected;
                }
                bool open = true;
                char label[300];
                std::snprintf(label, sizeof(label), "%s###doc%d", tab_document.display_name.c_str(), tab_document.tab_id);
                if (ImGui::BeginTabItem(label, &open, item_flags))
                {
                    selected_index = i;
                    ImGui::EndTabItem();
                }
                if (!open)
                {
                    close_request_index = i;
                }
            }
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
            {
                new_tab_requested = true;
            }
            ImGui::EndTabBar();
        }
        pending_select_id = 0;
        active_index = selected_index;

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        bool narrow = main_viewport->WorkSize.x < 900.0f;
        if ((!layout_built && !had_layout_ini) || narrow != last_narrow)
        {
            build_dock_layout(dockspace_id, narrow);
        }
        layout_built = true;
        last_narrow = narrow;
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
        ImGui::End();

        Document& active = *documents[active_index];
        if (active.tab_id != last_synced_tab_id)
        {
            sync_active_gl(active);
            last_synced_tab_id = active.tab_id;
        }

        float ui_font_size_before = ui_font_size;
        render_about_popup(show_about, logo_texture, keybindings, ui_font_size);
        if (ui_font_size != ui_font_size_before)
        {
            apply_dpi_scale(io, dpi_scale * (ui_font_size / kDefaultBaseFontSize));
        }

        if (chord_pressed(io, keybindings.command_palette))
        {
            command_palette_open = true;
        }
        if (chord_pressed(io, keybindings.new_tab))
        {
            menu_new = true;
        }
        if (chord_pressed(io, keybindings.open_file))
        {
            menu_open = true;
        }
        if (chord_pressed(io, keybindings.save_file))
        {
            menu_save = true;
        }
        if (chord_pressed(io, keybindings.close_tab))
        {
            menu_close = true;
        }

        std::vector<PaletteCommand> palette_commands = {
            {"Run golf", [&]() { run_full_golf(active); }},
            {"New tab", [&]() { new_tab_requested = true; }},
            {"Open file...", [&]() { menu_open = true; }},
            {"Save", [&]() { menu_save = true; }},
            {"Save as...", [&]() { menu_save_as = true; }},
            {"Toggle pass: Aggressive golf", [&]() { active.pass_toggles.aggressive = !active.pass_toggles.aggressive; }},
            {"Toggle pass: Dead locals", [&]() { active.pass_toggles.eliminate_dead_locals = !active.pass_toggles.eliminate_dead_locals; }},
            {"Toggle pass: Dead stores", [&]() { active.pass_toggles.eliminate_dead_stores = !active.pass_toggles.eliminate_dead_stores; }},
            {"Toggle pass: Fold constants", [&]() { active.pass_toggles.fold_constants = !active.pass_toggles.fold_constants; }},
            {"Toggle pass: Constant vectors", [&]() { active.pass_toggles.reduce_constant_vectors = !active.pass_toggles.reduce_constant_vectors; }},
            {"Toggle pass: Trailing return", [&]() { active.pass_toggles.strip_trailing_void_return = !active.pass_toggles.strip_trailing_void_return; }},
            {"Toggle pass: Compound assign", [&]() { active.pass_toggles.compound_assignments = !active.pass_toggles.compound_assignments; }},
            {"Toggle pass: Increment/decrement", [&]() { active.pass_toggles.increment_decrement = !active.pass_toggles.increment_decrement; }},
            {"Toggle pass: Ternary", [&]() { active.pass_toggles.ternary_from_if_else = !active.pass_toggles.ternary_from_if_else; }},
            {"Toggle pass: Merge declarations", [&]() { active.pass_toggles.merge_declarations = !active.pass_toggles.merge_declarations; }},
            {"Toggle pass: Redundant braces", [&]() { active.pass_toggles.strip_redundant_braces = !active.pass_toggles.strip_redundant_braces; }},
            {"Toggle pass: Redundant parens", [&]() { active.pass_toggles.strip_redundant_parens = !active.pass_toggles.strip_redundant_parens; }},
            {"Toggle pass: Duplicate precision", [&]() { active.pass_toggles.strip_duplicate_precision = !active.pass_toggles.strip_duplicate_precision; }},
            {"Toggle pass: Dead functions", [&]() { active.pass_toggles.eliminate_dead_functions = !active.pass_toggles.eliminate_dead_functions; }},
            {"Toggle pass: Inline single-call", [&]() { active.pass_toggles.inline_single_call_functions = !active.pass_toggles.inline_single_call_functions; }},
            {"Toggle pass: Algebraic identities", [&]() { active.pass_toggles.simplify_algebraic_identities = !active.pass_toggles.simplify_algebraic_identities; }},
            {"Toggle pass: Common subexpressions", [&]() { active.pass_toggles.eliminate_common_subexpressions = !active.pass_toggles.eliminate_common_subexpressions; }},
            {"Save profile...", [&]() { save_golf_profile_action(active.pass_toggles, active.protected_names, active.budget_preset_index, last_profile_path, window); }},
            {"Load profile...", [&]() { load_golf_profile_action(active.pass_toggles, active.protected_names, active.budget_preset_index, last_profile_path, window); }},
            {"Toggle Compare mode", [&]() { compare_mode = !compare_mode; }},
            {"Export golfed shader (Shadertoy)...", [&]() {
                std::optional<std::string> path = show_save_file_dialog(window, kGlslFilter, L"glsl", L"shader.glsl");
                if (path.has_value())
                {
                    write_utf8_file(*path, active.golfed_text);
                }
            }},
            {"Copy golfed shader to clipboard", [&]() { SDL_SetClipboardText(active.golfed_text.c_str()); }},
            {"Copy as Shadertoy mainImage", [&]() { SDL_SetClipboardText(wrap_as_shadertoy_main_image(active.golfed_text).c_str()); }},
            {"Copy as Bonzomatic-ready source", [&]() { SDL_SetClipboardText(wrap_as_bonzomatic_source(active.golfed_text).c_str()); }},
            {"Copy as bare void main()", [&]() { SDL_SetClipboardText(wrap_as_bare_main(active.golfed_text).c_str()); }},
            {"Toggle Minimap", [&]() { minimap_settings.enabled = !minimap_settings.enabled; }},
            {"Show Diff panel", [&]() { ImGui::SetWindowFocus(kDiffWindowTitle); }},
            {"Export report...", [&]() { menu_export_report = true; }},
        };
        for (std::size_t tab_index = 0; tab_index < documents.size(); ++tab_index)
        {
            palette_commands.push_back({"Switch to tab: " + documents[tab_index]->display_name, [&, tab_index]() { active_index = static_cast<int>(tab_index); }});
        }
        render_command_palette(command_palette_open, palette_commands);

        ImGui::Begin(kSourceWindowTitle);
        if (icon_button_ex(ICON_PLAY, "Run golf", true))
        {
            run_full_golf(active);
        }
        if (active.equivalence_result.valid)
        {
            bool equivalence_passed = active.equivalence_result.samples_failed == 0;
            StatusKind equivalence_kind = equivalence_passed ? StatusKind::Ok : StatusKind::Error;
            ImGui::SameLine(0.0f, 16.0f);
            render_status_dot(equivalence_passed ? "Equivalent" : "Differs", equivalence_kind);
        }
        ImGui::SameLine();
        if (icon_button(ICON_FOLDER_OPEN, "Open"))
        {
            menu_open = true;
        }
        ImGui::SameLine();
        if (icon_button(ICON_SAVE, "Save"))
        {
            menu_save = true;
        }
        ImGui::SameLine();
        themed_checkbox("Minimap", &minimap_settings.enabled);
        if (active.equivalence_result.valid && active.equivalence_result.samples_failed > 0)
        {
            ImGui::TextColored(tokens::status_error, "%d/%d samples differ, max delta %d",
                active.equivalence_result.samples_failed, active.equivalence_result.samples_total, active.equivalence_result.worst_max_delta);
        }
        render_golf_controls(active.pass_toggles, active.protected_names, active.budget_preset_index, last_profile_path, window);
        if (ImGui::CollapsingHeader("EQUIVALENCE"))
        {
            render_equivalence_tolerance_control(equivalence_config);
        }
        ImGui::Separator();
        if (minimap_should_render(active.source_editor.GetTotalLines(), minimap_settings))
        {
            float editor_height = ImGui::GetContentRegionAvail().y;
            float editor_width = ImGui::GetContentRegionAvail().x - minimap_settings.width - ImGui::GetStyle().ItemSpacing.x;
            active.source_editor.Render("##source", ImVec2(editor_width, editor_height));
            ImGui::SameLine();
            render_minimap("##source_minimap", active.source_editor.GetText(), active.source_editor.GetPalette(), editor_height, minimap_settings.width);
        }
        else
        {
            active.source_editor.Render("##source");
        }
        active.dirty = active.source_editor.GetText() != active.saved_source;
        ImGui::End();

        ImGui::Begin(kGolfedWindowTitle);
        if (themed_checkbox("Formatted view", &active.formatted_view))
        {
            active.golfed_editor.SetText(active.formatted_view ? format_glsl(active.golfed_text) : active.golfed_text);
        }
        ImGui::SameLine();
        if (icon_button(ICON_COPY, "Copy"))
        {
            SDL_SetClipboardText(active.golfed_text.c_str());
        }
        ImGui::SameLine();
        if (icon_button(ICON_DOWNLOAD, "Export (Shadertoy)"))
        {
            std::optional<std::string> path = show_save_file_dialog(window, kGlslFilter, L"glsl", L"shader.glsl");
            if (path.has_value())
            {
                write_utf8_file(*path, active.golfed_text);
            }
        }
        ImGui::SameLine();
        if (icon_button(ICON_COPY, "Copy as Shadertoy"))
        {
            SDL_SetClipboardText(wrap_as_shadertoy_main_image(active.golfed_text).c_str());
        }
        ImGui::SameLine();
        if (icon_button(ICON_COPY, "Copy as Bonzomatic"))
        {
            SDL_SetClipboardText(wrap_as_bonzomatic_source(active.golfed_text).c_str());
        }
        ImGui::SameLine();
        if (icon_button(ICON_COPY, "Copy as bare main()"))
        {
            SDL_SetClipboardText(wrap_as_bare_main(active.golfed_text).c_str());
        }
        render_stats_panel(active.golf_stats, active.golfed_text.size(), active.budget_result, active.budget_preset_index);
        ImGui::Separator();
        if (minimap_should_render(active.golfed_editor.GetTotalLines(), minimap_settings))
        {
            float editor_height = ImGui::GetContentRegionAvail().y;
            float editor_width = ImGui::GetContentRegionAvail().x - minimap_settings.width - ImGui::GetStyle().ItemSpacing.x;
            active.golfed_editor.Render("##golfed", ImVec2(editor_width, editor_height));
            ImGui::SameLine();
            render_minimap("##golfed_minimap", active.golfed_editor.GetText(), active.golfed_editor.GetPalette(), editor_height, minimap_settings.width);
        }
        else
        {
            active.golfed_editor.Render("##golfed");
        }
        ImGui::End();

        ImGui::Begin(kTraceWindowTitle);
        if (active.golf_trace.empty())
        {
            ImGui::TextColored(tokens::text_secondary, "Run golf to see the pass-by-pass trace.");
        }
        for (std::size_t trace_index = 0; trace_index < active.golf_trace.size(); ++trace_index)
        {
            const GolfTraceStep& step = active.golf_trace[trace_index];
            bool changed = step.count > 0;

            ImGui::PushID(static_cast<int>(trace_index));
            ImGui::PushStyleColor(ImGuiCol_Text, changed ? tokens::text_primary : tokens::text_disabled);
            std::string header_label = step.pass_name + " (" + std::to_string(step.count)
                + (step.count == 1 ? " change)" : " changes)");
            bool open = ImGui::CollapsingHeader(header_label.c_str());
            ImGui::PopStyleColor();

            if (open)
            {
                ImVec2 trace_avail = ImGui::GetContentRegionAvail();
                float trace_half_w = (trace_avail.x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
                float trace_height = 220.0f;

                ImGui::BeginChild("##trace_before_pane", ImVec2(trace_half_w, trace_height), true);
                ImGui::TextColored(tokens::text_secondary, "Before");
                trace_before_editor.SetText(step.before);
                trace_before_editor.Render("##trace_before");
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("##trace_after_pane", ImVec2(trace_half_w, trace_height), true);
                ImGui::TextColored(tokens::text_secondary, "After");
                trace_after_editor.SetText(step.after);
                trace_after_editor.Render("##trace_after");
                ImGui::EndChild();
            }
            ImGui::PopID();
        }
        ImGui::End();

        ImGui::Begin(kDiffWindowTitle);
        if (active.diff_spans.empty())
        {
            ImGui::TextColored(tokens::text_secondary, "Run golf to see the Source/Golfed diff.");
        }
        else
        {
            ImGui::TextColored(tokens::status_error, "Removed");
            ImGui::SameLine();
            ImGui::TextColored(tokens::text_secondary, "/");
            ImGui::SameLine();
            ImGui::TextColored(tokens::status_ok, "Added");
            ImGui::Separator();
            ImGui::BeginChild("##diff_scroll");
            render_diff_view(active.diff_spans);
            ImGui::EndChild();
        }
        ImGui::End();

        ImGui::Begin(kViewportWindowTitle);
        themed_checkbox("Compare", &compare_mode);
        ImGui::SameLine();
        if (icon_button(ICON_CAMERA, "Screenshot"))
        {
            std::optional<std::string> path = show_save_file_dialog(window, kPngFilter, L"png", L"screenshot.png");
            if (path.has_value())
            {
                save_framebuffer_png(golfed_viewport_fb, *path);
            }
        }

        ImGui::SameLine();
        themed_checkbox("Include screenshot in report", &report_include_screenshot);

        ImGui::SameLine();
        static const char* kRecordFormatLabels[] = {"GIF", "MP4", "WebM"};
        int record_format_index = static_cast<int>(record_format);
        ImGui::SetNextItemWidth(80.0f);
        ImGui::BeginDisabled(recorder.is_recording());
        if (ImGui::Combo("##record_format", &record_format_index, kRecordFormatLabels, 3))
        {
            record_format = static_cast<RecordingFormat>(record_format_index);
        }
        ImGui::EndDisabled();

        bool record_needs_ffmpeg = record_format != RecordingFormat::Gif;
        bool record_disabled = record_needs_ffmpeg && !ffmpeg_available();

        ImGui::SameLine();
        ImGui::BeginDisabled(record_disabled);
        if (!recorder.is_recording())
        {
            if (icon_button(ICON_CIRCLE, "Record"))
            {
                start_recording_requested = true;
            }
        }
        else
        {
            if (icon_button(ICON_CIRCLE_STOP, "Stop"))
            {
                const wchar_t* filter = record_format == RecordingFormat::Gif ? kGifFilter
                    : record_format == RecordingFormat::Mp4 ? kMp4Filter : kWebMFilter;
                const wchar_t* ext = record_format == RecordingFormat::Gif ? L"gif"
                    : record_format == RecordingFormat::Mp4 ? L"mp4" : L"webm";
                const wchar_t* default_name = record_format == RecordingFormat::Gif ? L"recording.gif"
                    : record_format == RecordingFormat::Mp4 ? L"recording.mp4" : L"recording.webm";
                std::optional<std::string> path = show_save_file_dialog(window, filter, ext, default_name);
                if (path.has_value())
                {
                    recorder.stop(*path);
                }
                else
                {
                    recorder.cancel();
                }
            }
        }
        ImGui::EndDisabled();
        if (record_disabled && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Bundled ffmpeg.exe not found next to the application");
        }
        if (recorder.is_recording())
        {
            ImGui::SameLine();
            ImGui::TextColored(tokens::status_error, "REC %d frames", recorder.frame_count());
        }

        ImGui::SameLine(0.0f, 16.0f);
        float elapsed_seconds = static_cast<float>(SDL_GetTicks() - start_ticks) / 1000.0f;
        if (g_mono_font != nullptr)
        {
            ImGui::PushFont(g_mono_font);
        }
        ImGui::TextColored(tokens::text_secondary, "%s", format_timecode(elapsed_seconds).c_str());
        if (g_mono_font != nullptr)
        {
            ImGui::PopFont();
        }

        {
            StatusKind compile_kind = active.compile_error.empty() ? StatusKind::Ok : StatusKind::Error;
            render_status_dot(active.compile_error.empty() ? "Compiled" : "Shader error", compile_kind);
        }

        if (!active.compile_error.empty())
        {
            int display_line = parse_error_line_number(active.compile_error) - ShaderRunner::fragment_header_lines();
            ImGui::TextWrapped("%s%s", error_line_prefix(display_line).c_str(), active.compile_error.c_str());
        }

        ImGui::Separator();

        ImVec2 avail = ImGui::GetContentRegionAvail();

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

        ImGui::PushStyleColor(ImGuiCol_ChildBg, tokens::bg_field_deepest);
        ImGui::BeginChild("##viewport_letterbox", avail, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

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

        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (start_recording_requested)
        {
            recorder.start(record_format, golfed_viewport_fb.width(), golfed_viewport_fb.height(), kRecordFps);
            start_recording_requested = false;
            last_capture_ticks = 0;
        }

        if (recorder.is_recording())
        {
            Uint64 capture_interval_ms = static_cast<Uint64>(1000 / kRecordFps);
            if (now_ticks - last_capture_ticks >= capture_interval_ms)
            {
                std::vector<unsigned char> pixels;
                if (golfed_viewport_fb.read_pixels(pixels))
                {
                    recorder.add_frame(pixels.data(), golfed_viewport_fb.width(), golfed_viewport_fb.height());
                }
                last_capture_ticks = now_ticks;
            }
        }

        ImGui::End();

        if (menu_new)
        {
            new_tab_requested = true;
        }
        if (menu_open)
        {
            open_document();
        }
        if (!menu_open_recent_path.empty())
        {
            open_document_at_path(menu_open_recent_path);
        }
        for (const std::string& dropped_path : dropped_file_paths)
        {
            if (has_glsl_extension(dropped_path))
            {
                open_document_at_path(dropped_path);
            }
        }
        if (menu_save)
        {
            save_document(*documents[active_index]);
        }
        if (menu_save_as)
        {
            save_document_as(*documents[active_index]);
        }
        if (menu_export_report)
        {
            std::vector<unsigned char> report_screenshot_png;
            if (report_include_screenshot)
            {
                encode_framebuffer_png_to_memory(golfed_viewport_fb, report_screenshot_png);
            }
            ReportOptions report_options;
            report_options.include_screenshot = report_include_screenshot;
            export_report_action(
                active.display_name, active.source_editor.GetText(), active.golfed_text,
                active.golf_stats, active.budget_result, active.budget_preset_index,
                active.equivalence_result, report_options, report_screenshot_png, window);
        }
        if (menu_close)
        {
            close_request_index = active_index;
        }

        if (close_request_index >= 0 && close_request_index < static_cast<int>(documents.size()))
        {
            if (documents[close_request_index]->dirty)
            {
                close_confirm_tab_id = documents[close_request_index]->tab_id;
            }
            else
            {
                remove_document(close_request_index);
            }
        }

        if (new_tab_requested)
        {
            new_document();
        }

        if (restore_prompt_open)
        {
            ImGui::OpenPopup("Restore last session?");
        }
        if (ImGui::BeginPopupModal("Restore last session?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Reopen the shaders from your last session?");
            ImGui::TextColored(tokens::text_secondary, "%d file(s) will be restored.",
                static_cast<int>(restore_state.documents.size()));
            ImGui::Separator();
            if (ImGui::Button("Restore"))
            {
                documents.clear();
                for (const WorkspaceDocument& saved : restore_state.documents)
                {
                    std::unique_ptr<Document> restored = make_document(read_utf8_file(saved.file_path), saved.file_path);
                    restored->tab_id = next_tab_id++;
                    restored->pass_toggles = saved.pass_toggles;
                    restored->protected_names = saved.protected_names;
                    restored->budget_preset_index = saved.budget_preset_index;
                    documents.push_back(std::move(restored));
                }
                if (documents.empty())
                {
                    documents.push_back(make_default_document());
                    documents.back()->tab_id = next_tab_id++;
                }
                int restored_active = restore_state.active_tab;
                if (restored_active < 0 || restored_active >= static_cast<int>(documents.size()))
                {
                    restored_active = 0;
                }
                active_index = restored_active;
                for (int i = 0; i < static_cast<int>(documents.size()); ++i)
                {
                    if (i == active_index)
                    {
                        run_full_golf(*documents[i]);
                    }
                    else
                    {
                        compute_golf_engine(*documents[i]);
                    }
                }
                last_synced_tab_id = documents[active_index]->tab_id;
                pending_select_id = documents[active_index]->tab_id;
                restore_prompt_open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Start fresh"))
            {
                restore_prompt_open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (close_confirm_tab_id != -1)
        {
            ImGui::OpenPopup("Discard changes?");
        }
        if (ImGui::BeginPopupModal("Discard changes?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            int confirm_index = index_of_tab(close_confirm_tab_id);
            if (confirm_index < 0)
            {
                close_confirm_tab_id = -1;
                ImGui::CloseCurrentPopup();
            }
            else
            {
                ImGui::Text("\"%s\" has unsaved changes.", documents[confirm_index]->display_name.c_str());
                ImGui::Separator();
                if (ImGui::Button("Save"))
                {
                    if (save_document(*documents[confirm_index]))
                    {
                        remove_document(confirm_index);
                    }
                    close_confirm_tab_id = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Discard"))
                {
                    remove_document(confirm_index);
                    close_confirm_tab_id = -1;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    close_confirm_tab_id = -1;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        if (exit_requested)
        {
            exit_requested = false;
            bool any_dirty = false;
            for (const std::unique_ptr<Document>& document : documents)
            {
                if (document->dirty)
                {
                    any_dirty = true;
                    break;
                }
            }
            if (any_dirty)
            {
                exit_confirm_open = true;
            }
            else
            {
                running = false;
            }
        }

        if (exit_confirm_open)
        {
            ImGui::OpenPopup("Unsaved shaders");
        }
        if (ImGui::BeginPopupModal("Unsaved shaders", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("The following shaders have unsaved changes:");
            for (const std::unique_ptr<Document>& document : documents)
            {
                if (document->dirty)
                {
                    ImGui::BulletText("%s", document->display_name.c_str());
                }
            }
            ImGui::Separator();
            if (ImGui::Button("Save all and exit"))
            {
                bool all_saved = true;
                for (const std::unique_ptr<Document>& document : documents)
                {
                    if (document->dirty && !save_document(*document))
                    {
                        all_saved = false;
                        break;
                    }
                }
                if (all_saved)
                {
                    running = false;
                }
                exit_confirm_open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Discard and exit"))
            {
                running = false;
                exit_confirm_open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                exit_confirm_open = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        }

        accessibility_end_frame();
        ImGui::Render();

        glViewport(0, 0, window_pixel_w, window_pixel_h);
        glClearColor(tokens::bg_app.x, tokens::bg_app.y, tokens::bg_app.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    if (!layout_ini_path.empty())
    {
        ImGui::SaveIniSettingsToDisk(layout_ini_path.c_str());
    }

    std::string session_path = workspace_session_path();
    if (!session_path.empty())
    {
        WorkspaceState session_state;
        session_state.layout_ini = workspace_layout_name();
        session_state.ui_font_size = ui_font_size;
        int serialized_active = 0;
        int serialized_count = 0;
        for (int i = 0; i < static_cast<int>(documents.size()); ++i)
        {
            if (documents[i]->file_path.empty())
            {
                continue;
            }
            WorkspaceDocument saved;
            saved.file_path = documents[i]->file_path;
            saved.pass_toggles = documents[i]->pass_toggles;
            saved.protected_names = documents[i]->protected_names;
            saved.budget_preset_index = documents[i]->budget_preset_index;
            session_state.documents.push_back(saved);
            if (i == active_index)
            {
                serialized_active = serialized_count;
            }
            ++serialized_count;
        }
        session_state.active_tab = serialized_active;
        write_utf8_file(session_path, serialize_workspace(session_state));
    }

    destroy_texture(logo_texture);
    destroy_texture(app_icon_texture);
    source_viewport_fb.destroy();
    golfed_viewport_fb.destroy();
    equivalence_source_fb.destroy();
    equivalence_golfed_fb.destroy();
    source_runner.destroy();
    golfed_runner.destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    accessibility_shutdown();
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
