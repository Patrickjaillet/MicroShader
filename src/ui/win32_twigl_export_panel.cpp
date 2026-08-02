#include "win32_twigl_export_panel.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include <cstdio>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "budget_presets.h"
#include "theme_tokens.h"
#include "../platform/utf8.h"
#include "../platform/accessibility_core.h"

namespace
{
    const wchar_t* kModeLabels[4] = { L"Classic", L"Geek", L"Geeker", L"Geekest" };
    const char* kModeNames[4] = { "Classic", "Geek", "Geeker", "Geekest" };
    const char* kSnippetNames[10] = {
        "PI", "PI2", "hsv", "rotate2D", "rotate3D",
        "fsnoise", "fsnoiseDigits", "snoise2D", "snoise3D", "snoise4D",
    };

    constexpr int kButtonHeight = 26;
    constexpr int kButtonGap = 6;
    constexpr int kRowGap = 8;
    constexpr int kPanelPadding = 8;

    void register_button(const Win32ToolButton& button, const std::string& name)
    {
        accessibility_register(name.c_str(), AccessibleRole::Button,
            static_cast<float>(button.origin_x_px()), static_cast<float>(button.origin_y_px()),
            static_cast<float>(button.width_in_px()), static_cast<float>(button.height_in_px()), true);
    }

    void register_toggle(const Win32ToolButton& button, const std::string& name, bool checked)
    {
        accessibility_register_toggle(name.c_str(), AccessibleRole::CheckBox,
            static_cast<float>(button.origin_x_px()), static_cast<float>(button.origin_y_px()),
            static_cast<float>(button.width_in_px()), static_cast<float>(button.height_in_px()), true, checked);
    }
}

bool Win32TwiglExportPanel::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    for (Win32ToolButton& button : mode_buttons)
    {
        if (!button.create(render_target, dwrite_factory)) { return false; }
    }
    if (!es300_button.create(render_target, dwrite_factory)) { return false; }
    if (!mrt_button.create(render_target, dwrite_factory)) { return false; }
    if (!backbuffer_button.create(render_target, dwrite_factory)) { return false; }
    if (!sound_button.create(render_target, dwrite_factory)) { return false; }
    for (Win32ToolButton& button : snippet_buttons)
    {
        if (!button.create(render_target, dwrite_factory)) { return false; }
    }
    if (!preview_editor.create(render_target, dwrite_factory, true)) { return false; }

    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &label_format)))
    {
        return false;
    }

    if (FAILED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush)))
    {
        return false;
    }

    recompute_preview();
    return true;
}

void Win32TwiglExportPanel::destroy()
{
    for (Win32ToolButton& button : mode_buttons) { button.destroy(); }
    es300_button.destroy();
    mrt_button.destroy();
    backbuffer_button.destroy();
    sound_button.destroy();
    for (Win32ToolButton& button : snippet_buttons) { button.destroy(); }
    preview_editor.destroy();

    if (label_format != nullptr) { label_format->Release(); label_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32TwiglExportPanel::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;

    int row_y = y + kPanelPadding;
    int button_width = 96;

    int col_x = x + kPanelPadding;
    for (Win32ToolButton& button : mode_buttons)
    {
        button.layout(col_x, row_y, button_width, kButtonHeight);
        col_x += button_width + kButtonGap;
    }

    row_y += kButtonHeight + kRowGap;
    col_x = x + kPanelPadding;
    es300_button.layout(col_x, row_y, button_width, kButtonHeight);
    col_x += button_width + kButtonGap;
    mrt_button.layout(col_x, row_y, button_width, kButtonHeight);
    col_x += button_width + kButtonGap;
    backbuffer_button.layout(col_x, row_y, button_width, kButtonHeight);
    col_x += button_width + kButtonGap;
    sound_button.layout(col_x, row_y, button_width, kButtonHeight);

    row_y += kButtonHeight + kRowGap;
    int snippet_col_x = x + kPanelPadding;
    int snippets_per_row = 5;
    int snippet_width = 88;
    for (int i = 0; i < kSnippetCount; ++i)
    {
        int column = i % snippets_per_row;
        int row = i / snippets_per_row;
        snippet_buttons[i].layout(snippet_col_x + column * (snippet_width + kButtonGap),
            row_y + row * (kButtonHeight + kButtonGap), snippet_width, kButtonHeight);
    }
    int snippet_rows = (kSnippetCount + snippets_per_row - 1) / snippets_per_row;
    row_y += snippet_rows * (kButtonHeight + kButtonGap) + kRowGap;

    int budget_label_height = 22;
    row_y += budget_label_height;

    int preview_top = row_y;
    int preview_height = (y + height) - preview_top;
    if (preview_height < 0) { preview_height = 0; }
    preview_editor.layout(x, preview_top, width, preview_height);
}

void Win32TwiglExportPanel::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    if (dynamic_brush == nullptr || label_format == nullptr)
    {
        return;
    }

    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_app);

    for (int i = 0; i < kModeCount; ++i)
    {
        mode_buttons[i].paint(render_target, brushes, kModeLabels[i], mode == i);
        register_button(mode_buttons[i], std::string("Twigl mode: ") + kModeNames[i]);
    }
    es300_button.paint(render_target, brushes, L"ES 300", es300);
    register_toggle(es300_button, "Twigl ES 300 es output", es300);
    mrt_button.paint(render_target, brushes, L"MRT x2", mrt_targets == 2);
    register_toggle(mrt_button, "Twigl MRT (2 targets)", mrt_targets == 2);
    backbuffer_button.paint(render_target, brushes, L"Backbuffer", has_backbuffer);
    register_toggle(backbuffer_button, "Twigl backbuffer uniform", has_backbuffer);
    sound_button.paint(render_target, brushes, L"Sound", has_sound);
    register_toggle(sound_button, "Twigl sound uniform", has_sound);

    for (int i = 0; i < kSnippetCount; ++i)
    {
        std::wstring label = utf8_to_wide(kSnippetNames[i]);
        snippet_buttons[i].paint(render_target, brushes, label, false);
        register_button(snippet_buttons[i], std::string("Insert Twigl snippet: ") + kSnippetNames[i]);
    }

    char budget_text[160];
    std::size_t preset_count = 0;
    const BudgetPreset* presets = budget_presets(preset_count);
    long long tweet_limit = -1;
    for (std::size_t i = 0; i < preset_count; ++i)
    {
        if (std::string(presets[i].name) == "X/Twitter shader")
        {
            tweet_limit = presets[i].raw_limit;
            break;
        }
    }
    bool over = tweet_limit >= 0 && static_cast<long long>(last_budget.raw_bytes) > tweet_limit;
    std::snprintf(budget_text, sizeof(budget_text), "%s Raw: %zu bytes  Deflate: %zu bytes  (tweet budget: %lld)",
        over ? "X" : "OK",
        static_cast<size_t>(last_budget.raw_bytes), static_cast<size_t>(last_budget.deflate_bytes), tweet_limit);

    int snippets_per_row = 5;
    int snippet_rows = (kSnippetCount + snippets_per_row - 1) / snippets_per_row;
    float budget_y = static_cast<float>(origin_y + kPanelPadding
        + (kButtonHeight + kRowGap) * 2
        + snippet_rows * (kButtonHeight + kButtonGap) + kRowGap);
    D2D1_RECT_F budget_rect = D2D1::RectF(static_cast<float>(origin_x + kPanelPadding), budget_y,
        static_cast<float>(origin_x + width_px - kPanelPadding), budget_y + 20.0f);
    dynamic_brush->SetColor(over
        ? D2D1::ColorF(tokens::status_error.x, tokens::status_error.y, tokens::status_error.z)
        : D2D1::ColorF(tokens::status_ok.x, tokens::status_ok.y, tokens::status_ok.z));
    std::wstring budget_wide = utf8_to_wide(budget_text);
    render_target->DrawText(budget_wide.c_str(), static_cast<UINT32>(budget_wide.size()), label_format, budget_rect, dynamic_brush);

    preview_editor.paint(render_target, brushes);
}

