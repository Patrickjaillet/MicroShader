#pragma once

#include <string>

struct ShaderUniforms
{
    float time;
    float resolution_x;
    float resolution_y;
    float mouse_x;
    float mouse_y;
    float mouse_click_x;
    float mouse_click_y;
    float date_year;
    float date_month;
    float date_day;
    float date_seconds;
    int frame;
    float frame_rate;
};

class ShaderRunner
{
public:
    bool compile(const std::string& fragment_source, std::string& error_log);
    void draw(const ShaderUniforms& uniforms) const;
    void destroy();

private:
    unsigned int program = 0;
    unsigned int vao = 0;
    int loc_time = -1;
    int loc_resolution = -1;
    int loc_mouse = -1;
    int loc_date = -1;
    int loc_frame = -1;
    int loc_frame_rate = -1;
};
