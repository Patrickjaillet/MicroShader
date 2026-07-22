#pragma once

#include <chrono>

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct ThemeBrushes;

enum class CompileStatus
{
    Idle,
    Compiling,
    Ok,
    Error,
};

class StatusDot
{
public:
    bool create(ID2D1RenderTarget* render_target);
    void destroy();

    void begin_compiling();
    void report_result(bool ok);
    void tick();
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes, float center_x, float center_y) const;

    static constexpr float kRadius = 4.0f;

private:
    ID2D1SolidColorBrush* dynamic_brush = nullptr;
    CompileStatus displayed_status = CompileStatus::Idle;
    std::chrono::steady_clock::time_point status_since;

    bool has_pending = false;
    CompileStatus pending_status = CompileStatus::Idle;
    std::chrono::steady_clock::time_point pending_since;
};
