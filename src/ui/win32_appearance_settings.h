#pragma once

constexpr float kDefaultUiFontSize = 18.0f;
constexpr float kMinUiFontSize = 13.0f;
constexpr float kMaxUiFontSize = 28.0f;

extern float g_ui_font_size;
extern bool g_colorblind_safe_indicators;

float ui_font_pt(float base_pt);
