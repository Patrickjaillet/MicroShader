#include "render/wgl_viewport_host.h"
#include "render/gl_functions.h"
#include "render/shader_runner.h"
#include "render/framebuffer.h"
#include "render/shader_error_state.h"
#include "render/default_shader.h"
#include "ui/win32_theme_brushes.h"
#include "ui/win32_title_bar.h"
#include "ui/win32_tab_strip.h"
#include "ui/win32_icon_set.h"
#include "ui/win32_status_dot.h"
#include "ui/win32_text_editor.h"
#include "ui/win32_minimap.h"
#include "ui/win32_diff_view.h"
#include "ui/win32_trace_view.h"
#include "ui/win32_command_palette.h"
#include "ui/win32_keybindings.h"
#include "ui/win32_golf_controls.h"
#include "ui/win32_stats_panel.h"
#include "ui/win32_appearance_panel.h"
#include "ui/win32_appearance_settings.h"
#include "ui/win32_about_panel.h"
#include "ui/theme_tokens.h"
#include "ui/glsl_format.h"
#include "ui/golf_trace.h"
#include "ui/recent_files.h"
#include "platform/utf8.h"
#include "platform/paths.h"
#include "platform/file_dialog.h"
#include "platform/accessibility_core.h"

#include "ushader/version.h"
#include "ushader/golf_core.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <gdiplus.h>
#include <GL/gl.h>

#include <chrono>
#include <string>
#include <optional>

namespace
{
    const wchar_t* kMainClassName = L"uShaderWin32Shell";
    constexpr int kResizeBorder = 6;
    constexpr float kShimmerHoldSeconds = 0.2f;
    constexpr int kSourceTabIndex = 0;
    constexpr int kGolfedTabIndex = 1;
    constexpr int kDiffTabIndex = 2;
    constexpr int kTraceTabIndex = 3;
    constexpr int kControlsTabIndex = 4;
    constexpr int kStatsTabIndex = 5;
    constexpr int kViewportTabIndex = 6;
    constexpr int kAppearanceTabIndex = 7;
    constexpr int kAboutTabIndex = 8;

    const wchar_t kGlslFilter[] = L"GLSL shaders (*.glsl)\0*.glsl\0All files (*.*)\0*.*\0";

    const char* kShimmerShaderSource =
        "void mainImage(out vec4 fragColor, in vec2 fragCoord)\n"
        "{\n"
        "    vec2 uv = fragCoord / iResolution.xy;\n"
        "    float diag = uv.x + uv.y;\n"
        "    float phase = fract(diag * 1.5 - iTime * 0.6);\n"
        "    float shimmer = smoothstep(0.0, 0.15, phase) * smoothstep(0.35, 0.15, phase);\n"
        "    vec3 base = vec3(0.14, 0.14, 0.14);\n"
        "    vec3 highlight = vec3(0.30, 0.30, 0.30);\n"
        "    fragColor = vec4(mix(base, highlight, shimmer), 1.0);\n"
        "}\n";

    ID2D1Factory* g_d2d_factory = nullptr;
    IDWriteFactory* g_dwrite_factory = nullptr;
    ID2D1HwndRenderTarget* g_render_target = nullptr;
    ID2D1SolidColorBrush* g_minimap_brush = nullptr;
    MinimapSettings g_minimap_settings;
    ThemeBrushes g_brushes;
    TitleBar g_title_bar;
    TabStrip g_tab_strip;
    IconSet g_icons;
    StatusDot g_status_dot;
    Win32TextEditor g_source_editor;
    Win32TextEditor g_golfed_editor;
    Win32DiffView g_diff_view;
    Win32TraceView g_trace_view;
    Win32CommandPalette g_command_palette;
    Win32Keybindings g_keybindings;
    Win32GolfControls g_golf_controls;
    Win32StatsPanel g_stats_panel;
    Win32AppearancePanel g_appearance_panel;
    bool g_appearance_slider_dragging = false;
    Win32AboutPanel g_about_panel;
    WglViewportHost g_viewport;
    ShaderRunner g_shader_runner;
    ShaderRunner g_golfed_runner;
    OffscreenFramebuffer g_compare_source_fb;
    OffscreenFramebuffer g_compare_golfed_fb;
    ShaderRunner g_shimmer_runner;
    bool g_gl_ready = false;
    bool g_golfed_gl_ready = false;
    bool g_compare_mode = false;
    bool g_shimmer_ready = false;
    bool g_window_focused = false;
    bool g_editor_dragging = false;
    bool g_formatted_view = false;
    std::string g_golfed_text_raw;
    std::chrono::steady_clock::time_point g_shimmer_until;
    ULONG_PTR g_gdiplus_token = 0;

