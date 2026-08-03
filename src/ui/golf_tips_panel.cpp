#include "golf_tips_panel.h"

#define NOMINMAX
#include <d2d1.h>
#include <dwrite.h>

#include <algorithm>
#include <cctype>
#include <cwchar>
#include <cwctype>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"
#include "../platform/utf8.h"
#include "../platform/accessibility_core.h"

namespace
{
    struct GolfTipEntry
    {
        const wchar_t* category;
        const wchar_t* source_catalogue;
        const wchar_t* title;
        const wchar_t* description;
        const char* snippet;
    };

    // golf.md Phase 33.1 -- values kept numerically consistent with
    // rust-core/src/neyret.rs's ROTATION_CONSTANTS/NEYRET_HASH_SNIPPETS
    // (Phase 35 catalogue slice), reference survey section 3 above.
    const GolfTipEntry kEntries[] = {
        { L"Rotation constant", L"Fabrice Neyret's catalogue",
          L"30° rotation matrix",
          L"mat2(cos(a),sin(a),-sin(a),cos(a)) for a fixed 30-degree angle, as a clean 2-digit literal instead of a runtime cos()/sin() call.",
          "mat2(.87,.5,-.5,.87)" },
        { L"Rotation constant", L"Fabrice Neyret's catalogue",
          L"36.87° rotation matrix (3-4-5 triangle, exact)",
          L"The 3-4-5 triangle angle: .8/.6 satisfy sin^2+cos^2=1 exactly, so this one is not an approximation at all.",
          "mat2(.8,.6,-.6,.8)" },
        { L"Rotation constant", L"Fabrice Neyret's catalogue",
          L"45° rotation matrix",
          L"mat2(cos(a),sin(a),-sin(a),cos(a)) for a fixed 45-degree angle.",
          "mat2(.71,.71,-.71,.71)" },
        { L"Rotation constant", L"Fabrice Neyret's catalogue",
          L"60° rotation matrix",
          L"mat2(cos(a),sin(a),-sin(a),cos(a)) for a fixed 60-degree angle.",
          "mat2(.5,.87,-.87,.5)" },
        { L"Identity substitution", L"Xor's Mini: Code Golfing",
          L"pow(x,2.) -> x*x",
          L"Squaring via multiplication is shorter than a pow() call and avoids pow()'s general-exponent overhead.",
          "x*x" },
        { L"Identity substitution", L"Xor's Mini: Code Golfing",
          L"sqrt(dot(v,v)) -> length(v)",
          L"length() is the built-in name for exactly this computation -- shorter and communicates intent.",
          "length(v)" },
        { L"Identity substitution", L"Xor's Mini: Code Golfing",
          L"min(max(x,0.),1.) -> clamp(x,0.,1.)",
          L"clamp() is the built-in name for exactly this nested min/max -- shorter and avoids repeating x.",
          "clamp(x,0.,1.)" },
        { L"Hash/noise snippet", L"Fabrice Neyret's catalogue",
          L"hash11 -- 1D to 1D",
          L"Single-multiply fract-sin hash, one float in, one float out.",
          "float hash11(float p){return fract(sin(p*127.1)*43758.5453);}" },
        { L"Hash/noise snippet", L"Fabrice Neyret's catalogue",
          L"hash12 -- 2D to 1D",
          L"fract-sin-dot hash with small-integer dot constants, one vec2 in, one float out.",
          "float hash12(vec2 p){return fract(sin(dot(p,vec2(41.,289.)))*43758.5453);}" },
        { L"Hash/noise snippet", L"Fabrice Neyret's catalogue",
          L"hash21 -- 1D to 2D",
          L"Two independent channels derived from a single float input.",
          "vec2 hash21(float p){return fract(sin(p+vec2(0.,52.7))*vec2(43758.5453,28001.8384));}" },
        { L"Hash/noise snippet", L"Fabrice Neyret's catalogue",
          L"hash22 -- 2D to 2D",
          L"Two independent dot products, one vec2 in, one vec2 out.",
          "vec2 hash22(vec2 p){return fract(sin(vec2(dot(p,vec2(41.,289.)),dot(p,vec2(127.1,311.7))))*43758.5453);}" },
        { L"Hash/noise snippet", L"Fabrice Neyret's catalogue",
          L"hash13 -- 3D to 1D",
          L"Complements twigl's 2D-only fsnoise with a 3D-input variant, one vec3 in, one float out.",
          "float hash13(vec3 p){return fract(sin(dot(p,vec3(41.,289.,157.)))*43758.5453);}" },
        // golf.md Phase 35.3 -- values kept textually consistent with
        // rust-core/src/neyret.rs's RAYMARCH_LOOP_IDIOMS. Riskier than the
        // entries above: drops an early-exit break entirely, relying on
        // numeric tolerance rather than an exact rewrite -- the per-entry
        // description states the caveat explicitly, on top of the panel's
        // own persistent disclaimer footer.
        { L"Loop compaction idiom", L"Fabrice Neyret's catalogue",
          L"Break-free raymarch accumulation",
          L"Drops the early-exit break: once within epsilon of the surface, map() returns close enough to zero that further iterations barely move t. Correct within the same visual tolerance the break threshold already accepted, not bit-identical.",
          "float t=0.;for(int i=0;i<64;i++)t+=map(ro+rd*t);" },
        { L"Loop compaction idiom", L"Fabrice Neyret's catalogue",
          L"Branch-free fractal escape counting",
          L"Replaces the early-exit break with a step()-gated accumulator. Changes z's trajectory after escape (it keeps iterating), which only matters if z is read again after the loop -- safe when only the escape count is used for coloring.",
          "vec2 z=uv;float m=0.;for(int i=0;i<8;i++)z=vec2(z.x*z.x-z.y*z.y,2.*z.x*z.y)+c,m+=step(dot(z,z),4.);" },
        // ROADMAP.md Phase 36 -- values kept textually consistent with
        // rust-core/src/iq.rs's IQ_SDF_SNIPPETS/IQ_HASH_SNIPPETS/
        // IQ_PALETTE_PRESETS/IQ_TONEMAP_SNIPPETS. Offered alongside, not
        // replacing, the Fabrice Neyret entries above (Phase 36.2 is a
        // distinct hash-function lineage from Phase 35.2's).
        { L"SDF primitive", L"Inigo Quilez's iquilezles.org articles",
          L"sdSphere",
          L"Signed distance to a sphere of radius r centered at the origin.",
          "float sdSphere(vec3 p,float r){return length(p)-r;}" },
        { L"SDF primitive", L"Inigo Quilez's iquilezles.org articles",
          L"sdBox",
          L"Signed distance to an axis-aligned box with half-extents b.",
          "float sdBox(vec3 p,vec3 b){vec3 q=abs(p)-b;return length(max(q,0.))+min(max(q.x,max(q.y,q.z)),0.);}" },
        { L"SDF primitive", L"Inigo Quilez's iquilezles.org articles",
          L"sdPlane",
          L"Signed distance to a plane with unit normal n, offset h from the origin.",
          "float sdPlane(vec3 p,vec3 n,float h){return dot(p,n)+h;}" },
        { L"SDF primitive", L"Inigo Quilez's iquilezles.org articles",
          L"sdTorus",
          L"Signed distance to a torus: t.x is the ring radius, t.y is the tube radius.",
          "float sdTorus(vec3 p,vec2 t){vec2 q=vec2(length(p.xz)-t.x,p.y);return length(q)-t.y;}" },
        { L"SDF primitive", L"Inigo Quilez's iquilezles.org articles",
          L"sdCapsule",
          L"Signed distance to a capsule (rounded cylinder) between points a and b, radius r.",
          "float sdCapsule(vec3 p,vec3 a,vec3 b,float r){vec3 pa=p-a,ba=b-a;float h=clamp(dot(pa,ba)/dot(ba,ba),0.,1.);return length(pa-ba*h)-r;}" },
        { L"SDF operator", L"Inigo Quilez's iquilezles.org articles",
          L"opUnion / opSubtraction / opIntersection",
          L"Boolean combination of two SDFs -- union is min(), subtraction is max(-d1,d2), intersection is max().",
          "float opUnion(float d1,float d2){return min(d1,d2);}float opSubtraction(float d1,float d2){return max(-d1,d2);}float opIntersection(float d1,float d2){return max(d1,d2);}" },
        { L"SDF operator", L"Inigo Quilez's iquilezles.org articles",
          L"smin / smax (polynomial smooth blend)",
          L"k-parameterized quadratic polynomial smooth-min/max: a smooth blend between two SDFs, shorter than a naive mix(...,clamp(...)) expansion.",
          "float smin(float a,float b,float k){float h=clamp(.5+.5*(b-a)/k,0.,1.);return mix(b,a,h)-k*h*(1.-h);}float smax(float a,float b,float k){return -smin(-a,-b,k);}" },
        { L"Hash/noise snippet", L"Inigo Quilez's iquilezles.org articles",
          L"hash11 (iq lineage)",
          L"1D to 1D hash, fract-multiply-self style -- a different lineage than Fabrice Neyret's fract-sin hash11 above.",
          "float hash11(float p){p=fract(p*.1031);p*=p+33.33;p*=p+p;return fract(p);}" },
        { L"Hash/noise snippet", L"Inigo Quilez's iquilezles.org articles",
          L"hash12 (iq lineage)",
          L"2D to 1D hash, fract-multiply-self style.",
          "float hash12(vec2 p){vec3 p3=fract(vec3(p.xyx)*.1031);p3+=dot(p3,p3.yzx+33.33);return fract((p3.x+p3.y)*p3.z);}" },
        { L"Hash/noise snippet", L"Inigo Quilez's iquilezles.org articles",
          L"hash33 (iq lineage)",
          L"3D to 3D hash, fract-multiply-self style -- complements Neyret's 3D-to-1D hash13.",
          "vec3 hash33(vec3 p3){p3=fract(p3*vec3(.1031,.1030,.0973));p3+=dot(p3,p3.yxz+33.33);return fract((p3.xxy+p3.yxx)*p3.zyx);}" },
        { L"Cosine palette", L"Inigo Quilez's iquilezles.org articles",
          L"palette() generator function",
          L"a+b*cos(2*pi*(c*t+d)): the reusable generator every preset below calls with a different coefficient set.",
          "vec3 palette(float t,vec3 a,vec3 b,vec3 c,vec3 d){return a+b*cos(6.28318*(c*t+d));}" },
        { L"Cosine palette", L"Inigo Quilez's iquilezles.org articles",
          L"rainbow preset",
          L"Even hue cycling through the full rainbow as t goes from 0 to 1.",
          "vec3 col=palette(t,vec3(.5,.5,.5),vec3(.5,.5,.5),vec3(1.,1.,1.),vec3(0.,.33,.67));" },
        { L"Cosine palette", L"Inigo Quilez's iquilezles.org articles",
          L"warm preset",
          L"Warm reds/oranges/yellows cycle.",
          "vec3 col=palette(t,vec3(.5,.5,.5),vec3(.5,.5,.5),vec3(1.,.7,.4),vec3(0.,.15,.2));" },
        { L"Cosine palette", L"Inigo Quilez's iquilezles.org articles",
          L"cool blue-green preset",
          L"Cool blue-to-green cycle.",
          "vec3 col=palette(t,vec3(.5,.5,.5),vec3(.5,.5,.5),vec3(1.,1.,.5),vec3(.8,.9,.3));" },
        { L"Cosine palette", L"Inigo Quilez's iquilezles.org articles",
          L"high-contrast red-cyan preset",
          L"High-contrast cycling between red and cyan.",
          "vec3 col=palette(t,vec3(.5,.5,.5),vec3(.5,.5,.5),vec3(2.,1.,0.),vec3(.5,.2,.25));" },
        { L"Tonemap / gamma", L"Inigo Quilez's iquilezles.org articles",
          L"gamma 2.2 correction",
          L"Short linear-to-sRGB-ish gamma correction, applied at the very end of mainImage.",
          "col=pow(col,vec3(1./2.2));" },
        { L"Tonemap / gamma", L"Inigo Quilez's iquilezles.org articles",
          L"ACES-approximation tonemap",
          L"Compact single-expression approximation of the ACES filmic tonemap curve.",
          "col=clamp((col*(2.51*col+.03))/(col*(2.43*col+.59)+.14),0.,1.);" },
    };

