#include "gl_functions.h"

#include <SDL3/SDL.h>

PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM2FPROC glUniform2f = nullptr;
PFNGLUNIFORM3FPROC glUniform3f = nullptr;
PFNGLUNIFORM4FPROC glUniform4f = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;

namespace
{
    template <typename T>
    bool load(T& fn, const char* name)
    {
        fn = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
        return fn != nullptr;
    }
}

bool gl_load_functions()
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
    return ok;
}
