#include "win32_tab_strip.h"

#include <d2d1.h>
#include <dwrite.h>

#include <cstdio>
#include <cstring>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "../platform/accessibility_core.h"

namespace
{
    // ROADMAP.md Phase 38.1 -- workspace restructure: tabs are ordered into
    // three semantic groups (Author / Analyze / Export) plus a Settings
    // catch-all for the two tabs that don't fit any of the three named
    // groups, with a visible separator drawn before each new group's first
    // tab (see kGroupBoundaries below). Every `kXxxTabIndex` constant in
    // main_win32.cpp was renumbered to match this order.
    const wchar_t* kTabLabels[] = { L"Source", L"Golfed", L"Diff", L"Viewport", L"Trace", L"Stats", L"Golf Tips", L"Twigl", L"Appearance", L"About" };
    const char* kTabNames[] = { "Source", "Golfed", "Diff", "Viewport", "Trace", "Stats", "Golf Tips", "Twigl", "Appearance", "About" };

    // Tab indices that start a new group (Author=0, Analyze=4, Export=7,
    // Settings=8): a wider gap and a divider line are drawn immediately
    // before each of these.
    const int kGroupBoundaries[] = { 4, 7, 8 };
    constexpr float kGroupGap = 14.0f;

    bool starts_new_group(int index)
    {
        for (int boundary : kGroupBoundaries)
        {
            if (index == boundary)
            {
                return true;
            }
        }
        return false;
    }

    float group_gap_before(int index)
    {
        float gap = 0.0f;
        for (int boundary : kGroupBoundaries)
        {
            if (index >= boundary)
            {
                gap += kGroupGap;
            }
        }
        return gap;
    }
}

bool TabStrip::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    HRESULT hr = dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &text_format);
    if (FAILED(hr))
    {
        return false;
    }

    text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    if (FAILED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush)))
    {
        return false;
    }

    for (int i = 0; i < kTabCount; ++i)
    {
        hover_anim[i].snap_to(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 0.0f);
    }
    open_anim.snap_to(1.0f, 1.0f, 1.0f, 0.0f);
    open_anim.set_target(1.0f, 1.0f, 1.0f, 1.0f, 0.22f);
    return true;
}

void TabStrip::destroy()
{
    if (text_format != nullptr)
    {
        text_format->Release();
        text_format = nullptr;
    }
    if (dynamic_brush != nullptr)
    {
        dynamic_brush->Release();
        dynamic_brush = nullptr;
    }
}

void TabStrip::layout(int origin_x, int origin_y, int window_width)
{
    left = origin_x;
    top = origin_y;
    (void)window_width;
}

int TabStrip::hit_test(int x, int y) const
{
    if (y < top || y >= top + static_cast<int>(kHeight))
    {
        return -1;
    }
    int relative_x = x - left;
    if (relative_x < 0)
    {
        return -1;
    }
    for (int index = 0; index < kTabCount; ++index)
    {
        float tab_left = kTabWidth * static_cast<float>(index) + group_gap_before(index);
        float tab_right = tab_left + kTabWidth;
        if (static_cast<float>(relative_x) >= tab_left && static_cast<float>(relative_x) < tab_right)
        {
            return index;
        }
    }
    return -1;
}

void TabStrip::set_hover(int index)
{
    if (index == hovered_index)
    {
        return;
    }
    if (hovered_index >= 0 && hovered_index < kTabCount)
    {
        hover_anim[hovered_index].set_target(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 0.0f);
    }
    hovered_index = index;
    if (hovered_index >= 0 && hovered_index < kTabCount)
    {
        hover_anim[hovered_index].set_target(tokens::bg_hover.x, tokens::bg_hover.y, tokens::bg_hover.z, 1.0f);
    }
}

void TabStrip::set_focused(bool focused_value)
{
    focused = focused_value;
}

void TabStrip::switch_to(int index)
{
    if (index >= 0 && index < kTabCount)
    {
        active = index;
    }
}

void TabStrip::tick()
{
    for (int i = 0; i < kTabCount; ++i)
    {
        hover_anim[i].tick();
    }
    open_anim.tick();
}

void TabStrip::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    float total_width = kTabWidth * static_cast<float>(kTabCount) + group_gap_before(kTabCount - 1) + kGroupGap;
    D2D1_RECT_F strip_rect = D2D1::RectF(
        static_cast<float>(left), static_cast<float>(top),
        static_cast<float>(left) + total_width, static_cast<float>(top) + kHeight);
    render_target->FillRectangle(strip_rect, brushes.bg_app);

    float open_r, open_g, open_b, open_alpha;
    open_anim.current(open_r, open_g, open_b, open_alpha);
    (void)open_r; (void)open_g; (void)open_b;
    float slide_offset = (1.0f - open_alpha) * -24.0f;

    for (int index = 0; index < kTabCount; ++index)
    {
        float tab_left = static_cast<float>(left) + kTabWidth * static_cast<float>(index) + group_gap_before(index) + slide_offset;

        if (starts_new_group(index) && dynamic_brush != nullptr)
        {
            float divider_x = tab_left - kGroupGap * 0.5f;
            dynamic_brush->SetColor(D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
            D2D1_RECT_F divider_rect = D2D1::RectF(divider_x - 0.5f, static_cast<float>(top) + 6.0f,
                divider_x + 0.5f, static_cast<float>(top) + kHeight - 6.0f);
            render_target->FillRectangle(divider_rect, dynamic_brush);
        }

        D2D1_RECT_F tab_rect = D2D1::RectF(tab_left + 2.0f, static_cast<float>(top) + 2.0f,
            tab_left + kTabWidth - 2.0f, static_cast<float>(top) + kHeight - 2.0f);
        D2D1_ROUNDED_RECT rounded_rect = D2D1::RoundedRect(tab_rect, kCornerRadius, kCornerRadius);

        bool is_active = index == active;
        ID2D1SolidColorBrush* fill = is_active ? brushes.bg_panel_raised : brushes.bg_panel;
        render_target->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), nullptr, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::IdentityMatrix(), open_alpha), nullptr);
        render_target->FillRoundedRectangle(rounded_rect, fill);

        if (dynamic_brush != nullptr)
        {
            float r, g, b, a;
            hover_anim[index].current(r, g, b, a);
            if (a > 0.0f)
            {
                dynamic_brush->SetColor(D2D1::ColorF(r, g, b, a));
                render_target->FillRoundedRectangle(rounded_rect, dynamic_brush);
            }
        }

        if (is_active && focused)
        {
            render_target->DrawRoundedRectangle(rounded_rect, brushes.accent, 1.0f);
        }

        if (text_format != nullptr)
        {
            render_target->DrawText(kTabLabels[index], static_cast<UINT32>(wcslen(kTabLabels[index])),
                text_format, tab_rect, is_active ? brushes.text_primary : brushes.text_secondary);
        }

        render_target->PopLayer();

        char name_buffer[40];
        std::snprintf(name_buffer, sizeof(name_buffer), "Tab: %s%s", kTabNames[index], is_active ? " (active)" : "");
        accessibility_register(name_buffer, AccessibleRole::Button,
            tab_left, static_cast<float>(top), kTabWidth, kHeight, true);
    }
}