    constexpr int kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]);

    std::wstring to_lower(const std::wstring& s)
    {
        std::wstring out = s;
        std::transform(out.begin(), out.end(), out.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        return out;
    }

    bool matches_query(const GolfTipEntry& entry, const std::wstring& query_lower)
    {
        if (query_lower.empty())
        {
            return true;
        }
        std::wstring haystack = to_lower(entry.category) + L" " + to_lower(entry.source_catalogue) + L" "
            + to_lower(entry.title) + L" " + to_lower(entry.description);
        return haystack.find(query_lower) != std::wstring::npos;
    }
}

bool Win32GolfTipsPanel::create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory)
{
    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &text_format)))
    {
        return false;
    }

    if (FAILED(dwrite_factory->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ui_font_pt(13.0f), L"en-us", &header_format)))
    {
        return false;
    }

    return SUCCEEDED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush));
}

void Win32GolfTipsPanel::destroy()
{
    if (text_format != nullptr) { text_format->Release(); text_format = nullptr; }
    if (header_format != nullptr) { header_format->Release(); header_format = nullptr; }
    if (dynamic_brush != nullptr) { dynamic_brush->Release(); dynamic_brush = nullptr; }
}

void Win32GolfTipsPanel::layout(int x, int y, int width, int height)
{
    origin_x = x;
    origin_y = y;
    width_px = width;
    height_px = height;
}

