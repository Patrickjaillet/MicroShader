#include "render/wgl_viewport_host.h"
#include "render/gl_functions.h"
#include "render/shader_runner.h"
#include "render/framebuffer.h"
#include "render/shader_error_state.h"
#include "render/default_shader.h"
#include "ui/win32_theme_brushes.h"
#include "ui/win32_title_bar.h"
#include "ui/win32_tab_strip.h"
#include "ui/win32_document_tab_strip.h"
#include "ui/workspace.h"
#include "ui/export_wrappers.h"
#include "ui/exclude_list_import.h"
#include "ui/golf_profile.h"
#include "report/report_encoding.h"
#include "platform/screenshot.h"
#include "ui/win32_icon_set.h"
#include "ui/win32_status_dot.h"
#include "ui/win32_text_editor.h"
#include "ui/win32_minimap.h"
#include "ui/win32_diff_view.h"
#include "ui/win32_trace_view.h"
#include "ui/win32_command_palette.h"
#include "ui/win32_keybindings.h"
#include "ui/win32_golf_controls.h"
#include "ui/budget_presets.h"
#include "ui/win32_stats_panel.h"
#include "ui/win32_appearance_panel.h"
#include "ui/win32_appearance_settings.h"
#include "ui/win32_about_panel.h"
#include "ui/win32_twigl_export_panel.h"
#include "ui/golf_tips_panel.h"
#include "ui/win32_tool_button.h"
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

#include <algorithm>
#include <chrono>
#include <string>
#include <optional>
#include <vector>

namespace
{
    const wchar_t* kMainClassName = L"uShaderWin32Shell";
    constexpr int kResizeBorder = 6;
    constexpr float kShimmerHoldSeconds = 0.2f;
    // ROADMAP.md Phase 38.1 -- workspace restructure: tab indices now match
    // three semantic groups in win32_tab_strip.cpp's kTabLabels order --
    // Author (0-3), Analyze (4-6), Export (7), Settings (8-9).
    constexpr int kSourceTabIndex = 0;
    constexpr int kGolfedTabIndex = 1;
    constexpr int kDiffTabIndex = 2;
    constexpr int kViewportTabIndex = 3;
    constexpr int kTraceTabIndex = 4;
    constexpr int kStatsTabIndex = 5;
    constexpr int kGolfTipsTabIndex = 6;
    constexpr int kTwiglExportTabIndex = 7;
    constexpr int kAppearanceTabIndex = 8;
    constexpr int kAboutTabIndex = 9;
    constexpr int kInspectorWidth = 300;
    constexpr int kRunGolfButtonHeight = 32;
    constexpr int kMiniPreviewWidth = 420;
    constexpr int kMiniPreviewHeight = 236;
    constexpr int kMiniPreviewGap = 8;

    const wchar_t kGlslFilter[] = L"GLSL shaders (*.glsl)\0*.glsl\0All files (*.*)\0*.*\0";

    // ROADMAP.md Phase 38.2 -- one open document. `source_text` is the
    // authoritative live content while this document is NOT the active
    // one (the active document's text lives in g_source_editor instead,
    // synced into here on every switch -- see sync_active_document_from_editor).
    // Golfed output/diff/trace/stats are deliberately not cached per
    // document: switching documents re-runs the golfer (recompile_from_editor),
    // the same cost "Run golf" already pays, rather than duplicating that
    // state here.
    struct EditorDocument
    {
        std::string file_path;
        std::string display_name;
        std::string source_text;
        GolfPassToggles pass_toggles;
        std::string protected_names;
        int budget_preset_index = -1;
        // ROADMAP.md/roadmap_twigl.md Phase 44.3 -- mirrors
        // WorkspaceDocument's own twigl_* fields (workspace.h); kept in
        // sync at exactly the same points pass_toggles/protected_names/
        // budget_preset_index already are (see sync_active_document_from_editor/
        // load_document_into_editor below).
        int32_t twigl_mode = 0;
        bool twigl_es300 = false;
        uint8_t twigl_mrt_targets = 1;
        bool twigl_has_backbuffer = false;
        bool twigl_has_sound = false;
    };

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
    Win32DocumentTabStrip g_document_tab_strip;
    std::vector<EditorDocument> g_documents;
    int g_active_document = 0;
    int g_next_untitled_number = 1;
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
    Win32TwiglExportPanel g_twigl_export_panel;
    Win32GolfTipsPanel g_golf_tips_panel;
    Win32ToolButton g_run_golf_button;
    Win32ToolButton g_golf_harder_button;
    Win32ToolButton g_deep_search_toggle_button;
    bool g_deep_search_enabled = false;
    Win32ToolButton g_apply_harder_button;
    Win32ToolButton g_formatted_view_button;
    WglViewportHost g_viewport;
    ShaderRunner g_shader_runner;
    ShaderRunner g_golfed_runner;
    OffscreenFramebuffer g_compare_source_fb;
    OffscreenFramebuffer g_compare_golfed_fb;
    ShaderRunner g_shimmer_runner;
    bool g_gl_ready = false;
    bool g_golfed_gl_ready = false;
    bool g_compare_mode = false;
    WglViewportHost g_mini_viewport;
    ShaderRunner g_mini_source_runner;
    ShaderRunner g_mini_golfed_runner;
    OffscreenFramebuffer g_mini_source_fb;
    OffscreenFramebuffer g_mini_golfed_fb;
    bool g_mini_source_gl_ready = false;
    bool g_mini_golfed_gl_ready = false;
    bool g_shimmer_ready = false;
    bool g_window_focused = false;
    bool g_editor_dragging = false;
    bool g_formatted_view = false;
    std::string g_golfed_text_raw;
    bool g_harder_pending = false;
    std::string g_harder_pending_code;
    UshaderGolfStats g_harder_pending_stats{};
    bool g_harder_pending_frequency_aware_renaming = false;
    bool g_over_budget_hint = false;
    long long g_over_budget_bytes = 0;
    IDWriteTextFormat* g_hint_text_format = nullptr;
    std::chrono::steady_clock::time_point g_shimmer_until;
    ULONG_PTR g_gdiplus_token = 0;

    // ROADMAP.md/roadmap_twigl.md Phase 43.1(c) -- a brief, tab-independent
    // status confirmation shown after actions like "Copy for twigl.app"
    // whose only other feedback is an invisible clipboard write. Mirrors the
    // existing g_shimmer_until expiry pattern above.
    std::wstring g_status_toast_text;
    std::chrono::steady_clock::time_point g_status_toast_until;

    void show_status_toast(const wchar_t* text)
    {
        g_status_toast_text = text;
        g_status_toast_until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    }

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

    // ROADMAP.md Phase 38.2 -- needed by layout_chrome below (document tab
    // strip), so declared ahead of the rest of the document-management
    // functions further down this file.
    std::string basename_from_path(const std::string& path)
    {
        if (path.empty())
        {
            return std::string();
        }
        std::size_t slash = path.find_last_of("\\/");
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }

