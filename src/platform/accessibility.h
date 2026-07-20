#pragma once

struct SDL_Window;

enum class AccessibleRole
{
    Button,
    CheckBox,
};

void accessibility_init(SDL_Window* window);
void accessibility_shutdown();

void accessibility_begin_frame();
void accessibility_register(const char* name, AccessibleRole role, float client_x, float client_y, float width, float height, bool enabled);
void accessibility_register_toggle(const char* name, AccessibleRole role, float client_x, float client_y, float width, float height, bool enabled, bool toggled);
void accessibility_end_frame();