bool Win32GolfTipsPanel::contains(int client_x, int client_y) const
{
    return client_x >= origin_x && client_x < origin_x + width_px
        && client_y >= origin_y && client_y < origin_y + height_px;
}

RECT Win32GolfTipsPanel::field_rect() const
{
    return RECT{ origin_x + 12, origin_y + 12, origin_x + width_px - 12, origin_y + 12 + static_cast<LONG>(kFieldHeight) };
}

RECT Win32GolfTipsPanel::list_rect() const
{
    RECT field = field_rect();
    return RECT{ origin_x, field.bottom + 12, origin_x + width_px, origin_y + height_px };
}

Win32GolfTipsPanel::VisibleEntries Win32GolfTipsPanel::filtered_entries() const
{
    VisibleEntries result;
    std::wstring query_lower = to_lower(utf8_to_wide(search_text));
    for (int i = 0; i < kEntryCount && result.count < 48; ++i)
    {
        if (matches_query(kEntries[i], query_lower))
        {
            result.indices[result.count] = i;
            result.count += 1;
        }
    }
    return result;
}

RECT Win32GolfTipsPanel::row_rect(int visible_index) const
{
    RECT list = list_rect();
    LONG top = list.top + (visible_index - scroll_top_row) * static_cast<LONG>(kRowHeight);
    return RECT{ list.left + 12, top, list.right - 12, top + static_cast<LONG>(kRowHeight) - 4 };
}