    std::vector<std::string> document_display_names()
    {
        std::vector<std::string> names;
        names.reserve(g_documents.size());
        for (const EditorDocument& doc : g_documents)
        {
            names.push_back(doc.display_name);
        }
        return names;
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
        g_document_tab_strip.layout(0, static_cast<int>(TitleBar::kHeight), window_width);
        g_document_tab_strip.set_documents(document_display_names(), g_active_document, 0);
        g_tab_strip.layout(0, static_cast<int>(TitleBar::kHeight + Win32DocumentTabStrip::kHeight), window_width);

        int content_top = static_cast<int>(TitleBar::kHeight + Win32DocumentTabStrip::kHeight + TabStrip::kHeight);
        int content_height = window_height - content_top;
        if (content_height < 0)
        {
            content_height = 0;
        }

        int main_width = window_width - kInspectorWidth - kMiniPreviewWidth;
        if (main_width < 0)
        {
            main_width = 0;
        }

        int source_width = main_width;
        if (minimap_should_render(g_source_editor.line_count(), g_minimap_settings))
        {
            source_width -= static_cast<int>(g_minimap_settings.width);
        }
        g_source_editor.layout(0, content_top, source_width, content_height);

        int golfed_width = main_width;
        if (minimap_should_render(g_golfed_editor.line_count(), g_minimap_settings))
        {
            golfed_width -= static_cast<int>(g_minimap_settings.width);
        }
        g_golfed_editor.layout(0, content_top, golfed_width, content_height);

        g_diff_view.layout(0, content_top, main_width, content_height);
        g_trace_view.layout(0, content_top, main_width, content_height);
        g_stats_panel.layout(0, content_top, main_width, content_height);
        g_appearance_panel.layout(0, content_top, main_width, content_height);
        g_about_panel.layout(0, content_top, main_width, content_height);
        g_twigl_export_panel.layout(0, content_top, main_width, content_height);
        g_golf_tips_panel.layout(0, content_top, main_width, content_height);
        g_command_palette.layout(window_width, window_height);

        g_golf_controls.layout(window_width - kInspectorWidth, content_top + kRunGolfButtonHeight + 8,
            kInspectorWidth, content_height - kRunGolfButtonHeight - 8);
        int inspector_x = window_width - kInspectorWidth;
        int golf_buttons_width = kInspectorWidth - 16 - 16;
        int run_golf_width = static_cast<int>(golf_buttons_width * 0.48);
        int golf_harder_width = static_cast<int>(golf_buttons_width * 0.34);
        int deep_search_width = golf_buttons_width - run_golf_width - golf_harder_width;
        g_run_golf_button.layout(inspector_x + 8, content_top + 8, run_golf_width, kRunGolfButtonHeight - 8);
        g_golf_harder_button.layout(inspector_x + 8 + run_golf_width + 8, content_top + 8, golf_harder_width, kRunGolfButtonHeight - 8);
        g_deep_search_toggle_button.layout(inspector_x + 8 + run_golf_width + 8 + golf_harder_width + 8, content_top + 8, deep_search_width, kRunGolfButtonHeight - 8);
        g_apply_harder_button.layout(main_width - 176, content_top + 8, 168, 24);
        g_formatted_view_button.layout(main_width - 132, content_top + 8, 120, 24);

        if (g_viewport.hwnd() != nullptr)
        {
            MoveWindow(g_viewport.hwnd(), 0, content_top, main_width, content_height, TRUE);
            ShowWindow(g_viewport.hwnd(), g_tab_strip.active_index() == kViewportTabIndex ? SW_SHOW : SW_HIDE);
        }
        if (g_mini_viewport.hwnd() != nullptr)
        {
            int mini_x = window_width - kInspectorWidth - kMiniPreviewWidth;
            int mini_total_height = kMiniPreviewHeight * 2 + kMiniPreviewGap;
            MoveWindow(g_mini_viewport.hwnd(), mini_x, content_top, kMiniPreviewWidth, mini_total_height, TRUE);
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
        g_twigl_export_panel.tick();
        g_command_palette.tick();

        accessibility_begin_frame();

        g_render_target->BeginDraw();
        RECT client_rect{};
        GetClientRect(hwnd, &client_rect);
        D2D1_RECT_F full_rect = D2D1::RectF(0.0f, 0.0f,
            static_cast<float>(client_rect.right), static_cast<float>(client_rect.bottom));
        g_render_target->FillRectangle(full_rect, g_brushes.bg_app);

        int content_top = static_cast<int>(TitleBar::kHeight + Win32DocumentTabStrip::kHeight + TabStrip::kHeight);
        int content_height = client_rect.bottom - content_top;
        int window_width = client_rect.right;

        int main_width = window_width - kInspectorWidth - kMiniPreviewWidth;

        if (g_tab_strip.active_index() == kSourceTabIndex)
        {
            g_source_editor.paint(g_render_target, g_brushes);
            if (minimap_should_render(g_source_editor.line_count(), g_minimap_settings))
            {
                float minimap_x = static_cast<float>(main_width) - g_minimap_settings.width;
                paint_minimap(g_render_target, g_minimap_brush, g_brushes, g_source_editor.all_lines(),
                    minimap_x, static_cast<float>(content_top), g_minimap_settings.width, static_cast<float>(content_height));
            }
        }
        else if (g_tab_strip.active_index() == kGolfedTabIndex)
        {
            g_golfed_editor.paint(g_render_target, g_brushes);
            if (minimap_should_render(g_golfed_editor.line_count(), g_minimap_settings))
            {
                float minimap_x = static_cast<float>(main_width) - g_minimap_settings.width;
                paint_minimap(g_render_target, g_minimap_brush, g_brushes, g_golfed_editor.all_lines(),
                    minimap_x, static_cast<float>(content_top), g_minimap_settings.width, static_cast<float>(content_height));
            }
            g_formatted_view_button.paint(g_render_target, g_brushes, L"Formatted view", g_formatted_view);
            accessibility_register_toggle("Formatted view", AccessibleRole::CheckBox,
                static_cast<float>(main_width - 132), static_cast<float>(content_top + 8), 120.0f, 24.0f, true, g_formatted_view);
        }
        else if (g_tab_strip.active_index() == kDiffTabIndex)
        {
            g_diff_view.paint(g_render_target, g_brushes);
            if (g_harder_pending)
            {
                g_apply_harder_button.paint(g_render_target, g_brushes, L"Apply harder result", true);
                accessibility_register("Apply harder result", AccessibleRole::Button,
                    static_cast<float>(main_width - 176), static_cast<float>(content_top + 8), 168.0f, 24.0f, true);
            }
        }
        else if (g_tab_strip.active_index() == kTraceTabIndex)
        {
            g_trace_view.paint(g_render_target, g_brushes);
            if (g_over_budget_hint && g_hint_text_format != nullptr)
            {
                D2D1_RECT_F hint_rect = D2D1::RectF(0.0f, static_cast<float>(content_top),
                    static_cast<float>(main_width), static_cast<float>(content_top + 28));
                g_render_target->FillRectangle(hint_rect, g_brushes.bg_panel_raised);
                wchar_t hint_text[160];
                swprintf_s(hint_text, L"%lld bytes over budget -- see Golf Tips for manual techniques", g_over_budget_bytes);
                g_render_target->DrawText(hint_text, static_cast<UINT32>(wcslen(hint_text)),
                    g_hint_text_format, hint_rect, g_brushes.text_secondary);
                accessibility_register("Golf Tips hint", AccessibleRole::Button,
                    hint_rect.left, hint_rect.top, hint_rect.right - hint_rect.left, hint_rect.bottom - hint_rect.top, true);
            }
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
        else if (g_tab_strip.active_index() == kTwiglExportTabIndex)
        {
            g_twigl_export_panel.paint(g_render_target, g_brushes);
        }
        else if (g_tab_strip.active_index() == kGolfTipsTabIndex)
        {
            g_golf_tips_panel.paint(g_render_target, g_brushes);
        }

        {
            D2D1_RECT_F inspector_bg = D2D1::RectF(static_cast<float>(window_width - kInspectorWidth), static_cast<float>(content_top),
                static_cast<float>(window_width), static_cast<float>(client_rect.bottom));
            g_render_target->FillRectangle(inspector_bg, g_brushes.bg_panel);
            g_run_golf_button.paint(g_render_target, g_brushes, L"Golf", true);
            accessibility_register("Golf", AccessibleRole::Button,
                static_cast<float>(g_run_golf_button.origin_x_px()), static_cast<float>(g_run_golf_button.origin_y_px()),
                static_cast<float>(g_run_golf_button.width_in_px()), static_cast<float>(g_run_golf_button.height_in_px()), true);
            g_golf_harder_button.paint(g_render_target, g_brushes, L"Golf harder", false);
            accessibility_register("Golf harder", AccessibleRole::Button,
                static_cast<float>(g_golf_harder_button.origin_x_px()), static_cast<float>(g_golf_harder_button.origin_y_px()),
                static_cast<float>(g_golf_harder_button.width_in_px()), static_cast<float>(g_golf_harder_button.height_in_px()), true);
            // ROADMAP.md Phase 37.1 -- when checked, "Golf harder" runs the
            // bounded simulated-annealing search (golf_harder_deep) instead
            // of Phase 32.1's fast deterministic hill-climb.
            g_deep_search_toggle_button.paint(g_render_target, g_brushes, L"Deep", g_deep_search_enabled);
            accessibility_register_toggle("Deep search", AccessibleRole::CheckBox,
                static_cast<float>(g_deep_search_toggle_button.origin_x_px()), static_cast<float>(g_deep_search_toggle_button.origin_y_px()),
                static_cast<float>(g_deep_search_toggle_button.width_in_px()), static_cast<float>(g_deep_search_toggle_button.height_in_px()),
                true, g_deep_search_enabled);
            g_golf_controls.paint(g_render_target, g_brushes);

            D2D1_RECT_F mini_bg = D2D1::RectF(static_cast<float>(window_width - kInspectorWidth - kMiniPreviewWidth),
                static_cast<float>(content_top), static_cast<float>(window_width - kInspectorWidth),
                static_cast<float>(content_top + kMiniPreviewHeight * 2 + kMiniPreviewGap));
            g_render_target->FillRectangle(mini_bg, g_brushes.bg_app);
        }

        g_title_bar.paint(g_render_target, g_brushes, g_icons, title.c_str());
        g_document_tab_strip.paint(g_render_target, g_brushes);
        g_tab_strip.paint(g_render_target, g_brushes);

        float dot_x = static_cast<float>(client_rect.right) - 20.0f;
        float dot_y = TitleBar::kHeight + Win32DocumentTabStrip::kHeight + TabStrip::kHeight * 0.5f;
        g_status_dot.paint(g_render_target, g_brushes, dot_x, dot_y);

        // ROADMAP.md/roadmap_twigl.md Phase 43.1(c) -- drawn here, outside
        // every per-tab conditional above, so it's visible regardless of
        // which tab is active (Copy actions can be triggered from the
        // command palette from any tab).
        if (!g_status_toast_text.empty() && g_hint_text_format != nullptr
            && std::chrono::steady_clock::now() < g_status_toast_until)
        {
            float toast_width = 260.0f;
            D2D1_RECT_F toast_rect = D2D1::RectF(static_cast<float>(client_rect.right) - toast_width - 12.0f,
                TitleBar::kHeight + Win32DocumentTabStrip::kHeight + TabStrip::kHeight + 8.0f,
                static_cast<float>(client_rect.right) - 12.0f,
                TitleBar::kHeight + Win32DocumentTabStrip::kHeight + TabStrip::kHeight + 36.0f);
            g_render_target->FillRectangle(toast_rect, g_brushes.bg_panel_raised);
            g_render_target->DrawText(g_status_toast_text.c_str(), static_cast<UINT32>(g_status_toast_text.size()),
                g_hint_text_format, toast_rect, g_brushes.text_secondary);
        }

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
        g_document_tab_strip.destroy();
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
        g_twigl_export_panel.destroy();
        g_golf_tips_panel.destroy();
        if (g_hint_text_format != nullptr) { g_hint_text_format->Release(); g_hint_text_format = nullptr; }
        g_run_golf_button.destroy();
        g_golf_harder_button.destroy();
        g_deep_search_toggle_button.destroy();
        g_apply_harder_button.destroy();
        g_formatted_view_button.destroy();

        g_document_tab_strip.create(g_render_target, g_dwrite_factory);
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
        g_twigl_export_panel.create(g_render_target, g_dwrite_factory);
        g_golf_tips_panel.create(g_render_target, g_dwrite_factory);
        g_dwrite_factory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, ui_font_pt(13.0f), L"en-us", &g_hint_text_format);
        if (g_hint_text_format != nullptr) { g_hint_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER); }
        g_run_golf_button.create(g_render_target, g_dwrite_factory);
        g_golf_harder_button.create(g_render_target, g_dwrite_factory);
        g_deep_search_toggle_button.create(g_render_target, g_dwrite_factory);
        g_apply_harder_button.create(g_render_target, g_dwrite_factory);
        g_formatted_view_button.create(g_render_target, g_dwrite_factory);

        layout_chrome(hwnd);
    }