bool Win32TwiglExportPanel::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

bool Win32TwiglExportPanel::on_mouse_down(int client_x, int client_y)
{
    for (int i = 0; i < kModeCount; ++i)
    {
        if (mode_buttons[i].contains(client_x, client_y))
        {
            mode = i;
            recompute_preview();
            return true;
        }
    }
    if (es300_button.contains(client_x, client_y))
    {
        es300 = !es300;
        recompute_preview();
        return true;
    }
    if (mrt_button.contains(client_x, client_y))
    {
        mrt_targets = (mrt_targets == 2) ? 1 : 2;
        recompute_preview();
        return true;
    }
    if (backbuffer_button.contains(client_x, client_y))
    {
        has_backbuffer = !has_backbuffer;
        return true;
    }
    if (sound_button.contains(client_x, client_y))
    {
        has_sound = !has_sound;
        return true;
    }
    for (int i = 0; i < kSnippetCount; ++i)
    {
        if (snippet_buttons[i].contains(client_x, client_y))
        {
            char* snippet_source = ushader_twigl_snippet(kSnippetNames[i]);
            if (snippet_source != nullptr)
            {
                pending_snippet_insert_source = std::string(snippet_source);
                has_pending_snippet_insert = true;
                ushader_free_string(snippet_source);
            }
            return false;
        }
    }
    return false;
}

void Win32TwiglExportPanel::on_mouse_wheel(int wheel_delta)
{
    preview_editor.on_mouse_wheel(wheel_delta);
}

void Win32TwiglExportPanel::tick()
{
    preview_editor.tick();
}
void Win32TwiglExportPanel::set_golfed_source(const std::string& golfed_source)
{
    golfed_source_cache = golfed_source;
    recompute_preview();
}

bool Win32TwiglExportPanel::take_pending_snippet_insert(std::string& out_source)
{
    if (!has_pending_snippet_insert)
    {
        return false;
    }
    out_source = pending_snippet_insert_source;
    has_pending_snippet_insert = false;
    return true;
}

void Win32TwiglExportPanel::recompute_preview()
{
    char* rewritten = (mrt_targets == 2)
        ? ushader_twigl_rewrite_mrt(golfed_source_cache.c_str(), mode, mrt_targets)
        : ushader_twigl_rewrite(golfed_source_cache.c_str(), mode, es300);

    std::string preview_text = rewritten != nullptr ? std::string(rewritten) : std::string();
    if (rewritten != nullptr)
    {
        ushader_free_string(rewritten);
    }

    preview_editor.set_text_utf8(preview_text);
    last_budget = ushader_estimate_budget(preview_text.c_str());
}