RECT Win32GolfTipsPanel::copy_button_rect(int visible_index) const
{
    RECT row = row_rect(visible_index);
    return RECT{ row.right - static_cast<LONG>(kCopyButtonWidth), row.top,
        row.right, row.top + static_cast<LONG>(kCopyButtonHeight) };
}

RECT Win32GolfTipsPanel::insert_button_rect(int visible_index) const
{
    RECT copy_rect = copy_button_rect(visible_index);
    return RECT{ copy_rect.left - 8 - static_cast<LONG>(kInsertButtonWidth), copy_rect.top,
        copy_rect.left - 8, copy_rect.bottom };
}

void Win32GolfTipsPanel::copy_entry_to_clipboard(int catalogue_index) const
{
    if (catalogue_index < 0 || catalogue_index >= kEntryCount)
    {
        return;
    }
    std::wstring text = utf8_to_wide(kEntries[catalogue_index].snippet);

    if (OpenClipboard(nullptr))
    {
        EmptyClipboard();
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
        if (mem != nullptr)
        {
            void* dest = GlobalLock(mem);
            if (dest != nullptr)
            {
                memcpy(dest, text.c_str(), (text.size() + 1) * sizeof(wchar_t));
                GlobalUnlock(mem);
                SetClipboardData(CF_UNICODETEXT, mem);
            }
        }
        CloseClipboard();
    }
}

