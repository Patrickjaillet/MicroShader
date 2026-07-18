#include "shader_runner.h"

#include <algorithm>
#include <vector>

#include "gl_functions.h"

namespace
{
    const char* kVertexSource =
        "#version 330 core\n"
        "void main() {\n"
        "    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
        "    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n"
        "}\n";

    const char* kFragmentPrefix =
        "#version 330 core\n"
        "uniform float iTime;\n"
        "uniform vec3 iResolution;\n"
        "uniform vec4 iMouse;\n"
        "uniform vec4 iDate;\n"
        "uniform int iFrame;\n"
        "uniform float iFrameRate;\n"
        "out vec4 uShaderOutColor;\n";

    const char* kFragmentSuffix =
        "\nvoid main() {\n"
        "    mainImage(uShaderOutColor, gl_FragCoord.xy);\n"
        "}\n";

    bool compile_stage(GLenum stage, const std::string& source, GLuint& out_shader, std::string& error_log)
    {
        GLuint shader = glCreateShader(stage);
        const char* source_ptr = source.c_str();
        GLint source_len = static_cast<GLint>(source.size());
        glShaderSource(shader, 1, &source_ptr, &source_len);
        glCompileShader(shader);

        GLint status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint log_len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
            std::vector<char> log(static_cast<size_t>(log_len) + 1, '\0');
            glGetShaderInfoLog(shader, log_len, nullptr, log.data());
            error_log = log.data();
            glDeleteShader(shader);
            return false;
        }

        out_shader = shader;
        return true;
    }
}

int ShaderRunner::fragment_header_lines()
{
    static const int count = static_cast<int>(std::count(kFragmentPrefix, kFragmentPrefix + std::char_traits<char>::length(kFragmentPrefix), '\n'));
    return count;
}

bool ShaderRunner::compile(const std::string& fragment_source, std::string& error_log)
{
    GLuint vertex_shader = 0;
    if (!compile_stage(GL_VERTEX_SHADER, kVertexSource, vertex_shader, error_log))
    {
        return false;
    }

    std::string full_fragment_source = std::string(kFragmentPrefix) + fragment_source + kFragmentSuffix;
    GLuint fragment_shader = 0;
    if (!compile_stage(GL_FRAGMENT_SHADER, full_fragment_source, fragment_shader, error_log))
    {
        glDeleteShader(vertex_shader);
        return false;
    }

    GLuint new_program = glCreateProgram();
    glAttachShader(new_program, vertex_shader);
    glAttachShader(new_program, fragment_shader);
    glLinkProgram(new_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    GLint link_status = 0;
    glGetProgramiv(new_program, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE)
    {
        GLint log_len = 0;
        glGetProgramiv(new_program, GL_INFO_LOG_LENGTH, &log_len);
        std::vector<char> log(static_cast<size_t>(log_len) + 1, '\0');
        glGetProgramInfoLog(new_program, log_len, nullptr, log.data());
        error_log = log.data();
        glDeleteProgram(new_program);
        return false;
    }

    destroy();
    program = new_program;

    if (vao == 0)
    {
        glGenVertexArrays(1, &vao);
    }

    loc_time = glGetUniformLocation(program, "iTime");
    loc_resolution = glGetUniformLocation(program, "iResolution");
    loc_mouse = glGetUniformLocation(program, "iMouse");
    loc_date = glGetUniformLocation(program, "iDate");
    loc_frame = glGetUniformLocation(program, "iFrame");
    loc_frame_rate = glGetUniformLocation(program, "iFrameRate");

    return true;
}

void ShaderRunner::draw(const ShaderUniforms& uniforms) const
{
    if (program == 0)
    {
        return;
    }

    glUseProgram(program);
    glUniform1f(loc_time, uniforms.time);
    glUniform3f(loc_resolution, uniforms.resolution_x, uniforms.resolution_y, 1.0f);
    glUniform4f(loc_mouse, uniforms.mouse_x, uniforms.mouse_y, uniforms.mouse_click_x, uniforms.mouse_click_y);
    glUniform4f(loc_date, uniforms.date_year, uniforms.date_month, uniforms.date_day, uniforms.date_seconds);
    glUniform1i(loc_frame, uniforms.frame);
    glUniform1f(loc_frame_rate, uniforms.frame_rate);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void ShaderRunner::destroy()
{
    if (program != 0)
    {
        glDeleteProgram(program);
        program = 0;
    }
}
