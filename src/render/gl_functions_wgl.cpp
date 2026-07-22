#include "gl_functions.h"

#include <windows.h>
#include <GL/gl.h>

namespace
{
    template <typename T>
    bool load(T& fn, const char* name)
    {
        fn = reinterpret_cast<T>(wglGetProcAddress(name));
        if (fn == nullptr)
        {
            HMODULE opengl32 = GetModuleHandleA("opengl32.dll");
            if (opengl32 != nullptr)
            {
                fn = reinterpret_cast<T>(GetProcAddress(opengl32, name));
            }
        }
        return fn != nullptr;
    }
}

bool gl_load_functions_wgl()
{
    bool ok = true;
    ok = load(glCreateShader, "glCreateShader") && ok;
    ok = load(glShaderSource, "glShaderSource") && ok;
    ok = load(glCompileShader, "glCompileShader") && ok;
    ok = load(glGetShaderiv, "glGetShaderiv") && ok;
    ok = load(glGetShaderInfoLog, "glGetShaderInfoLog") && ok;
    ok = load(glDeleteShader, "glDeleteShader") && ok;
    ok = load(glCreateProgram, "glCreateProgram") && ok;
    ok = load(glAttachShader, "glAttachShader") && ok;
    ok = load(glLinkProgram, "glLinkProgram") && ok;
    ok = load(glGetProgramiv, "glGetProgramiv") && ok;
    ok = load(glGetProgramInfoLog, "glGetProgramInfoLog") && ok;
    ok = load(glDeleteProgram, "glDeleteProgram") && ok;
    ok = load(glUseProgram, "glUseProgram") && ok;
    ok = load(glGenVertexArrays, "glGenVertexArrays") && ok;
    ok = load(glBindVertexArray, "glBindVertexArray") && ok;
    ok = load(glDeleteVertexArrays, "glDeleteVertexArrays") && ok;
    ok = load(glGetUniformLocation, "glGetUniformLocation") && ok;
    ok = load(glUniform1f, "glUniform1f") && ok;
    ok = load(glUniform2f, "glUniform2f") && ok;
    ok = load(glUniform3f, "glUniform3f") && ok;
    ok = load(glUniform4f, "glUniform4f") && ok;
    ok = load(glUniform1i, "glUniform1i") && ok;
    ok = load(glGenFramebuffers, "glGenFramebuffers") && ok;
    ok = load(glBindFramebuffer, "glBindFramebuffer") && ok;
    ok = load(glFramebufferTexture2D, "glFramebufferTexture2D") && ok;
    ok = load(glCheckFramebufferStatus, "glCheckFramebufferStatus") && ok;
    ok = load(glDeleteFramebuffers, "glDeleteFramebuffers") && ok;
    ok = load(glBlitFramebuffer, "glBlitFramebuffer") && ok;
    return ok;
}