bool Win32GolfTipsPanel::take_pending_snippet_insert(std::string& out_source)
{
    if (!has_pending_snippet_insert)
    {
        return false;
    }
    out_source = pending_snippet_insert_source;
    has_pending_snippet_insert = false;
    return true;
}

void Win32GolfTipsPanel::focus_search_with_query(const std::string& query)
{
    search_text = query;
    field_focused = true;
    scroll_top_row = 0;
}

void Win32GolfTipsPanel::set_field_focus(bool focused)
{
    field_focused = focused;
}

bool Win32GolfTipsPanel::on_mouse_down(int client_x, int client_y)
{
    POINT pt{ client_x, client_y };

    RECT field = field_rect();
    if (PtInRect(&field, pt))
    {
        field_focused = true;
        return true;
    }
    field_focused = false;

    VisibleEntries visible = filtered_entries();
    for (int i = scroll_top_row; i < visible.count; ++i)
    {
        RECT copy_rect = copy_button_rect(i);
        if (copy_rect.bottom > list_rect().top && PtInRect(&copy_rect, pt))
        {
            copy_entry_to_clipboard(visible.indices[i]);
            return true;
        }
        RECT insert_rect = insert_button_rect(i);
        if (insert_rect.bottom > list_rect().top && PtInRect(&insert_rect, pt))
        {
            int catalogue_index = visible.indices[i];
            if (catalogue_index >= 0 && catalogue_index < kEntryCount)
            {
                pending_snippet_insert_source = kEntries[catalogue_index].snippet;
                has_pending_snippet_insert = true;
            }
            return true;
        }
    }

    return contains(client_x, client_y);
}

void Win32GolfTipsPanel::on_mouse_move(int client_x, int client_y)
{
    POINT pt{ client_x, client_y };
    hovered_copy_row = -1;
    hovered_insert_row = -1;
    VisibleEntries visible = filtered_entries();
    for (int i = scroll_top_row; i < visible.count; ++i)
    {
        RECT copy_rect = copy_button_rect(i);
        if (PtInRect(&copy_rect, pt))
        {
            hovered_copy_row = i;
            break;
        }
        RECT insert_rect = insert_button_rect(i);
        if (PtInRect(&insert_rect, pt))
        {
            hovered_insert_row = i;
            break;
        }
    }
}

void Win32GolfTipsPanel::on_mouse_wheel(int wheel_delta)
{
    VisibleEntries visible = filtered_entries();
    int max_scroll = visible.count > 0 ? visible.count - 1 : 0;
    scroll_top_row -= wheel_delta / 40;
    scroll_top_row = std::max(0, std::min(scroll_top_row, max_scroll));
}

bool Win32GolfTipsPanel::on_char(wchar_t character)
{
    if (!field_focused)
    {
        return false;
    }
    if (character == L'\b' || character < 32)
    {
        return true;
    }
    search_text.push_back(character < 128 ? static_cast<char>(character) : '?');
    scroll_top_row = 0;
    return true;
}