    std::wstring exe_directory()
    {
        wchar_t path[MAX_PATH];
        DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring full(path, length);
        size_t slash = full.find_last_of(L"\\/");
        return slash == std::wstring::npos ? L"." : full.substr(0, slash);
    }

    Win32TextEditor* active_editor()
    {
        int active = g_tab_strip.active_index();
        if (active == kSourceTabIndex) { return &g_source_editor; }
        if (active == kGolfedTabIndex) { return &g_golfed_editor; }
        return nullptr;
    }

    void refresh_golfed_view()
    {
        g_golfed_editor.set_text_utf8(g_formatted_view ? format_glsl(g_golfed_text_raw) : g_golfed_text_raw);
    }

    bool create_device_resources(HWND hwnd)
    {
        RECT client_rect{};
        GetClientRect(hwnd, &client_rect);
        D2D1_SIZE_U size = D2D1::SizeU(
            static_cast<UINT32>(client_rect.right - client_rect.left),
            static_cast<UINT32>(client_rect.bottom - client_rect.top));

        D2D1_RENDER_TARGET_PROPERTIES rt_props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN),
            0.0f, 0.0f,
            D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);

        HRESULT hr = g_d2d_factory->CreateHwndRenderTarget(
            rt_props,
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &g_render_target);
        if (FAILED(hr))
        {
            return false;
        }

        if (!g_brushes.create(g_render_target))
        {
            return false;
        }
        if (FAILED(g_render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &g_minimap_brush)))
        {
            return false;
        }
        return g_status_dot.create(g_render_target);
    }

    void release_device_resources()
    {
        g_brushes.release();
        g_status_dot.destroy();
        if (g_minimap_brush != nullptr)
        {
            g_minimap_brush->Release();
            g_minimap_brush = nullptr;
        }
        if (g_render_target != nullptr)
        {
            g_render_target->Release();
            g_render_target = nullptr;
        }
    }

    void layout_chrome(HWND hwnd)
    {
        RECT client_rect{};
        GetClientRect(hwnd, &client_rect);
        int window_width = client_rect.right - client_rect.left;
        int window_height = client_rect.bottom - client_rect.top;

        g_title_bar.layout(window_width);
        g_tab_strip.layout(0, static_cast<int>(TitleBar::kHeight), window_width);

        int content_top = static_cast<int>(TitleBar::kHeight + TabStrip::kHeight);
        int content_height = window_height - content_top;
        if (content_height < 0)
        {
            content_height = 0;
        }

        int source_width = window_width;
        if (minimap_should_render(g_source_editor.line_count(), g_minimap_settings))
        {
            source_width -= static_cast<int>(g_minimap_settings.width);
        }
        g_source_editor.layout(0, content_top, source_width, content_height);

        int golfed_width = window_width;
        if (minimap_should_render(g_golfed_editor.line_count(), g_minimap_settings))
        {
            golfed_width -= static_cast<int>(g_minimap_settings.width);
        }
        g_golfed_editor.layout(0, content_top, golfed_width, content_height);

        g_diff_view.layout(0, content_top, window_width, content_height);
        g_trace_view.layout(0, content_top, window_width, content_height);
        g_golf_controls.layout(0, content_top, window_width, content_height);
        g_stats_panel.layout(0, content_top, window_width, content_height);
        g_appearance_panel.layout(0, content_top, window_width, content_height);
        g_about_panel.layout(0, content_top, window_width, content_height);
        g_command_palette.layout(window_width, window_height);

        if (g_viewport.hwnd() != nullptr)
        {
            MoveWindow(g_viewport.hwnd(), 0, content_top, window_width, content_height, TRUE);
            ShowWindow(g_viewport.hwnd(), g_tab_strip.active_index() == kViewportTabIndex ? SW_SHOW : SW_HIDE);
        }
    }

    void paint_chrome(HWND hwnd, const std::wstring& title)
    {
        if (g_render_target == nullptr)
        {
            return;
        }

        g_title_bar.tick();
        g_tab_strip.tick();
        g_status_dot.tick();
        g_source_editor.set_focus(g_window_focused && g_tab_strip.active_index() == kSourceTabIndex);
        g_golfed_editor.set_focus(g_window_focused && g_tab_strip.active_index() == kGolfedTabIndex);
        g_source_editor.tick();
        g_golfed_editor.tick();
        g_command_palette.tick();

        accessibility_begin_frame();

        g_render_target->BeginDraw();
        RECT client_rect{};
        GetClientRect(hwnd, &client_rect);
        D2D1_RECT_F full_rect = D2D1::RectF(0.0f, 0.0f,
            static_cast<float>(client_rect.right), static_cast<float>(client_rect.bottom));
        g_render_target->FillRectangle(full_rect, g_brushes.bg_app);

        int content_top = static_cast<int>(TitleBar::kHeight + TabStrip::kHeight);
        int content_height = client_rect.bottom - content_top;
        int window_width = client_rect.right;

        if (g_tab_strip.active_index() == kSourceTabIndex)
        {
            g_source_editor.paint(g_render_target, g_brushes);
            if (minimap_should_render(g_source_editor.line_count(), g_minimap_settings))
            {
                float minimap_x = static_cast<float>(window_width) - g_minimap_settings.width;
                paint_minimap(g_render_target, g_minimap_brush, g_brushes, g_source_editor.all_lines(),
                    minimap_x, static_cast<float>(content_top), g_minimap_settings.width, static_cast<float>(content_height));
            }
        }
        else if (g_tab_strip.active_index() == kGolfedTabIndex)
        {
            g_golfed_editor.paint(g_render_target, g_brushes);
            if (minimap_should_render(g_golfed_editor.line_count(), g_minimap_settings))
            {
                float minimap_x = static_cast<float>(window_width) - g_minimap_settings.width;
                paint_minimap(g_render_target, g_minimap_brush, g_brushes, g_golfed_editor.all_lines(),
                    minimap_x, static_cast<float>(content_top), g_minimap_settings.width, static_cast<float>(content_height));
            }
        }
        else if (g_tab_strip.active_index() == kDiffTabIndex)
        {
            g_diff_view.paint(g_render_target, g_brushes);
        }
        else if (g_tab_strip.active_index() == kTraceTabIndex)
        {
            g_trace_view.paint(g_render_target, g_brushes);
        }
        else if (g_tab_strip.active_index() == kControlsTabIndex)
        {
            g_golf_controls.paint(g_render_target, g_brushes);
        }
        else if (g_tab_strip.active_index() == kStatsTabIndex)
        {
            g_stats_panel.paint(g_render_target, g_brushes);
        }
        else if (g_tab_strip.active_index() == kAppearanceTabIndex)
        {
            g_appearance_panel.paint(g_render_target, g_brushes);
        }
        else if (g_tab_strip.active_index() == kAboutTabIndex)
        {
            g_about_panel.paint(g_render_target, g_brushes);
        }

        g_title_bar.paint(g_render_target, g_brushes, g_icons, title.c_str());
        g_tab_strip.paint(g_render_target, g_brushes);

        float dot_x = static_cast<float>(client_rect.right) - 20.0f;
        float dot_y = TitleBar::kHeight + TabStrip::kHeight * 0.5f;
        g_status_dot.paint(g_render_target, g_brushes, dot_x, dot_y);

        g_command_palette.paint(g_render_target, g_brushes);

        accessibility_end_frame();

        HRESULT hr = g_render_target->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET)
        {
            release_device_resources();
            create_device_resources(hwnd);
        }
    }

    void rebuild_ui_fonts(HWND hwnd)
    {
        g_tab_strip.destroy();
        g_source_editor.destroy();
        g_golfed_editor.destroy();
        g_diff_view.destroy();
        g_trace_view.destroy();
        g_command_palette.destroy();
        g_golf_controls.destroy();
        g_stats_panel.destroy();
        g_appearance_panel.destroy();
        g_about_panel.destroy();

        g_tab_strip.create(g_render_target, g_dwrite_factory);
        g_source_editor.create(g_render_target, g_dwrite_factory, false);
        g_golfed_editor.create(g_render_target, g_dwrite_factory, true);
        g_diff_view.create(g_render_target, g_dwrite_factory);
        g_trace_view.create(g_render_target, g_dwrite_factory);
        g_command_palette.create(g_render_target, g_dwrite_factory);
        g_golf_controls.create(g_render_target, g_dwrite_factory);
        g_stats_panel.create(g_render_target, g_dwrite_factory);
        g_appearance_panel.create(g_render_target, g_dwrite_factory);
        g_about_panel.create(g_render_target, g_dwrite_factory);

        layout_chrome(hwnd);
    }

    void recompile_from_editor()
    {
        g_status_dot.begin_compiling();
        std::string source = g_source_editor.text_utf8();
        std::string compile_error;
        bool ok = g_shader_runner.compile(source, compile_error);
        g_gl_ready = ok;
        g_status_dot.report_result(ok);

        if (ok)
        {
            g_source_editor.clear_error_line();

            UshaderGolfOptions options = to_golf_options(g_golf_controls.toggles());
            const std::string& protected_names = g_golf_controls.protected_names();
            UshaderGolfStats stats{};
            char* trace_json = nullptr;
            char* golfed = ushader_golf_traced(source.c_str(), options,
                protected_names.empty() ? nullptr : protected_names.c_str(), &stats, &trace_json);
            g_golfed_text_raw = golfed != nullptr ? std::string(golfed) : std::string();
            if (golfed != nullptr)
            {
                ushader_free_string(golfed);
            }
            refresh_golfed_view();

            std::vector<GolfTraceStep> trace_steps = trace_json != nullptr
                ? parse_golf_trace(std::string(trace_json))
                : std::vector<GolfTraceStep>();
            if (trace_json != nullptr)
            {
                ushader_free_string(trace_json);
            }
            g_trace_view.set_steps(trace_steps);

            g_diff_view.set_diff(compute_unified_diff(source, g_golfed_text_raw));

            UshaderBudgetResult budget = ushader_estimate_budget(g_golfed_text_raw.c_str());
            g_stats_panel.set_stats(stats, g_golfed_text_raw.size(), budget, g_golf_controls.budget_preset_index(), true);

            std::string golfed_compile_error;
            g_golfed_gl_ready = g_golfed_runner.compile(g_golfed_text_raw, golfed_compile_error);
        }
        else
        {
            ShaderErrorState error_state = make_shader_error_state(compile_error, ShaderRunner::fragment_header_lines());
            g_source_editor.set_error_line(error_state.source_line, utf8_to_wide(error_state.display_message));
        }
    }

    void switch_tab(HWND hwnd, int index)
    {
        g_tab_strip.switch_to(index);
        layout_chrome(hwnd);
    }

    void load_shader_file(HWND hwnd, const std::string& utf8_path)
    {
        std::string content = read_utf8_file(utf8_path);
        g_source_editor.set_text_utf8(content);
        recompile_from_editor();
        layout_chrome(hwnd);
        add_recent_file(utf8_path);
        switch_tab(hwnd, kSourceTabIndex);
    }

    void open_file_action(HWND hwnd)
    {
        std::optional<std::string> path = show_open_file_dialog_hwnd(hwnd, kGlslFilter, L"glsl");
        if (path.has_value())
        {
            load_shader_file(hwnd, *path);
        }
    }

    void save_file_action(HWND hwnd)
    {
        std::optional<std::string> path = show_save_file_dialog_hwnd(hwnd, kGlslFilter, L"glsl", L"shader.glsl");
        if (path.has_value())
        {
            write_utf8_file(*path, g_source_editor.text_utf8());
            add_recent_file(*path);
        }
    }

    void open_command_palette_for(HWND hwnd)
    {
        std::vector<PaletteCommand> commands = {
            {"Switch to tab: Source", [hwnd]() { switch_tab(hwnd, kSourceTabIndex); }},
            {"Switch to tab: Golfed", [hwnd]() { switch_tab(hwnd, kGolfedTabIndex); }},
            {"Switch to tab: Diff", [hwnd]() { switch_tab(hwnd, kDiffTabIndex); }},
            {"Switch to tab: Trace", [hwnd]() { switch_tab(hwnd, kTraceTabIndex); }},
            {"Switch to tab: Controls", [hwnd]() { switch_tab(hwnd, kControlsTabIndex); }},
            {"Switch to tab: Stats", [hwnd]() { switch_tab(hwnd, kStatsTabIndex); }},
            {"Switch to tab: Viewport", [hwnd]() { switch_tab(hwnd, kViewportTabIndex); }},
            {"Switch to tab: Appearance", [hwnd]() { switch_tab(hwnd, kAppearanceTabIndex); }},
            {"Switch to tab: About", [hwnd]() { switch_tab(hwnd, kAboutTabIndex); }},
            {"Run golf (recompile)", [hwnd]() { recompile_from_editor(); layout_chrome(hwnd); }},
            {"Reset to default shader", [hwnd]() { g_source_editor.set_text_utf8(kDefaultShaderSource); recompile_from_editor(); layout_chrome(hwnd); }},
            {"Toggle Formatted view", [hwnd]() { g_formatted_view = !g_formatted_view; refresh_golfed_view(); layout_chrome(hwnd); }},
            {"Toggle Compare mode", [hwnd]() { g_compare_mode = !g_compare_mode; switch_tab(hwnd, kViewportTabIndex); }},
            {"Open file...", [hwnd]() { open_file_action(hwnd); }},
            {"Save as...", [hwnd]() { save_file_action(hwnd); }},
        };
        for (const std::string& recent_path : load_recent_files())
        {
            commands.push_back({"Open recent: " + recent_path, [hwnd, recent_path]() { load_shader_file(hwnd, recent_path); }});
        }
        g_command_palette.open(commands);
    }

    void render_viewport_frame(const std::chrono::steady_clock::time_point& start_time, int frame_index)
    {
        g_viewport.make_current();

        float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start_time).count();
        bool show_shimmer = (!g_gl_ready || std::chrono::steady_clock::now() < g_shimmer_until) && g_shimmer_ready;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glViewport(0, 0, g_viewport.width(), g_viewport.height());
        glClear(GL_COLOR_BUFFER_BIT);

        if (show_shimmer)
        {
            ShaderUniforms uniforms{};
            uniforms.time = elapsed;
            uniforms.resolution_x = static_cast<float>(g_viewport.width());
            uniforms.resolution_y = static_cast<float>(g_viewport.height());
            uniforms.frame = frame_index;
            uniforms.frame_rate = 60.0f;
            g_shimmer_runner.draw(uniforms);
        }
        else if (g_compare_mode && g_gl_ready)
        {
            int window_width = g_viewport.width();
            int window_height = g_viewport.height();
            int half_width = window_width / 2;
            int right_width = window_width - half_width;

            ShaderUniforms left_uniforms{};
            left_uniforms.time = elapsed;
            left_uniforms.resolution_x = static_cast<float>(half_width);
            left_uniforms.resolution_y = static_cast<float>(window_height);
            left_uniforms.frame = frame_index;
            left_uniforms.frame_rate = 60.0f;

            g_compare_source_fb.resize(half_width, window_height);
            g_compare_source_fb.bind();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            g_shader_runner.draw(left_uniforms);
            g_compare_source_fb.unbind(window_width, window_height);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, g_compare_source_fb.framebuffer_id());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, half_width, window_height, 0, 0, half_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            if (g_golfed_gl_ready)
            {
                ShaderUniforms right_uniforms = left_uniforms;
                right_uniforms.resolution_x = static_cast<float>(right_width);

                g_compare_golfed_fb.resize(right_width, window_height);
                g_compare_golfed_fb.bind();
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                g_golfed_runner.draw(right_uniforms);
                g_compare_golfed_fb.unbind(window_width, window_height);

                glBindFramebuffer(GL_READ_FRAMEBUFFER, g_compare_golfed_fb.framebuffer_id());
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                glBlitFramebuffer(0, 0, right_width, window_height,
                    half_width, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else if (g_gl_ready)
        {
            ShaderUniforms uniforms{};
            uniforms.time = elapsed;
            uniforms.resolution_x = static_cast<float>(g_viewport.width());
            uniforms.resolution_y = static_cast<float>(g_viewport.height());
            uniforms.frame = frame_index;
            uniforms.frame_rate = 60.0f;
            g_shader_runner.draw(uniforms);
        }

        g_viewport.swap_buffers();
    }

    LRESULT CALLBACK main_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
        case WM_NCCALCSIZE:
        {
            if (wparam == TRUE)
            {
                WINDOWPLACEMENT placement{};
                placement.length = sizeof(WINDOWPLACEMENT);
                if (GetWindowPlacement(hwnd, &placement) && placement.showCmd == SW_SHOWMAXIMIZED)
                {
                    auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lparam);
                    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                    MONITORINFO monitor_info{};
                    monitor_info.cbSize = sizeof(MONITORINFO);
                    if (GetMonitorInfoW(monitor, &monitor_info))
                    {
                        params->rgrc[0] = monitor_info.rcWork;
                    }
                }
                return 0;
            }
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
        case WM_NCACTIVATE:
            return DefWindowProcW(hwnd, msg, wparam, -1);
        case WM_NCHITTEST:
        {
            POINT screen_pt{ GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            POINT client_pt = screen_pt;
            ScreenToClient(hwnd, &client_pt);

            RECT client_rect{};
            GetClientRect(hwnd, &client_rect);

            bool on_left = client_pt.x < kResizeBorder;
            bool on_right = client_pt.x >= client_rect.right - kResizeBorder;
            bool on_top = client_pt.y < kResizeBorder;
            bool on_bottom = client_pt.y >= client_rect.bottom - kResizeBorder;

            if (on_top && on_left) { return HTTOPLEFT; }
            if (on_top && on_right) { return HTTOPRIGHT; }
            if (on_bottom && on_left) { return HTBOTTOMLEFT; }
            if (on_bottom && on_right) { return HTBOTTOMRIGHT; }
            if (on_left) { return HTLEFT; }
            if (on_right) { return HTRIGHT; }
            if (on_top) { return HTTOP; }
            if (on_bottom) { return HTBOTTOM; }

            if (client_pt.y < static_cast<int>(TitleBar::kHeight))
            {
                TitleBarHit hit = g_title_bar.hit_test(client_pt.x, client_pt.y);
                switch (hit)
                {
                case TitleBarHit::Minimize: return HTMINBUTTON;
                case TitleBarHit::Maximize: return HTMAXBUTTON;
                case TitleBarHit::Close: return HTCLOSE;
                default: return HTCAPTION;
                }
            }

            return HTCLIENT;
        }
        case WM_NCMOUSEMOVE:
        {
            TitleBarHit hit = TitleBarHit::None;
            switch (wparam)
            {
            case HTMINBUTTON: hit = TitleBarHit::Minimize; break;
            case HTMAXBUTTON: hit = TitleBarHit::Maximize; break;
            case HTCLOSE: hit = TitleBarHit::Close; break;
            default: break;
            }
            g_title_bar.set_hover(hit);
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
        case WM_MOUSEMOVE:
        {
            int x = GET_X_LPARAM(lparam);
            int y = GET_Y_LPARAM(lparam);
            if (g_command_palette.is_open())
            {
                g_command_palette.on_mouse_move(x, y);
                return 0;
            }
            g_tab_strip.set_hover(g_tab_strip.hit_test(x, y));
            if (g_editor_dragging && active_editor() != nullptr)
            {
                active_editor()->on_mouse_move(x, y, true);
            }
            if (g_appearance_slider_dragging)
            {
                g_appearance_panel.on_mouse_move(x, y);
            }
            if (g_tab_strip.active_index() == kAboutTabIndex)
            {
                g_about_panel.on_mouse_move(x, y);
            }
            return 0;
        }
        case WM_LBUTTONDOWN:
        {
            int x = GET_X_LPARAM(lparam);
            int y = GET_Y_LPARAM(lparam);
            if (g_command_palette.is_open())
            {
                g_command_palette.on_mouse_down(x, y);
                return 0;
            }
            int tab_hit = g_tab_strip.hit_test(x, y);
            if (tab_hit >= 0)
            {
                g_tab_strip.switch_to(tab_hit);
                layout_chrome(hwnd);
                SetFocus(hwnd);
            }
            else if (active_editor() != nullptr && active_editor()->contains(x, y))
            {
                SetFocus(hwnd);
                bool shift_held = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                active_editor()->on_mouse_down(x, y, shift_held);
                g_editor_dragging = true;
                SetCapture(hwnd);
            }
            else if (g_tab_strip.active_index() == kTraceTabIndex && g_trace_view.contains(x, y))
            {
                SetFocus(hwnd);
                g_trace_view.on_mouse_down(x, y);
            }
            else if (g_tab_strip.active_index() == kControlsTabIndex && g_golf_controls.contains(x, y))
            {
                SetFocus(hwnd);
                g_golf_controls.on_mouse_down(x, y);
            }
            else if (g_tab_strip.active_index() == kAppearanceTabIndex && g_appearance_panel.contains(x, y))
            {
                SetFocus(hwnd);
                g_appearance_panel.on_mouse_down(x, y);
                if (g_appearance_panel.is_dragging())
                {
                    g_appearance_slider_dragging = true;
                    SetCapture(hwnd);
                }
                else if (g_appearance_panel.font_size_changed_and_clear())
                {
                    rebuild_ui_fonts(hwnd);
                }
            }
            else if (g_tab_strip.active_index() == kAboutTabIndex && g_about_panel.contains(x, y))
            {
                SetFocus(hwnd);
                g_about_panel.on_mouse_down(x, y);
            }
            return 0;
        }
        case WM_LBUTTONUP:
            if (g_editor_dragging)
            {
                g_editor_dragging = false;
                ReleaseCapture();
            }
            if (g_appearance_slider_dragging)
            {
                g_appearance_slider_dragging = false;
                g_appearance_panel.on_mouse_up();
                ReleaseCapture();
                if (g_appearance_panel.font_size_changed_and_clear())
                {
                    rebuild_ui_fonts(hwnd);
                }
            }
            return 0;
        case WM_MOUSEWHEEL:
        {
            if (active_editor() != nullptr)
            {
                active_editor()->on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wparam));
            }
            else if (g_tab_strip.active_index() == kDiffTabIndex)
            {
                g_diff_view.on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wparam));
            }
            else if (g_tab_strip.active_index() == kTraceTabIndex)
            {
                g_trace_view.on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wparam));
            }
            return 0;
        }
        case WM_SETFOCUS:
            g_window_focused = true;
            g_tab_strip.set_focused(true);
            return 0;
        case WM_KILLFOCUS:
            g_window_focused = false;
            g_tab_strip.set_focused(false);
            g_editor_dragging = false;
            return 0;
        case WM_CHAR:
        {
            if (g_command_palette.is_open())
            {
                if (g_command_palette.on_char(static_cast<wchar_t>(wparam)))
                {
                    return 0;
                }
            }
            if (active_editor() != nullptr)
            {
                if (active_editor()->on_char(static_cast<wchar_t>(wparam)))
                {
                    return 0;
                }
            }
            if (g_tab_strip.active_index() == kControlsTabIndex)
            {
                if (g_golf_controls.on_char(static_cast<wchar_t>(wparam)))
                {
                    return 0;
                }
            }
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
        case WM_KEYDOWN:
        {
            bool ctrl_held = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool shift_held = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            bool alt_held = (GetKeyState(VK_MENU) & 0x8000) != 0;

            if (win32_chord_matches(g_keybindings.command_palette, wparam, ctrl_held, shift_held, alt_held))
            {
                if (g_command_palette.is_open())
                {
                    g_command_palette.close();
                }
                else
                {
                    open_command_palette_for(hwnd);
                }
                return 0;
            }
            if (g_command_palette.is_open())
            {
                if (g_command_palette.on_key_down(wparam))
                {
                    return 0;
                }
            }
            if (wparam == VK_F5)
            {
                if (shift_held)
                {
                    g_source_editor.set_text_utf8(kDefaultShaderSource);
                }
                recompile_from_editor();
                layout_chrome(hwnd);
                return 0;
            }
            if (ctrl_held && shift_held && wparam == L'F')
            {
                g_formatted_view = !g_formatted_view;
                refresh_golfed_view();
                layout_chrome(hwnd);
                return 0;
            }
            if (ctrl_held && shift_held && wparam == L'C')
            {
                g_compare_mode = !g_compare_mode;
                switch_tab(hwnd, kViewportTabIndex);
                return 0;
            }
            if (win32_chord_matches(g_keybindings.open_file, wparam, ctrl_held, shift_held, alt_held))
            {
                open_file_action(hwnd);
                return 0;
            }
            if (win32_chord_matches(g_keybindings.save_file, wparam, ctrl_held, shift_held, alt_held))
            {
                save_file_action(hwnd);
                return 0;
            }
            if (win32_chord_matches(g_keybindings.new_tab, wparam, ctrl_held, shift_held, alt_held))
            {
                g_source_editor.set_text_utf8(kDefaultShaderSource);
                recompile_from_editor();
                layout_chrome(hwnd);
                return 0;
            }
            if (active_editor() != nullptr)
            {
                if (active_editor()->on_key_down(wparam, ctrl_held, shift_held))
                {
                    return 0;
                }
            }
            if (g_tab_strip.active_index() == kControlsTabIndex)
            {
                if (g_golf_controls.on_key_down(wparam))
                {
                    return 0;
                }
            }
            return DefWindowProcW(hwnd, msg, wparam, lparam);
        }
        case WM_SIZE:
        {
            if (g_render_target != nullptr)
            {
                RECT client_rect{};
                GetClientRect(hwnd, &client_rect);
                g_render_target->Resize(D2D1::SizeU(
                    static_cast<UINT32>(client_rect.right - client_rect.left),
                    static_cast<UINT32>(client_rect.bottom - client_rect.top)));
            }
            layout_chrome(hwnd);
            g_shimmer_until = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<float>(kShimmerHoldSeconds));
            return 0;
        }
        case WM_DROPFILES:
        {
            HDROP drop = reinterpret_cast<HDROP>(wparam);
            wchar_t path_buffer[MAX_PATH];
            if (DragQueryFileW(drop, 0, path_buffer, MAX_PATH) > 0)
            {
                load_shader_file(hwnd, wide_to_utf8(path_buffer));
            }
            DragFinish(drop);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
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
    Gdiplus::GdiplusStartupInput gdiplus_input;
    Gdiplus::GdiplusStartup(&g_gdiplus_token, &gdiplus_input, nullptr);

    HINSTANCE instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = main_wndproc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kMainClassName;
    RegisterClassExW(&wc);

    std::wstring title = L"uShader " + utf8_to_wide(USHADER_VERSION_STRING);

    HWND hwnd = CreateWindowExW(
        0, kMainClassName, title.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr, nullptr, instance, nullptr);
    if (hwnd == nullptr)
    {
        return 1;
    }

    MARGINS margins{ 0, 0, 0, 1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    DragAcceptFiles(hwnd, TRUE);
    accessibility_init_hwnd(hwnd);

    g_keybindings = load_win32_keybindings();

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2d_factory)))
    {
        return 1;
    }
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&g_dwrite_factory))))
    {
        return 1;
    }
    if (!create_device_resources(hwnd))
    {
        return 1;
    }
    if (!g_title_bar.create(g_render_target, g_dwrite_factory) || !g_tab_strip.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_source_editor.create(g_render_target, g_dwrite_factory, false))
    {
        return 1;
    }
    if (!g_golfed_editor.create(g_render_target, g_dwrite_factory, true))
    {
        return 1;
    }
    if (!g_diff_view.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_trace_view.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_command_palette.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_golf_controls.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_stats_panel.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_appearance_panel.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_about_panel.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    g_source_editor.set_text_utf8(kDefaultShaderSource);
    g_icons.load(exe_directory() + L"\\assets\\icons\\ui");

    RECT client_rect{};
    GetClientRect(hwnd, &client_rect);
    layout_chrome(hwnd);

    int viewport_top = static_cast<int>(TitleBar::kHeight + TabStrip::kHeight);
    int viewport_height = (client_rect.bottom - client_rect.top) - viewport_top;
    if (!g_viewport.create(hwnd, 0, viewport_top, client_rect.right - client_rect.left, viewport_height))
    {
        return 1;
    }
    layout_chrome(hwnd);

    g_viewport.make_current();
    bool gl_loaded = gl_load_functions_wgl();
    if (gl_loaded)
    {
        std::string shimmer_error;
        g_shimmer_ready = g_shimmer_runner.compile(kShimmerShaderSource, shimmer_error);
        recompile_from_editor();
        layout_chrome(hwnd);
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    auto start_time = std::chrono::steady_clock::now();
    int frame_index = 0;
    bool running = true;
    while (running)
    {
        MSG msg{};
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

        render_viewport_frame(start_time, frame_index++);
        paint_chrome(hwnd, title);
    }

    g_shader_runner.destroy();
    g_golfed_runner.destroy();
    g_shimmer_runner.destroy();
    g_compare_source_fb.destroy();
    g_compare_golfed_fb.destroy();
    g_viewport.destroy();
    g_title_bar.destroy();
    g_tab_strip.destroy();
    g_source_editor.destroy();
    g_golfed_editor.destroy();
    g_diff_view.destroy();
    g_trace_view.destroy();
    g_command_palette.destroy();
    g_golf_controls.destroy();
    g_stats_panel.destroy();
    g_appearance_panel.destroy();
    g_about_panel.destroy();
    accessibility_shutdown();
    g_icons.release();
    release_device_resources();
    if (g_dwrite_factory != nullptr) { g_dwrite_factory->Release(); }
    if (g_d2d_factory != nullptr) { g_d2d_factory->Release(); }

    Gdiplus::GdiplusShutdown(g_gdiplus_token);

    return 0;
}