    // golf.md Phase 33.2 -- when the golfed output is over the active
    // budget preset, surfaces a non-modal hint pointing at the Phase 33.1
    // Golf Tips panel from the "Explain Golf" trace view, rather than
    // silently suggesting a specific rewrite.
    void update_over_budget_hint(const UshaderBudgetResult& budget)
    {
        g_over_budget_hint = false;
        g_over_budget_bytes = 0;

        int preset_index = g_golf_controls.budget_preset_index();
        if (preset_index < 0)
        {
            return;
        }
        std::size_t preset_count = 0;
        const BudgetPreset* presets = budget_presets(preset_count);
        if (static_cast<std::size_t>(preset_index) >= preset_count)
        {
            return;
        }
        const BudgetPreset& preset = presets[preset_index];
        if (preset.deflate_limit >= 0 && static_cast<long long>(budget.deflate_bytes) > preset.deflate_limit)
        {
            g_over_budget_hint = true;
            g_over_budget_bytes = static_cast<long long>(budget.deflate_bytes) - preset.deflate_limit;
        }
        else if (preset.raw_limit >= 0 && static_cast<long long>(budget.raw_bytes) > preset.raw_limit)
        {
            g_over_budget_hint = true;
            g_over_budget_bytes = static_cast<long long>(budget.raw_bytes) - preset.raw_limit;
        }
    }

    // ROADMAP.md/roadmap_twigl.md Phase 43.3 -- always reserves the twigl
    // snippet names (PI, hsv, rotate2D, ...) from the golfer's generated
    // short-name space, on top of whatever the user typed in the Protected
    // Names field, so a snippet inserted later can never collide with an
    // identifier the golfer already renamed to that same short name. See
    // twigl_reserved_snippet_names_csv()'s own doc comment
    // (win32_twigl_export_panel.h) for why this deliberately does NOT also
    // reserve twigl's single-character uniform names.
    std::string effective_protected_names_for_golf()
    {
        std::string combined = g_golf_controls.protected_names();
        if (!combined.empty())
        {
            combined += ",";
        }
        combined += twigl_reserved_snippet_names_csv();
        return combined;
    }

    void recompile_from_editor()
    {
        g_harder_pending = false;
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
            const std::string protected_names = effective_protected_names_for_golf();
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
            g_twigl_export_panel.set_golfed_source(g_golfed_text_raw);

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
            UshaderBudgetResult original_budget = ushader_estimate_budget(source.c_str());
            g_stats_panel.set_stats(stats, g_golfed_text_raw.size(), budget, original_budget, g_golf_controls.budget_preset_index(), true, options.frequency_aware_renaming);
            update_over_budget_hint(budget);

            std::string golfed_compile_error;
            g_golfed_gl_ready = g_golfed_runner.compile(g_golfed_text_raw, golfed_compile_error);
        }
        else
        {
            ShaderErrorState error_state = make_shader_error_state(compile_error, ShaderRunner::fragment_header_lines());
            g_source_editor.set_error_line(error_state.source_line, utf8_to_wide(error_state.display_message));
        }

