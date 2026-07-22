#include "win32_status_dot.h"

#include <d2d1.h>
#include <cmath>

#include "win32_theme_brushes.h"
#include "win32_appearance_settings.h"
#include "theme_tokens.h"

namespace
{
    constexpr float kMinBusyVisibleSeconds = 0.35f;
    constexpr float kPulseHz = 2.4f;
    constexpr float kShakeDurationSeconds = 0.4f;
    constexpr float kShakeAmplitude = 3.0f;
    constexpr float kShakeHz = 10.0f;
    constexpr float kSuccessPulseDurationSeconds = 0.3f;
    constexpr float kSuccessPulseGrowth = 6.0f;
    constexpr float kPi = 3.14159265358979323846f;
}

bool StatusDot::create(ID2D1RenderTarget* render_target)
{
    if (FAILED(render_target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0), &dynamic_brush)))
    {
        return false;
    }
    status_since = std::chrono::steady_clock::now();
    return true;
}

void StatusDot::destroy()
{
    if (dynamic_brush != nullptr)
    {
        dynamic_brush->Release();
        dynamic_brush = nullptr;
    }
}

void StatusDot::begin_compiling()
{
    displayed_status = CompileStatus::Compiling;
    status_since = std::chrono::steady_clock::now();
    has_pending = false;
}

void StatusDot::report_result(bool ok)
{
    pending_status = ok ? CompileStatus::Ok : CompileStatus::Error;
    pending_since = std::chrono::steady_clock::now();
    has_pending = true;
}

void StatusDot::tick()
{
    if (!has_pending)
    {
        return;
    }

    float since_busy_started = std::chrono::duration<float>(std::chrono::steady_clock::now() - status_since).count();
    if (since_busy_started >= kMinBusyVisibleSeconds)
    {
        displayed_status = pending_status;
        status_since = std::chrono::steady_clock::now();
        has_pending = false;
    }
}

void StatusDot::paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes, float center_x, float center_y) const
{
    if (dynamic_brush == nullptr)
    {
        return;
    }

    float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - status_since).count();

    float red = tokens::text_secondary.x;
    float green = tokens::text_secondary.y;
    float blue = tokens::text_secondary.z;
    float alpha = 0.6f;
    float offset_x = 0.0f;

    switch (displayed_status)
    {
    case CompileStatus::Idle:
        break;
    case CompileStatus::Compiling:
        red = tokens::accent.x;
        green = tokens::accent.y;
        blue = tokens::accent.z;
        alpha = 0.4f + 0.4f * (0.5f + 0.5f * std::sin(elapsed * kPulseHz * 2.0f * kPi));
        break;
    case CompileStatus::Ok:
        red = tokens::status_ok.x;
        green = tokens::status_ok.y;
        blue = tokens::status_ok.z;
        alpha = 1.0f;
        if (elapsed < kSuccessPulseDurationSeconds)
        {
            float t = elapsed / kSuccessPulseDurationSeconds;
            float pulse_radius = kRadius + t * kSuccessPulseGrowth;
            float pulse_alpha = (1.0f - t) * 0.7f;
            dynamic_brush->SetColor(D2D1::ColorF(red, green, blue, pulse_alpha));
            D2D1_ELLIPSE pulse_ellipse = D2D1::Ellipse(D2D1::Point2F(center_x, center_y), pulse_radius, pulse_radius);
            render_target->DrawEllipse(pulse_ellipse, dynamic_brush, 1.5f);
        }
        break;
    case CompileStatus::Error:
        red = tokens::status_error.x;
        green = tokens::status_error.y;
        blue = tokens::status_error.z;
        alpha = 1.0f;
        if (elapsed < kShakeDurationSeconds)
        {
            float decay = 1.0f - (elapsed / kShakeDurationSeconds);
            offset_x = kShakeAmplitude * std::sin(elapsed * kShakeHz * 2.0f * kPi) * decay;
        }
        break;
    }

    dynamic_brush->SetColor(D2D1::ColorF(red, green, blue, alpha));

    if (g_colorblind_safe_indicators && displayed_status == CompileStatus::Error)
    {
        D2D1_RECT_F square = D2D1::RectF(center_x + offset_x - kRadius, center_y - kRadius,
            center_x + offset_x + kRadius, center_y + kRadius);
        render_target->FillRectangle(square, dynamic_brush);
        return;
    }

    D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(center_x + offset_x, center_y), kRadius, kRadius);
    render_target->FillEllipse(ellipse, dynamic_brush);
}
