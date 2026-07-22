#include "win32_about_panel.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>
#include <shellapi.h>

#include <cwchar>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "ushader/version.h"
#include "../platform/accessibility_core.h"
#include "../platform/utf8.h"

namespace
{
    struct LinkEntry
    {
        const wchar_t* label;
        const wchar_t* url;
    };

    const LinkEntry kLinks[] = {
        { L"contact.shaderstudio@gmail.com", L"mailto:contact.shaderstudio@gmail.com" },
        { L"https://github.com/Patrickjaillet", L"https://github.com/Patrickjaillet" },
        { L"https://github.com/Patrickjaillet/MicroShader", L"https://github.com/Patrickjaillet/MicroShader" },
    };

    constexpr int kVersionRow = 0;
    constexpr int kHairlineRow1 = 1;
    constexpr int kCopyrightRow = 2;
    constexpr int kRightsRow = 3;
    constexpr int kLinksFirstRow = 4;
    constexpr int kHairlineRow2 = kLinksFirstRow + Win32AboutPanel::kLinkCount;
    constexpr int kLicenseRow = kHairlineRow2 + 1;
    constexpr int kNoticeRow = kHairlineRow2 + 2;
}

bool Win32AboutPanel::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &text_format)))
    {
        return false;
    }
    text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    return SUCCEEDED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush));
}

void Win32AboutPanel::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32AboutPanel::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
}

bool Win32AboutPanel::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

RECT Win32AboutPanel::row_rect(int row) const
{
    LONG top = origin_y + 12 + row * static_cast<LONG>(kRowHeight);
    return RECT{ origin_x + 8, top, origin_x + 8 + 480, top + static_cast<LONG>(kRowHeight) };
}

RECT Win32AboutPanel::link_rect(int index) const
{
    return row_rect(kLinksFirstRow + index);
}

void Win32AboutPanel::on_mouse_move(int client_x, int client_y)
{
    POINT pt{ client_x, client_y };
    hovered_link = -1;
    for (int i = 0; i < kLinkCount; ++i)
    {
        RECT rect = link_rect(i);
        if (PtInRect(&rect, pt))
        {
            hovered_link = i;
            break;
        }
    }
}

bool Win32AboutPanel::on_mouse_down(int client_x, int client_y)
{
    POINT pt{ client_x, client_y };
    for (int i = 0; i < kLinkCount; ++i)
    {
        RECT rect = link_rect(i);
        if (PtInRect(&rect, pt))
        {
            ShellExecuteW(nullptr, L"open", kLinks[i].url, nullptr, nullptr, SW_SHOWNORMAL);
            return true;
        }
    }
    return false;
}

void Win32AboutPanel::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_app);

    if (dynamic_brush == nullptr || text_format == nullptr)
    {
        return;
    }

    auto draw_line = [&](int row, const wchar_t* text, ID2D1SolidColorBrush* color_brush)
    {
        RECT rect = row_rect(row);
        D2D1_RECT_F rect_f = D2D1::RectF(static_cast<float>(rect.left), static_cast<float>(rect.top),
            static_cast<float>(rect.right), static_cast<float>(rect.bottom));
        render_target->DrawText(text, static_cast<UINT32>(wcslen(text)), text_format, rect_f, color_brush);
    };

    auto draw_hairline = [&](int row)
    {
        RECT rect = row_rect(row);
        D2D1_RECT_F rect_f = D2D1::RectF(static_cast<float>(rect.left),
            static_cast<float>(rect.top) + kRowHeight * 0.5f - 0.5f,
            static_cast<float>(rect.left) + 420.0f,
            static_cast<float>(rect.top) + kRowHeight * 0.5f + 0.5f);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
        render_target->FillRectangle(rect_f, dynamic_brush);
    };

    wchar_t version_line[64];
    swprintf_s(version_line, L"uShader %hs", USHADER_VERSION_STRING);
    draw_line(kVersionRow, version_line, brushes.text_primary);

    draw_hairline(kHairlineRow1);

    draw_line(kCopyrightRow, L"Copyright (c) 2026 SANDEFJORD DEVELOPMENT (Patrick JAILLET)", brushes.text_secondary);
    draw_line(kRightsRow, L"All rights reserved", brushes.text_secondary);

    for (int i = 0; i < kLinkCount; ++i)
    {
        RECT rect = link_rect(i);
        D2D1_RECT_F link_rect_f = D2D1::RectF(static_cast<float>(rect.left), static_cast<float>(rect.top),
            static_cast<float>(rect.right), static_cast<float>(rect.bottom));
        bool hovered = (hovered_link == i);
        dynamic_brush->SetColor(hovered
            ? D2D1::ColorF(tokens::accent_hover.x, tokens::accent_hover.y, tokens::accent_hover.z)
            : D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z));
        render_target->DrawText(kLinks[i].label, static_cast<UINT32>(wcslen(kLinks[i].label)), text_format, link_rect_f, dynamic_brush);

        accessibility_register(wide_to_utf8(kLinks[i].label).c_str(), AccessibleRole::Button,
            link_rect_f.left, link_rect_f.top, link_rect_f.right - link_rect_f.left, link_rect_f.bottom - link_rect_f.top, true);
    }

    draw_hairline(kHairlineRow2);

    draw_line(kLicenseRow, L"MIT License", brushes.text_secondary);
    draw_line(kNoticeRow, L"see THIRD_PARTY_NOTICES.md", brushes.text_secondary);
}