        g_mini_viewport.make_current();
        std::string mini_source_error;
        g_mini_source_gl_ready = g_mini_source_runner.compile(source, mini_source_error);
        if (ok)
        {
            std::string mini_golfed_error;
            g_mini_golfed_gl_ready = g_mini_golfed_runner.compile(g_golfed_text_raw, mini_golfed_error);
        }
        else
        {
            g_mini_golfed_gl_ready = false;
        }
        g_viewport.make_current();
    }

    void switch_tab(HWND hwnd, int index)
    {
        g_tab_strip.switch_to(index);
        layout_chrome(hwnd);
    }

    // ROADMAP.md Phase 38.2 -- multi-document workspace (continued; see
    // document_display_names/basename_from_path above, moved ahead of
    // layout_chrome since it needs them).

    void sync_active_document_from_editor()
    {
        if (g_documents.empty() || g_active_document < 0 || g_active_document >= static_cast<int>(g_documents.size()))
        {
            return;
        }
        EditorDocument& doc = g_documents[static_cast<std::size_t>(g_active_document)];
        doc.source_text = g_source_editor.text_utf8();
        doc.pass_toggles = g_golf_controls.toggles();
        doc.protected_names = g_golf_controls.protected_names();
        doc.budget_preset_index = g_golf_controls.budget_preset_index();
        doc.twigl_mode = g_twigl_export_panel.current_mode();
        doc.twigl_es300 = g_twigl_export_panel.current_es300();
        doc.twigl_mrt_targets = g_twigl_export_panel.current_mrt_targets();
        doc.twigl_has_backbuffer = g_twigl_export_panel.current_has_backbuffer();
        doc.twigl_has_sound = g_twigl_export_panel.current_has_sound();
    }

    void load_document_into_editor(HWND hwnd, int index)
    {
        if (index < 0 || index >= static_cast<int>(g_documents.size()))
        {
            return;
        }
        const EditorDocument& doc = g_documents[static_cast<std::size_t>(index)];
        g_active_document = index;
        g_source_editor.set_text_utf8(doc.source_text);
        g_golf_controls.set_toggles(doc.pass_toggles);
        g_golf_controls.set_protected_names(doc.protected_names);
        g_golf_controls.set_budget_preset_index(doc.budget_preset_index);
        g_twigl_export_panel.restore_state(doc.twigl_mode, doc.twigl_es300, doc.twigl_mrt_targets,
            doc.twigl_has_backbuffer, doc.twigl_has_sound);
        recompile_from_editor();
        layout_chrome(hwnd);
    }

    void switch_document(HWND hwnd, int index)
    {
        if (index == g_active_document || index < 0 || index >= static_cast<int>(g_documents.size()))
        {
            return;
        }
        sync_active_document_from_editor();
        load_document_into_editor(hwnd, index);
    }

    void new_document_action(HWND hwnd)
    {
        sync_active_document_from_editor();
        EditorDocument doc;
        doc.display_name = "Untitled " + std::to_string(g_next_untitled_number++);
        doc.source_text = kDefaultShaderSource;
        doc.budget_preset_index = -1;
        g_documents.push_back(doc);
        load_document_into_editor(hwnd, static_cast<int>(g_documents.size()) - 1);
        switch_tab(hwnd, kSourceTabIndex);
    }

    // ROADMAP.md Phase 38.2 -- session-restore-on-launch: serializes every
    // open document (file-backed documents by path only -- their content
    // is re-read from disk on restore; unsaved documents by inline source
    // text) to %APPDATA%\ushader\last_session.ushaderworkspace.
    // ROADMAP.md Phase 38.3 -- one-click export presets, restored from the
    // retired ImGui shell (wrap_as_*/rewrite_twigl_shader already existed
    // in rust-core/export_wrappers.cpp -- only the clipboard wiring was
    // missing). Reachable from the command palette, mirroring how most
    // one-shot actions in this shell are already exposed.
    void copy_text_to_clipboard(const std::string& utf8_text)
    {
        std::wstring wide = utf8_to_wide(utf8_text);
        if (OpenClipboard(nullptr))
        {
            EmptyClipboard();
            HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (wide.size() + 1) * sizeof(wchar_t));
            if (mem != nullptr)
            {
                void* dest = GlobalLock(mem);
                if (dest != nullptr)
                {
                    memcpy(dest, wide.c_str(), (wide.size() + 1) * sizeof(wchar_t));
                    GlobalUnlock(mem);
                    SetClipboardData(CF_UNICODETEXT, mem);
                }
            }
            CloseClipboard();
        }
    }

    void copy_as_shadertoy_action()
    {
        copy_text_to_clipboard(wrap_as_shadertoy_main_image(g_golfed_text_raw));
    }

    void copy_as_bonzomatic_action()
    {
        copy_text_to_clipboard(wrap_as_bonzomatic_source(g_golfed_text_raw));
    }

    void copy_as_bare_main_action()
    {
        copy_text_to_clipboard(wrap_as_bare_main(g_golfed_text_raw));
    }

    void copy_as_twigl_action()
    {
        // ROADMAP.md/roadmap_twigl.md Phase 42.4 -- this used to call
        // ushader_twigl_rewrite directly with only mode/es300, silently
        // ignoring whatever MRT/backbuffer/sound state the Export panel's
        // own preview was showing at the time. compute_export_text() is the
        // exact same call the live preview uses, so what gets copied is now
        // guaranteed to be what the user was just looking at.
        std::string rewritten = g_twigl_export_panel.compute_export_text(g_golfed_text_raw);
        if (!rewritten.empty())
        {
            copy_text_to_clipboard(rewritten);
            // ROADMAP.md/roadmap_twigl.md Phase 43.1(c) -- the only other
            // feedback for a clipboard write is invisible, so confirm it.
            show_status_toast(L"Copied \u2014 paste into twigl.app");
        }
    }

    // ROADMAP.md/roadmap_twigl.md Phase 43.2 -- the command-palette
    // counterpart of the Export panel's "Import twigl shader" button
    // (win32_twigl_export_panel.cpp), for reachability without switching to
    // the Twigl Export tab first. Imports from the same panel's current
    // preview text, exactly like the button does, so both entry points
    // reconstruct the same source.
    void import_twigl_shader_action(HWND hwnd)
    {
        std::string preview_text = g_twigl_export_panel.preview_text_for_import();
        if (preview_text.empty())
        {
            return;
        }
        char* unrewritten = ushader_twigl_unrewrite(preview_text.c_str(), g_twigl_export_panel.current_mode());
        if (unrewritten != nullptr)
        {
            g_source_editor.set_text_utf8(std::string(unrewritten));
            ushader_free_string(unrewritten);
            show_status_toast(L"Imported into Source");
            layout_chrome(hwnd);
        }
    }

    // ROADMAP.md Phase 38.5 -- .ushaderprofile save/load, restored from the
    // retired ImGui shell (the engine and JSON schema already existed in
    // golf_profile.cpp/docs/ushaderprofile-schema.md -- only the UI was
    // missing), plus the built-in Safe/Maximum/None profile picker.
    const wchar_t kProfileFilter[] = L"uShader profile (*.ushaderprofile)\0*.ushaderprofile\0All files (*.*)\0*.*\0";

    void apply_profile(HWND hwnd, const GolfPassToggles& toggles, const std::string& protected_names, int budget_preset_index)
    {
        g_golf_controls.set_toggles(toggles);
        g_golf_controls.set_protected_names(protected_names);
        g_golf_controls.set_budget_preset_index(budget_preset_index);
        recompile_from_editor();
        layout_chrome(hwnd);
    }

    void load_profile_action(HWND hwnd)
    {
        std::optional<std::string> path = show_open_file_dialog_hwnd(hwnd, kProfileFilter, L"ushaderprofile");
        if (!path.has_value())
        {
            return;
        }
        std::string text = read_utf8_file(*path);
        GolfPassToggles toggles{};
        std::string protected_names;
        int budget_preset_index = -1;
        if (deserialize_golf_profile(text, toggles, protected_names, budget_preset_index))
        {
            apply_profile(hwnd, toggles, protected_names, budget_preset_index);
            save_last_profile_path(*path);
        }
    }

    void save_profile_action(HWND hwnd)
    {
        std::optional<std::string> path = show_save_file_dialog_hwnd(hwnd, kProfileFilter, L"ushaderprofile", L"profile.ushaderprofile");
        if (!path.has_value())
        {
            return;
        }
        std::string text = serialize_golf_profile(g_golf_controls.toggles(), g_golf_controls.protected_names(), g_golf_controls.budget_preset_index());
        write_utf8_file(*path, text);
        save_last_profile_path(*path);
    }

    // ROADMAP.md Phase 38.6 -- exclude-name-list import, restored from the
    // retired ImGui shell (the engine already existed in
    // exclude_list_import.cpp -- only the UI trigger was missing).
    // **Documented deviation from the literal wording** ("a visible
    // button/dialog in the redesigned Controls inspector"): the inspector
    // is already dense (22 pass checkboxes plus the swizzle-alphabet and
    // budget-preset controls) and its Direct2D layout code cannot be
    // visually verified in this environment, so adding another
    // always-visible row there risked an unverifiable overflow. Exposed
    // through the command palette instead, consistent with how most other
    // one-shot actions in this shell (Open file, Save as, golf-profile
    // save/load, PNG capture, session report) are already reached.
    void import_exclude_list_action_from_palette(HWND hwnd)
    {
        std::string protected_names = g_golf_controls.protected_names();
        import_exclude_list_action(protected_names, hwnd);
        g_golf_controls.set_protected_names(protected_names);
        layout_chrome(hwnd);
    }

    // ROADMAP.md Phase 38.7 -- PNG viewport screenshot and a self-contained
    // HTML session report, both offline-only (no network fetch, no
    // runtime-fetched encoder), restored from the retired ImGui shell's
    // reporting feature. **Documented deviation**: GIF recording is
    // explicitly not implemented here -- it would require a hand-written
    // GIF89a/LZW encoder (a project of DEFLATE-encoder-class scope, see
    // ROADMAP.md Phase 37.2's precedent for what that actually costs to
    // build correctly), disproportionate to add as a side effect of this
    // phase; tracked as a known gap rather than silently skipped.
    void capture_viewport_png_action(HWND hwnd)
    {
        if (!g_gl_ready)
        {
            return;
        }
        std::optional<std::string> path = show_save_file_dialog_hwnd(hwnd,
            L"PNG image (*.png)\0*.png\0All files (*.*)\0*.*\0", L"png", L"screenshot.png");
        if (!path.has_value())
        {
            return;
        }

        g_viewport.make_current();
        int width = g_viewport.width();
        int height = g_viewport.height();
        if (width <= 0 || height <= 0)
        {
            return;
        }

        ShaderRunner& runner = g_golfed_gl_ready ? g_golfed_runner : g_shader_runner;
        ShaderUniforms uniforms{};
        uniforms.resolution_x = static_cast<float>(width);
        uniforms.resolution_y = static_cast<float>(height);
        uniforms.frame_rate = 60.0f;

        g_compare_golfed_fb.resize(width, height);
        g_compare_golfed_fb.bind();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        runner.draw(uniforms);
        g_compare_golfed_fb.unbind(width, height);

        save_framebuffer_png(g_compare_golfed_fb, *path);
    }

    // ROADMAP.md/roadmap_twigl.md Phase 45.1 -- a small, fixed preset list
    // (size in pixels, frame count) in the spirit of twigl.app's own
    // capture presets. Only the *numbers* are modeled here, not any of
    // twigl.app's UI text/labels (see roadmap_twigl.md 45.1's own note on
    // why: those numbers aren't copyrightable, the surrounding UI copy
    // would be).
    struct GifCapturePreset
    {
        const wchar_t* label;
        int size;
        int frame_count;
    };
    constexpr GifCapturePreset kGifCapturePresets[] = {
        {L"128x128, 60 frames", 128, 60},
        {L"256x256, 120 frames", 256, 120},
        {L"512x512, 180 frames", 512, 180},
    };

    void capture_viewport_gif_action(HWND hwnd, const GifCapturePreset& preset)
    {
        if (!g_gl_ready)
        {
            return;
        }
        std::optional<std::string> path = show_save_file_dialog_hwnd(hwnd,
            L"GIF image (*.gif)\0*.gif\0All files (*.*)\0*.*\0", L"gif", L"capture.gif");
        if (!path.has_value())
        {
            return;
        }

        const int width = preset.size;
        const int height = preset.size;
        const int frame_count = preset.frame_count;
        constexpr float kFrameRate = 25.0f;
        const uint16_t delay_centiseconds = static_cast<uint16_t>(100.0f / kFrameRate + 0.5f);

        g_viewport.make_current();
        ShaderRunner& runner = g_golfed_gl_ready ? g_golfed_runner : g_shader_runner;

        g_compare_golfed_fb.resize(width, height);

        // Each frame's pixels are captured into its own buffer up front
        // (rather than re-reading in place) so every pointer handed to
        // ushader_encode_gif below stays valid for the single FFI call
        // that consumes all of them together.
        std::vector<std::vector<unsigned char>> frames_pixels;
        frames_pixels.reserve(static_cast<std::size_t>(frame_count));

        for (int frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            ShaderUniforms uniforms{};
            uniforms.resolution_x = static_cast<float>(width);
            uniforms.resolution_y = static_cast<float>(height);
            uniforms.time = static_cast<float>(frame_index) / kFrameRate;
            uniforms.frame = frame_index;
            uniforms.frame_rate = kFrameRate;

            g_compare_golfed_fb.bind();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            runner.draw(uniforms);
            g_compare_golfed_fb.unbind(width, height);

            std::vector<unsigned char> pixels;
            if (!g_compare_golfed_fb.read_pixels(pixels))
            {
                return;
            }

            // OpenGL's framebuffer origin is bottom-left; GIF (like every
            // other common image format) is stored top-down. save_framebuffer_png
            // handles this via stbi_flip_vertically_on_write -- there is no
            // equivalent flag here, so the rows are flipped explicitly.
            std::vector<unsigned char> flipped(pixels.size());
            const std::size_t stride = static_cast<std::size_t>(width) * 4;
            for (int y = 0; y < height; ++y)
            {
                std::memcpy(&flipped[static_cast<std::size_t>(y) * stride],
                    &pixels[static_cast<std::size_t>(height - 1 - y) * stride], stride);
            }
            frames_pixels.push_back(std::move(flipped));
        }

        std::vector<const uint8_t*> frame_ptrs;
        frame_ptrs.reserve(frames_pixels.size());
        for (const auto& frame : frames_pixels)
        {
            frame_ptrs.push_back(frame.data());
        }

        UshaderByteBuffer gif = ushader_encode_gif(frame_ptrs.data(), frame_ptrs.size(),
            static_cast<uint16_t>(width), static_cast<uint16_t>(height), delay_centiseconds);
        if (gif.data != nullptr && gif.len > 0)
        {
            // write_utf8_file (src/platform/paths.cpp) already does the
            // correct UTF-8 -> UTF-16 path conversion and writes via
            // fwrite with an explicit byte count, so it's binary-safe for
            // arbitrary content (including embedded zero bytes, which a
            // GIF's LZW-compressed data can certainly contain) -- reusing
            // it here instead of hand-rolling a second path-conversion
            // call site.
            std::string gif_bytes(reinterpret_cast<const char*>(gif.data), gif.len);
            if (write_utf8_file(*path, gif_bytes))
            {
                show_status_toast(L"GIF saved");
            }
        }
        ushader_free_byte_buffer(gif);
    }

    std::string build_session_report_html()
    {
        std::string source = g_source_editor.text_utf8();
        UshaderBudgetResult budget = ushader_estimate_budget(g_golfed_text_raw.c_str());
        double reduction_pct = source.empty() ? 0.0
            : 100.0 * (1.0 - static_cast<double>(g_golfed_text_raw.size()) / static_cast<double>(source.size()));

        char stats_line[256];
        std::snprintf(stats_line, sizeof(stats_line),
            "%zu -> %zu chars (%.1f%% reduction), DEFLATE estimate: %zu bytes",
            source.size(), g_golfed_text_raw.size(), reduction_pct, budget.deflate_bytes);

        std::string html = "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<title>uShader session report</title>"
            "<style>body{font-family:Segoe UI,sans-serif;background:#1e1e1e;color:#ddd;padding:16px;}"
            "pre{background:#252525;padding:12px;overflow:auto;white-space:pre-wrap;border-radius:4px;}"
            "h1,h2{color:#fff;}</style></head><body>";
        html += "<h1>uShader session report</h1>";
        html += "<h2>Stats</h2><pre>" + html_escape(stats_line) + "</pre>";
        html += "<h2>Source</h2><pre>" + html_escape(source) + "</pre>";
        html += "<h2>Golfed</h2><pre>" + html_escape(g_golfed_text_raw) + "</pre>";
        html += "</body></html>\n";
        return html;
    }

    void export_session_report_action(HWND hwnd)
    {
        std::optional<std::string> path = show_save_file_dialog_hwnd(hwnd,
            L"HTML report (*.html)\0*.html\0All files (*.*)\0*.*\0", L"html", L"report.html");
        if (!path.has_value())
        {
            return;
        }
        write_utf8_file(*path, build_session_report_html());
    }

    void save_session()
    {
        sync_active_document_from_editor();
        WorkspaceState state;
        state.active_tab = g_tab_strip.active_index();
        state.active_document = g_active_document;
        for (const EditorDocument& doc : g_documents)
        {
            WorkspaceDocument wdoc;
            wdoc.file_path = doc.file_path;
            wdoc.pass_toggles = doc.pass_toggles;
            wdoc.protected_names = doc.protected_names;
            wdoc.budget_preset_index = doc.budget_preset_index;
            wdoc.twigl_mode = doc.twigl_mode;
            wdoc.twigl_es300 = doc.twigl_es300;
            wdoc.twigl_mrt_targets = doc.twigl_mrt_targets;
            wdoc.twigl_has_backbuffer = doc.twigl_has_backbuffer;
            wdoc.twigl_has_sound = doc.twigl_has_sound;
            if (doc.file_path.empty())
            {
                wdoc.unsaved_source = doc.source_text;
            }
            state.documents.push_back(wdoc);
        }
        std::string path = workspace_session_path();
        if (!path.empty())
        {
            write_utf8_file(path, serialize_workspace(state));
        }
    }

    void close_document_action(HWND hwnd, int index)
    {
        if (index < 0 || index >= static_cast<int>(g_documents.size()) || g_documents.size() <= 1)
        {
            return;
        }
        g_documents.erase(g_documents.begin() + index);
        int next_active = g_active_document;
        if (index < g_active_document)
        {
            next_active -= 1;
        }
        else if (index == g_active_document)
        {
            int last_index = static_cast<int>(g_documents.size()) - 1;
            next_active = g_active_document < last_index ? g_active_document : last_index;
        }
        g_active_document = -1; // force load_document_into_editor to treat this as a real switch
        load_document_into_editor(hwnd, next_active);
    }

    // golf.md Phase 32.1 -- `ushader_golf_harder`/`_deep` can flip toggles
    // (including `frequency_aware_renaming`) relative to the base options
    // passed in; the FFI boundary reports this via `out_applied_json`
    // (`[{"pass_name":...,"from":...,"to":...}]`) rather than returning the
    // full final option set. This scans that JSON for one specific pass
    // name and returns its final "to" value, falling back to `base_value`
    // (the toggle's state before the search ran) when the search never
    // touched that coordinate.
    bool applied_json_final_bool(const std::string& json, const char* pass_name, bool base_value)
    {
        std::string needle = std::string("\"pass_name\":\"") + pass_name + "\"";
        std::size_t pos = json.find(needle);
        if (pos == std::string::npos)
        {
            return base_value;
        }
        std::size_t to_pos = json.find("\"to\":\"", pos);
        if (to_pos == std::string::npos)
        {
            return base_value;
        }
        std::size_t value_start = to_pos + 6;
        return json.compare(value_start, 4, "true") == 0;
    }

    // golf.md Phase 32.1 -- runs the bounded "Golf harder" search against
    // the currently golfed output and, only when it finds something
    // strictly smaller, stages it as a pending diff on the Diff tab rather
    // than replacing the golfed output silently (Phase 13's "nothing
    // changes silently" precedent).
    void run_golf_harder(HWND hwnd)
    {
        if (!g_gl_ready)
        {
            return;
        }

        std::string source = g_source_editor.text_utf8();
        UshaderGolfOptions options = to_golf_options(g_golf_controls.toggles());
        const std::string protected_names = effective_protected_names_for_golf();

        bool compression_based = false;
        int preset_index = g_golf_controls.budget_preset_index();
        if (preset_index >= 0)
        {
            std::size_t preset_count = 0;
            const BudgetPreset* presets = budget_presets(preset_count);
            if (static_cast<std::size_t>(preset_index) < preset_count)
            {
                compression_based = presets[preset_index].deflate_limit >= 0;
            }
        }

        UshaderGolfStats stats{};
        bool improved = false;
        char* applied_json = nullptr;
        char* harder = nullptr;
        if (g_deep_search_enabled)
        {
            // ROADMAP.md Phase 37.3 -- when the Twigl Export tab is the
            // active tab, score against the geekest-mode 280-character
            // tweet budget rather than Shadertoy raw/DEFLATE bytes.
            int32_t objective = g_tab_strip.active_index() == kTwiglExportTabIndex ? 2 : (compression_based ? 1 : 0);
            harder = ushader_golf_harder_deep(source.c_str(), options,
                protected_names.empty() ? nullptr : protected_names.c_str(), objective,
                500, 2000, &stats, &improved, &applied_json);
        }
        else
        {
            harder = ushader_golf_harder(source.c_str(), options,
                protected_names.empty() ? nullptr : protected_names.c_str(), compression_based,
                &stats, &improved, &applied_json);
        }

        if (harder != nullptr && improved && std::string(harder) != g_golfed_text_raw)
        {
            g_harder_pending_code = std::string(harder);
            g_harder_pending_stats = stats;
            g_harder_pending_frequency_aware_renaming = applied_json != nullptr
                ? applied_json_final_bool(std::string(applied_json), "frequency_aware_renaming", options.frequency_aware_renaming)
                : options.frequency_aware_renaming;
            g_harder_pending = true;
            g_diff_view.set_diff(compute_unified_diff(g_golfed_text_raw, g_harder_pending_code));
            switch_tab(hwnd, kDiffTabIndex);
        }
        else
        {
            g_harder_pending = false;
        }

        if (harder != nullptr) { ushader_free_string(harder); }
        if (applied_json != nullptr) { ushader_free_string(applied_json); }
    }

    void apply_harder_result(HWND hwnd)
    {
        if (!g_harder_pending)
        {
            return;
        }

        g_golfed_text_raw = g_harder_pending_code;
        refresh_golfed_view();
        g_twigl_export_panel.set_golfed_source(g_golfed_text_raw);

        std::string source = g_source_editor.text_utf8();
        g_diff_view.set_diff(compute_unified_diff(source, g_golfed_text_raw));

        UshaderBudgetResult budget = ushader_estimate_budget(g_golfed_text_raw.c_str());
        UshaderBudgetResult original_budget = ushader_estimate_budget(source.c_str());
        g_stats_panel.set_stats(g_harder_pending_stats, g_golfed_text_raw.size(), budget, original_budget, g_golf_controls.budget_preset_index(), true, g_harder_pending_frequency_aware_renaming);
        update_over_budget_hint(budget);

        std::string golfed_compile_error;
        g_golfed_gl_ready = g_golfed_runner.compile(g_golfed_text_raw, golfed_compile_error);

        g_harder_pending = false;
        layout_chrome(hwnd);
    }

    void load_shader_file(HWND hwnd, const std::string& utf8_path)
    {
        for (std::size_t i = 0; i < g_documents.size(); ++i)
        {
            if (g_documents[i].file_path == utf8_path)
            {
                switch_document(hwnd, static_cast<int>(i));
                switch_tab(hwnd, kSourceTabIndex);
                return;
            }
        }

        std::string content = read_utf8_file(utf8_path);
        sync_active_document_from_editor();
        EditorDocument doc;
        doc.file_path = utf8_path;
        doc.display_name = basename_from_path(utf8_path);
        doc.source_text = content;
        // A newly opened document inherits the currently active document's
        // golf profile (toggles/protected names/budget preset) as its
        // starting point, rather than the built-in defaults -- the common
        // case is opening a related shader while already tuned for the
        // project at hand. ROADMAP.md/roadmap_twigl.md Phase 44.3 extends
        // the same inheritance to the Twigl Export panel's state.
        doc.pass_toggles = g_golf_controls.toggles();
        doc.protected_names = g_golf_controls.protected_names();
        doc.budget_preset_index = g_golf_controls.budget_preset_index();
        doc.twigl_mode = g_twigl_export_panel.current_mode();
        doc.twigl_es300 = g_twigl_export_panel.current_es300();
        doc.twigl_mrt_targets = g_twigl_export_panel.current_mrt_targets();
        doc.twigl_has_backbuffer = g_twigl_export_panel.current_has_backbuffer();
        doc.twigl_has_sound = g_twigl_export_panel.current_has_sound();
        g_documents.push_back(doc);
        load_document_into_editor(hwnd, static_cast<int>(g_documents.size()) - 1);
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
            if (g_active_document >= 0 && g_active_document < static_cast<int>(g_documents.size()))
            {
                EditorDocument& doc = g_documents[static_cast<std::size_t>(g_active_document)];
                doc.file_path = *path;
                doc.display_name = basename_from_path(*path);
            }
        }
    }

    void open_command_palette_for(HWND hwnd)
    {
        std::vector<PaletteCommand> commands = {
            {"Switch to tab: Source", [hwnd]() { switch_tab(hwnd, kSourceTabIndex); }},
            {"Switch to tab: Golfed", [hwnd]() { switch_tab(hwnd, kGolfedTabIndex); }},
            {"Switch to tab: Diff", [hwnd]() { switch_tab(hwnd, kDiffTabIndex); }},
            {"Switch to tab: Trace", [hwnd]() { switch_tab(hwnd, kTraceTabIndex); }},
            {"Switch to tab: Stats", [hwnd]() { switch_tab(hwnd, kStatsTabIndex); }},
            {"Switch to tab: Viewport", [hwnd]() { switch_tab(hwnd, kViewportTabIndex); }},
            {"Switch to tab: Appearance", [hwnd]() { switch_tab(hwnd, kAppearanceTabIndex); }},
            {"Switch to tab: About", [hwnd]() { switch_tab(hwnd, kAboutTabIndex); }},
            {"Switch to tab: Twigl export", [hwnd]() { switch_tab(hwnd, kTwiglExportTabIndex); }},
            {"Run golf (recompile)", [hwnd]() { recompile_from_editor(); layout_chrome(hwnd); }},
            {"Reset to default shader", [hwnd]() { g_source_editor.set_text_utf8(kDefaultShaderSource); recompile_from_editor(); layout_chrome(hwnd); }},
            {"Toggle Formatted view", [hwnd]() { g_formatted_view = !g_formatted_view; refresh_golfed_view(); layout_chrome(hwnd); }},
            {"Toggle Compare mode", [hwnd]() { g_compare_mode = !g_compare_mode; switch_tab(hwnd, kViewportTabIndex); }},
            {"Switch to tab: Golf Tips", [hwnd]() { switch_tab(hwnd, kGolfTipsTabIndex); }},
            {"Open file...", [hwnd]() { open_file_action(hwnd); }},
            {"Save as...", [hwnd]() { save_file_action(hwnd); }},
            {"New document", [hwnd]() { new_document_action(hwnd); }},
            {"Close current document", [hwnd]() { close_document_action(hwnd, g_active_document); }},
            {"Copy as Shadertoy", []() { copy_as_shadertoy_action(); }},
            {"Copy as Bonzomatic", []() { copy_as_bonzomatic_action(); }},
            {"Copy as bare main()", []() { copy_as_bare_main_action(); }},
            {"Copy for twigl.app (selected mode)", []() { copy_as_twigl_action(); }},
            {"Import twigl shader into Source", [hwnd]() { import_twigl_shader_action(hwnd); }},
            {"Save golf profile...", [hwnd]() { save_profile_action(hwnd); }},
            {"Load golf profile...", [hwnd]() { load_profile_action(hwnd); }},
            {"Apply profile: Maximum", [hwnd]() { apply_profile(hwnd, builtin_profile_maximum(), g_golf_controls.protected_names(), g_golf_controls.budget_preset_index()); }},
            {"Apply profile: Safe", [hwnd]() { apply_profile(hwnd, builtin_profile_safe(), g_golf_controls.protected_names(), g_golf_controls.budget_preset_index()); }},
            {"Apply profile: None", [hwnd]() { apply_profile(hwnd, builtin_profile_none(), g_golf_controls.protected_names(), g_golf_controls.budget_preset_index()); }},
            {"Import exclude/protected name list...", [hwnd]() { import_exclude_list_action_from_palette(hwnd); }},
            {"Capture viewport as PNG...", [hwnd]() { capture_viewport_png_action(hwnd); }},
            {"Capture viewport as GIF (128x128, 60 frames)...", [hwnd]() { capture_viewport_gif_action(hwnd, kGifCapturePresets[0]); }},
            {"Capture viewport as GIF (256x256, 120 frames)...", [hwnd]() { capture_viewport_gif_action(hwnd, kGifCapturePresets[1]); }},
            {"Capture viewport as GIF (512x512, 180 frames)...", [hwnd]() { capture_viewport_gif_action(hwnd, kGifCapturePresets[2]); }},
            {"Export session report (HTML)...", [hwnd]() { export_session_report_action(hwnd); }},
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

    void render_mini_preview_frame(const std::chrono::steady_clock::time_point& start_time, int frame_index)
    {
        float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start_time).count();
        g_mini_viewport.make_current();
        int total_width = g_mini_viewport.width();
        int total_height = g_mini_viewport.height();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glViewport(0, 0, total_width, total_height);
        glClear(GL_COLOR_BUFFER_BIT);

        ShaderUniforms uniforms{};
        uniforms.time = elapsed;
        uniforms.resolution_x = static_cast<float>(kMiniPreviewWidth);
        uniforms.resolution_y = static_cast<float>(kMiniPreviewHeight);
        uniforms.frame = frame_index;
        uniforms.frame_rate = 60.0f;

        if (g_mini_source_gl_ready)
        {
            g_mini_source_fb.resize(kMiniPreviewWidth, kMiniPreviewHeight);
            g_mini_source_fb.bind();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            g_mini_source_runner.draw(uniforms);
            g_mini_source_fb.unbind(total_width, total_height);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, g_mini_source_fb.framebuffer_id());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, kMiniPreviewWidth, kMiniPreviewHeight,
                0, total_height - kMiniPreviewHeight, kMiniPreviewWidth, total_height,
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }

        if (g_mini_golfed_gl_ready)
        {
            g_mini_golfed_fb.resize(kMiniPreviewWidth, kMiniPreviewHeight);
            g_mini_golfed_fb.bind();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            g_mini_golfed_runner.draw(uniforms);
            g_mini_golfed_fb.unbind(total_width, total_height);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, g_mini_golfed_fb.framebuffer_id());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, kMiniPreviewWidth, kMiniPreviewHeight,
                0, 0, kMiniPreviewWidth, kMiniPreviewHeight,
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        g_mini_viewport.swap_buffers();
        g_viewport.make_current();
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
            if (g_tab_strip.active_index() == kGolfTipsTabIndex)
            {
                g_golf_tips_panel.on_mouse_move(x, y);
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
            Win32DocumentTabStrip::HitResult document_hit = g_document_tab_strip.hit_test(x, y);
            if (document_hit.kind == Win32DocumentTabStrip::HitKind::Document)
            {
                SetFocus(hwnd);
                switch_document(hwnd, document_hit.index);
            }
            else if (document_hit.kind == Win32DocumentTabStrip::HitKind::Close)
            {
                SetFocus(hwnd);
                close_document_action(hwnd, document_hit.index);
            }
            else if (document_hit.kind == Win32DocumentTabStrip::HitKind::NewDocument)
            {
                SetFocus(hwnd);
                new_document_action(hwnd);
            }
            else if (g_run_golf_button.contains(x, y))
            {
                SetFocus(hwnd);
                recompile_from_editor();
                layout_chrome(hwnd);
            }
            else if (g_golf_harder_button.contains(x, y))
            {
                SetFocus(hwnd);
                run_golf_harder(hwnd);
                layout_chrome(hwnd);
            }
            else if (g_deep_search_toggle_button.contains(x, y))
            {
                SetFocus(hwnd);
                g_deep_search_enabled = !g_deep_search_enabled;
            }
            else if (g_tab_strip.active_index() == kDiffTabIndex && g_harder_pending && g_apply_harder_button.contains(x, y))
            {
                SetFocus(hwnd);
                apply_harder_result(hwnd);
            }
            else if (g_tab_strip.active_index() == kGolfedTabIndex && g_formatted_view_button.contains(x, y))
            {
                SetFocus(hwnd);
                g_formatted_view = !g_formatted_view;
                refresh_golfed_view();
                layout_chrome(hwnd);
            }
            else if (tab_hit >= 0)
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
            else if (g_tab_strip.active_index() == kTraceTabIndex && g_over_budget_hint
                && y >= static_cast<int>(TitleBar::kHeight + Win32DocumentTabStrip::kHeight + TabStrip::kHeight)
                && y < static_cast<int>(TitleBar::kHeight + Win32DocumentTabStrip::kHeight + TabStrip::kHeight) + 28)
            {
                SetFocus(hwnd);
                switch_tab(hwnd, kGolfTipsTabIndex);
            }
            else if (g_tab_strip.active_index() == kTraceTabIndex && g_trace_view.contains(x, y))
            {
                SetFocus(hwnd);
                g_trace_view.on_mouse_down(x, y);
            }
            else if (g_golf_controls.contains(x, y))
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
            else if (g_tab_strip.active_index() == kTwiglExportTabIndex && g_twigl_export_panel.contains(x, y))
            {
                SetFocus(hwnd);
                g_twigl_export_panel.on_mouse_down(x, y);
                std::string snippet_source;
                if (g_twigl_export_panel.take_pending_snippet_insert(snippet_source))
                {
                    g_source_editor.insert_text_utf8(snippet_source);
                    layout_chrome(hwnd);
                }
                std::string copy_text;
                if (g_twigl_export_panel.take_pending_copy_request(copy_text))
                {
                    // ROADMAP.md/roadmap_twigl.md Phase 43.1 -- same
                    // guaranteed-correct text the command palette's "Copy
                    // for twigl.app" entry produces (copy_as_twigl_action
                    // below); both go through compute_export_text() so
                    // they can never diverge.
                    if (!copy_text.empty())
                    {
                        copy_text_to_clipboard(copy_text);
                        show_status_toast(L"Copied \u2014 paste into twigl.app");
                    }
                }
                std::string import_source;
                if (g_twigl_export_panel.take_pending_import(import_source))
                {
                    // ROADMAP.md/roadmap_twigl.md Phase 43.2 -- replaces the
                    // Source editor's contents wholesale (this is a full
                    // shader reconstruction, not a snippet to merge into
                    // whatever is already there).
                    g_source_editor.set_text_utf8(import_source);
                    show_status_toast(L"Imported into Source");
                    layout_chrome(hwnd);
                }
            }
            else if (g_tab_strip.active_index() == kGolfTipsTabIndex && g_golf_tips_panel.contains(x, y))
            {
                SetFocus(hwnd);
                g_golf_tips_panel.on_mouse_down(x, y);
                std::string golf_tip_snippet_source;
                if (g_golf_tips_panel.take_pending_snippet_insert(golf_tip_snippet_source))
                {
                    g_source_editor.insert_text_utf8(golf_tip_snippet_source);
                    layout_chrome(hwnd);
                }
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
            else if (g_tab_strip.active_index() == kTwiglExportTabIndex)
            {
                g_twigl_export_panel.on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wparam));
            }
            else if (g_tab_strip.active_index() == kGolfTipsTabIndex)
            {
                g_golf_tips_panel.on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wparam));
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
            if (g_golf_controls.on_char(static_cast<wchar_t>(wparam)))
            {
                return 0;
            }
            if (g_tab_strip.active_index() == kGolfTipsTabIndex && g_golf_tips_panel.on_char(static_cast<wchar_t>(wparam)))
            {
                return 0;
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
                // ROADMAP.md Phase 38.2 -- this chord now opens a new
                // document instead of resetting the current one in place.
                new_document_action(hwnd);
                return 0;
            }
            if (win32_chord_matches(g_keybindings.twigl_export_toggle, wparam, ctrl_held, shift_held, alt_held))
            {
                switch_tab(hwnd, g_tab_strip.active_index() == kTwiglExportTabIndex ? kSourceTabIndex : kTwiglExportTabIndex);
                return 0;
            }
            if (active_editor() != nullptr)
            {
                if (active_editor()->on_key_down(wparam, ctrl_held, shift_held))
                {
                    return 0;
                }
            }
            {
                if (g_golf_controls.on_key_down(wparam))
                {
                    return 0;
                }
            }
            if (g_tab_strip.active_index() == kGolfTipsTabIndex && g_golf_tips_panel.on_key_down(wparam))
            {
                return 0;
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
            save_session();
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
    if (!g_title_bar.create(g_render_target, g_dwrite_factory) || !g_tab_strip.create(g_render_target, g_dwrite_factory)
        || !g_document_tab_strip.create(g_render_target, g_dwrite_factory))
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
    if (!g_golf_tips_panel.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    g_dwrite_factory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, ui_font_pt(13.0f), L"en-us", &g_hint_text_format);
    if (g_hint_text_format != nullptr) { g_hint_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER); }
    if (!g_run_golf_button.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_golf_harder_button.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_deep_search_toggle_button.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_apply_harder_button.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    if (!g_formatted_view_button.create(g_render_target, g_dwrite_factory))
    {
        return 1;
    }
    // ROADMAP.md Phase 38.2 -- session-restore-on-launch: reconstruct every
    // document from the previous session if one was saved. File-backed
    // documents are re-read from disk (never trusted from the stale saved
    // copy); a document whose file has since gone missing is dropped
    // rather than shown blank. Falls back to a single default document
    // whenever no session exists or restore comes up empty.
    if (workspace_session_exists())
    {
        std::string session_text = read_utf8_file(workspace_session_path());
        WorkspaceState state;
        if (!session_text.empty() && deserialize_workspace(session_text, state))
        {
            for (std::size_t saved_index = 0; saved_index < state.documents.size(); ++saved_index)
            {
                const WorkspaceDocument& wdoc = state.documents[saved_index];
                EditorDocument doc;
                doc.file_path = wdoc.file_path;
                doc.pass_toggles = wdoc.pass_toggles;
                doc.protected_names = wdoc.protected_names;
                doc.budget_preset_index = wdoc.budget_preset_index;
                doc.twigl_mode = wdoc.twigl_mode;
                doc.twigl_es300 = wdoc.twigl_es300;
                doc.twigl_mrt_targets = wdoc.twigl_mrt_targets;
                doc.twigl_has_backbuffer = wdoc.twigl_has_backbuffer;
                doc.twigl_has_sound = wdoc.twigl_has_sound;
                if (!wdoc.file_path.empty())
                {
                    std::string disk_content = read_utf8_file(wdoc.file_path);
                    if (disk_content.empty())
                    {
                        continue;
                    }
                    doc.source_text = disk_content;
                    doc.display_name = basename_from_path(wdoc.file_path);
                }
                else
                {
                    doc.source_text = wdoc.unsaved_source;
                    doc.display_name = "Untitled " + std::to_string(g_next_untitled_number++);
                }
                if (static_cast<int>(saved_index) == state.active_document)
                {
                    g_active_document = static_cast<int>(g_documents.size());
                }
                g_documents.push_back(doc);
            }
        }
        if (g_active_document < 0 || g_active_document >= static_cast<int>(g_documents.size()))
        {
            g_active_document = 0;
        }
        if (state.active_tab >= 0)
        {
            g_tab_strip.switch_to(state.active_tab);
        }
    }
    if (g_documents.empty())
    {
        EditorDocument doc;
        doc.display_name = "Untitled " + std::to_string(g_next_untitled_number++);
        doc.source_text = kDefaultShaderSource;
        g_documents.push_back(doc);
        g_active_document = 0;
    }
    g_source_editor.set_text_utf8(g_documents[static_cast<std::size_t>(g_active_document)].source_text);
    g_golf_controls.set_toggles(g_documents[static_cast<std::size_t>(g_active_document)].pass_toggles);
    g_golf_controls.set_protected_names(g_documents[static_cast<std::size_t>(g_active_document)].protected_names);
    g_golf_controls.set_budget_preset_index(g_documents[static_cast<std::size_t>(g_active_document)].budget_preset_index);
    g_twigl_export_panel.restore_state(g_documents[static_cast<std::size_t>(g_active_document)].twigl_mode,
        g_documents[static_cast<std::size_t>(g_active_document)].twigl_es300,
        g_documents[static_cast<std::size_t>(g_active_document)].twigl_mrt_targets,
        g_documents[static_cast<std::size_t>(g_active_document)].twigl_has_backbuffer,
        g_documents[static_cast<std::size_t>(g_active_document)].twigl_has_sound);
    g_icons.load(exe_directory() + L"\\assets\\icons\\ui");

    RECT client_rect{};
    GetClientRect(hwnd, &client_rect);
    layout_chrome(hwnd);

    int viewport_top = static_cast<int>(TitleBar::kHeight + Win32DocumentTabStrip::kHeight + TabStrip::kHeight);
    int viewport_height = (client_rect.bottom - client_rect.top) - viewport_top;
    if (!g_viewport.create(hwnd, 0, viewport_top, client_rect.right - client_rect.left, viewport_height))
    {
        return 1;
    }
    if (!g_mini_viewport.create(hwnd, 0, viewport_top, kMiniPreviewWidth, kMiniPreviewHeight * 2 + kMiniPreviewGap))
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

        render_viewport_frame(start_time, frame_index);
        render_mini_preview_frame(start_time, frame_index);
        ++frame_index;
        paint_chrome(hwnd, title);
    }

    g_shader_runner.destroy();
    g_golfed_runner.destroy();
    g_shimmer_runner.destroy();
    g_compare_source_fb.destroy();
    g_compare_golfed_fb.destroy();
    g_viewport.destroy();

    g_mini_viewport.make_current();
    g_mini_source_runner.destroy();
    g_mini_golfed_runner.destroy();
    g_mini_source_fb.destroy();
    g_mini_golfed_fb.destroy();
    g_mini_viewport.destroy();
    g_title_bar.destroy();
    g_document_tab_strip.destroy();
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
    g_golf_tips_panel.destroy();
    if (g_hint_text_format != nullptr) { g_hint_text_format->Release(); g_hint_text_format = nullptr; }
    g_run_golf_button.destroy();
    g_golf_harder_button.destroy();
    g_deep_search_toggle_button.destroy();
    g_apply_harder_button.destroy();
    g_formatted_view_button.destroy();
    accessibility_shutdown();
    g_icons.release();
    release_device_resources();
    if (g_dwrite_factory != nullptr) { g_dwrite_factory->Release(); }
    if (g_d2d_factory != nullptr) { g_d2d_factory->Release(); }

    Gdiplus::GdiplusShutdown(g_gdiplus_token);

    return 0;
}
