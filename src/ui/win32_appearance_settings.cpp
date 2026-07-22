#include "win32_appearance_settings.h"

float g_ui_font_size = kDefaultUiFontSize;
bool g_colorblind_safe_indicators = false;

float ui_font_pt(float base_pt)
{
    return base_pt * (g_ui_font_size / kDefaultUiFontSize);
}