bool Win32GolfTipsPanel::on_key_down(WPARAM key)
{
    if (!field_focused)
    {
        return false;
    }
    if (key == VK_BACK && !search_text.empty())
    {
        search_text.pop_back();
        scroll_top_row = 0;
        return true;
    }
    return field_focused;
}

void Win32GolfTipsPanel::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const
{
    D2D1_RECT_F bg_rect = D2D1::RectF(static_cast<float>(origin_x), static_cast<float>(origin_y),
        static_cast<float>(origin_x + width_px), static_cast<float>(origin_y + height_px));
    render_target->FillRectangle(bg_rect, brushes.bg_app);

    if (dynamic_brush == nullptr || text_format == nullptr || header_format == nullptr)
    {
        return;
    }

    RECT field = field_rect();
    D2D1_RECT_F field_bg = D2D1::RectF(static_cast<float>(field.left), static_cast<float>(field.top),
        static_cast<float>(field.right), static_cast<float>(field.bottom));
    render_target->FillRectangle(field_bg, brushes.bg_panel_raised);
    dynamic_brush->SetColor(field_focused
        ? D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z)
        : D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
    render_target->DrawRectangle(field_bg, dynamic_brush, 1.0f);

    std::wstring field_text = search_text.empty()
        ? std::wstring(L"Search golf tips (rotation, hash, SDF, palette, Neyret, Quilez...)")
        : utf8_to_wide(search_text);
    D2D1_RECT_F field_text_rect = D2D1::RectF(field_bg.left + 8.0f, field_bg.top, field_bg.right - 8.0f, field_bg.bottom);
    dynamic_brush->SetColor(search_text.empty()
        ? D2D1::ColorF(tokens::text_disabled.x, tokens::text_disabled.y, tokens::text_disabled.z)
        : D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
    render_target->DrawText(field_text.c_str(), static_cast<UINT32>(field_text.size()), text_format, field_text_rect, dynamic_brush);

    VisibleEntries visible = filtered_entries();
    RECT list = list_rect();

    D2D1_RECT_F clip_rect = D2D1::RectF(static_cast<float>(list.left), static_cast<float>(list.top),
        static_cast<float>(list.right), static_cast<float>(list.bottom));
    render_target->PushAxisAlignedClip(clip_rect, D2D1_ANTIALIAS_MODE_ALIASED);

    for (int i = scroll_top_row; i < visible.count; ++i)
    {
        const GolfTipEntry& entry = kEntries[visible.indices[i]];
        RECT row = row_rect(i);
        if (row.top > list.bottom)
        {
            break;
        }

        D2D1_RECT_F row_bg = D2D1::RectF(static_cast<float>(row.left), static_cast<float>(row.top),
            static_cast<float>(row.right), static_cast<float>(row.bottom));
        render_target->FillRectangle(row_bg, brushes.bg_panel_raised);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
        render_target->DrawRectangle(row_bg, dynamic_brush, 1.0f);

        std::wstring header_line = std::wstring(entry.title) + L"  —  " + entry.category + L" (" + entry.source_catalogue + L")";
        D2D1_RECT_F header_rect = D2D1::RectF(row_bg.left + 8.0f, row_bg.top + 4.0f,
            row_bg.right - kCopyButtonWidth - kInsertButtonWidth - 20.0f, row_bg.top + 22.0f);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
        render_target->DrawText(header_line.c_str(), static_cast<UINT32>(header_line.size()), header_format, header_rect, dynamic_brush);

        D2D1_RECT_F desc_rect = D2D1::RectF(row_bg.left + 8.0f, row_bg.top + 22.0f, row_bg.right - 8.0f, row_bg.bottom - 4.0f);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z));
        render_target->DrawText(entry.description, static_cast<UINT32>(wcslen(entry.description)), text_format, desc_rect, dynamic_brush);

        RECT copy_rect = copy_button_rect(i);
        D2D1_RECT_F copy_rect_f = D2D1::RectF(static_cast<float>(copy_rect.left), static_cast<float>(copy_rect.top),
            static_cast<float>(copy_rect.right), static_cast<float>(copy_rect.bottom));
        bool hovered = (hovered_copy_row == i);
        dynamic_brush->SetColor(hovered
            ? D2D1::ColorF(tokens::accent_hover.x, tokens::accent_hover.y, tokens::accent_hover.z)
            : D2D1::ColorF(tokens::accent.x, tokens::accent.y, tokens::accent.z));
        D2D1_ROUNDED_RECT copy_rounded = D2D1::RoundedRect(copy_rect_f, 3.0f, 3.0f);
        render_target->FillRoundedRectangle(copy_rounded, dynamic_brush);
        dynamic_brush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
        render_target->DrawText(L"Copy", 4, text_format, copy_rect_f, dynamic_brush);

        accessibility_register(("Copy snippet: " + std::string(wide_to_utf8(entry.title))).c_str(), AccessibleRole::Button,
            copy_rect_f.left, copy_rect_f.top, copy_rect_f.right - copy_rect_f.left, copy_rect_f.bottom - copy_rect_f.top, true);

        RECT insert_rect = insert_button_rect(i);
        D2D1_RECT_F insert_rect_f = D2D1::RectF(static_cast<float>(insert_rect.left), static_cast<float>(insert_rect.top),
            static_cast<float>(insert_rect.right), static_cast<float>(insert_rect.bottom));
        bool insert_hovered = (hovered_insert_row == i);
        dynamic_brush->SetColor(insert_hovered
            ? D2D1::ColorF(tokens::bg_panel_raised.x, tokens::bg_panel_raised.y, tokens::bg_panel_raised.z, 1.0f)
            : D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z, 1.0f));
        D2D1_ROUNDED_RECT insert_rounded = D2D1::RoundedRect(insert_rect_f, 3.0f, 3.0f);
        render_target->FillRoundedRectangle(insert_rounded, dynamic_brush);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::border_subtle.x, tokens::border_subtle.y, tokens::border_subtle.z));
        render_target->DrawRoundedRectangle(insert_rounded, dynamic_brush, 1.0f);
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_primary.x, tokens::text_primary.y, tokens::text_primary.z));
        render_target->DrawText(L"Insert", 6, text_format, insert_rect_f, dynamic_brush);

        accessibility_register(("Insert snippet: " + std::string(wide_to_utf8(entry.title))).c_str(), AccessibleRole::Button,
            insert_rect_f.left, insert_rect_f.top, insert_rect_f.right - insert_rect_f.left, insert_rect_f.bottom - insert_rect_f.top, true);
    }

    render_target->PopAxisAlignedClip();

    if (visible.count == 0)
    {
        D2D1_RECT_F empty_rect = D2D1::RectF(static_cast<float>(list.left + 12), static_cast<float>(list.top + 12),
            static_cast<float>(list.right - 12), static_cast<float>(list.top + 40));
        dynamic_brush->SetColor(D2D1::ColorF(tokens::text_secondary.x, tokens::text_secondary.y, tokens::text_secondary.z));
        render_target->DrawText(L"No matching golf tips.", 23, text_format, empty_rect, dynamic_brush);
    }

    const wchar_t* disclaimer = L"Manual reference only. Copy or Insert always requires your explicit click and changes shader output, not just its size -- nothing here is ever applied automatically.";
    D2D1_RECT_F disclaimer_rect = D2D1::RectF(static_cast<float>(origin_x + 12), static_cast<float>(origin_y + height_px - 34),
        static_cast<float>(origin_x + width_px - 12), static_cast<float>(origin_y + height_px - 8));
    dynamic_brush->SetColor(D2D1::ColorF(tokens::text_disabled.x, tokens::text_disabled.y, tokens::text_disabled.z));
    render_target->DrawText(disclaimer, static_cast<UINT32>(wcslen(disclaimer)), text_format, disclaimer_rect, dynamic_brush);
}
