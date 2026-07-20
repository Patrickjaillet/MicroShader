#pragma once

#include <string>

#include <imgui.h>

struct ImGuiIO;
struct ImFont;

extern ImFont* g_icon_font;
extern ImFont* g_mono_font;

void apply_theme();
void load_fonts(ImGuiIO& io, float base_size);

constexpr float kDefaultBaseFontSize = 18.0f;
constexpr float kMinBaseFontSize = 13.0f;
constexpr float kMaxBaseFontSize = 28.0f;

void apply_dpi_scale(ImGuiIO& io, float scale);
bool themed_checkbox(const char* label, bool* value);

enum class StatusKind
{
    Ok,
    Warning,
    Error,
};

extern bool g_colorblind_safe_indicators;

void render_status_dot(const char* label, StatusKind kind);
