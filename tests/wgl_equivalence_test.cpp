#include "../src/render/wgl_viewport_host.h"
#include "../src/render/gl_functions.h"
#include "../src/render/shader_runner.h"
#include "../src/render/framebuffer.h"
#include "../src/render/default_shader.h"

#include <windows.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    const wchar_t* kHiddenClassName = L"uShaderWglEquivalenceTestHost";

    LRESULT CALLBACK hidden_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    std::string read_text_file(const char* path)
    {
        std::ifstream file(path, std::ios::binary);
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    const char* kTrivialVertexSource =
        "#version 330 core\n"
        "void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }\n";

    // Phase 34.7: compiles+links a complete, standalone fragment shader (not the
    // Shadertoy mainImage() convention ShaderRunner::compile() expects) against a
    // trivial vertex shader, to verify a Twigl-mode export fixture is valid GLSL
    // once pasted into twigl.app (or an equivalent GLSL ES 1.00-style host).
    // Returns true and leaves error_log empty on success.
    bool compile_and_link_standalone_fragment(const std::string& fragment_source, std::string& error_log)
    {
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        {
            const char* source_ptr = kTrivialVertexSource;
            GLint source_len = static_cast<GLint>(std::char_traits<char>::length(kTrivialVertexSource));
            glShaderSource(vertex_shader, 1, &source_ptr, &source_len);
            glCompileShader(vertex_shader);
        }

        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        {
            const char* source_ptr = fragment_source.c_str();
            GLint source_len = static_cast<GLint>(fragment_source.size());
            glShaderSource(fragment_shader, 1, &source_ptr, &source_len);
            glCompileShader(fragment_shader);
        }

        GLint frag_status = 0;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &frag_status);
        if (frag_status == GL_FALSE)
        {
            GLint log_len = 0;
            glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_len);
            std::string log(static_cast<size_t>(log_len) + 1, '\0');
            glGetShaderInfoLog(fragment_shader, log_len, nullptr, &log[0]);
            error_log = log;
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            return false;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        GLint link_status = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &link_status);
        if (link_status == GL_FALSE)
        {
            GLint log_len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
            std::string log(static_cast<size_t>(log_len) + 1, '\0');
            glGetProgramInfoLog(program, log_len, nullptr, &log[0]);
            error_log = log;
            return false;
        }
        return true;
    }

    // Reconstructs the `precision`/`uniform`/`void main(){}` scaffold that
    // twigl.app's own exporter auto-complements around a Geeker/Geekest-mode
    // export (see ROADMAP.md Phase 34.1), so the fixture's bare expression body
    // can be verified as valid GLSL once that scaffold is present, exactly as it
    // would be when pasted into twigl.app itself.
    std::string complement_geekest_fixture(const std::string& bare_body)
    {
        return
            "precision mediump float;\n"
            "uniform vec2 r;\n"
            "uniform float t;\n"
            "uniform vec2 m;\n"
            "uniform float f;\n"
            "uniform sampler2D b;\n"
            "#define FC gl_FragCoord\n"
            "void main(){" + bare_body + "}\n";
    }
}

int main()
{
    HINSTANCE instance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = hidden_wndproc;
    wc.hInstance = instance;
    wc.lpszClassName = kHiddenClassName;
    RegisterClassExW(&wc);

    HWND hidden_hwnd = CreateWindowExW(
        0, kHiddenClassName, L"", WS_POPUP,
        0, 0, 640, 360, nullptr, nullptr, instance, nullptr);
    if (hidden_hwnd == nullptr)
    {
        std::fprintf(stderr, "could not create hidden host window\n");
        return 1;
    }

    WglViewportHost viewport;
    if (!viewport.create(hidden_hwnd, 0, 0, 640, 360))
    {
        std::fprintf(stderr, "could not create WGL viewport host\n");
        return 1;
    }
    viewport.make_current();

    if (!gl_load_functions_wgl())
    {
        std::fprintf(stderr, "could not load OpenGL 3.3 core functions under WGL\n");
        return 1;
    }

    ShaderRunner source_runner;
    ShaderRunner golfed_runner;
    std::string compile_error;
    if (!source_runner.compile(kDefaultShaderSource, compile_error))
    {
        std::fprintf(stderr, "source shader compile failed: %s\n", compile_error.c_str());
        return 1;
    }
    if (!golfed_runner.compile(kDefaultShaderSource, compile_error))
    {
        std::fprintf(stderr, "golfed shader compile failed: %s\n", compile_error.c_str());
        return 1;
    }

    OffscreenFramebuffer source_fb;
    OffscreenFramebuffer golfed_fb;

    EquivalenceSampleConfig config;
    EquivalenceRunResult result = run_equivalence_check(
        source_runner, golfed_runner, source_fb, golfed_fb, config, 640, 360);

    int failures = 0;

    // Phase 34.7: verify the Twigl-mode export fixtures are valid, standalone-
    // compilable GLSL under this same desktop OpenGL 3.3 core WGL context.
    // `twigl_300es.glsl` is deliberately excluded: it requires `#version 300 es`,
    // which this desktop-core context cannot accept without an ES-to-desktop
    // translation shim (e.g. ANGLE) that does not exist in this codebase.
    {
        std::string classic_source = read_text_file("fixtures/twigl_classic.glsl");
        std::string classic_error;
        if (classic_source.empty())
        {
            std::fprintf(stderr, "could not read fixtures/twigl_classic.glsl\n");
            failures += 1;
        }
        else if (!compile_and_link_standalone_fragment(classic_source, classic_error))
        {
            std::fprintf(stderr, "twigl_classic.glsl fixture failed to compile/link:\n%s\n", classic_error.c_str());
            failures += 1;
        }
        else
        {
            std::printf("twigl_classic.glsl fixture compiles and links as standalone GLSL under WGL hosting\n");
        }

        std::string geekest_body = read_text_file("fixtures/twigl_geekest.glsl");
        std::string geekest_error;
        if (geekest_body.empty())
        {
            std::fprintf(stderr, "could not read fixtures/twigl_geekest.glsl\n");
            failures += 1;
        }
        else if (!compile_and_link_standalone_fragment(complement_geekest_fixture(geekest_body), geekest_error))
        {
            std::fprintf(stderr, "twigl_geekest.glsl fixture failed to compile/link once auto-complemented:\n%s\n", geekest_error.c_str());
            failures += 1;
        }
        else
        {
            std::printf("twigl_geekest.glsl fixture compiles and links once auto-complemented (as twigl.app itself would do)\n");
        }
    }

    // golf.md Phase 29.2: `.xyzw`/`.rgba`/`.stpq` are three interchangeable
    // GLSL swizzle-letter alphabets. `fixtures/swizzle_alphabet.glsl` is
    // authored in `.xyzw`; `kSwizzleAlphabetRgbaSource` below is the exact,
    // hand-verified `.rgba` recoloring `apply_swizzle_alphabet` produces for
    // that same fixture (every `.xyzw`-style token replaced position-for-
    // position: x->r, y->g, z->b, w->a, nothing else touched). Unlike the
    // Twigl block above, this must prove pixel-identical rendering, not just
    // compile/link, since the whole point of Phase 29.2 is that recoloring
    // never changes shader output — so it goes through the same
    // `run_equivalence_check` pixel-sampling path used for `source_runner`/
    // `golfed_runner` above rather than `compile_and_link_standalone_fragment`.
    {
        const char* kSwizzleAlphabetRgbaSource =
            "void mainImage(out vec4 fragColor, in vec2 fragCoord)\n"
            "{\n"
            "    vec2 uv = fragCoord.rg / iResolution.rg;\n"
            "    vec3 col = vec3(uv.r, uv.g, uv.r + uv.g);\n"
            "    vec3 shifted = col.rgb + vec3(sin(iTime), cos(iTime), 0.0);\n"
            "    vec2 swirl = shifted.rg - shifted.gr * 0.5;\n"
            "    fragColor = vec4(swirl.rg, shifted.b, 1.0);\n"
            "}\n";

        std::string xyzw_source = read_text_file("fixtures/swizzle_alphabet.glsl");
        if (xyzw_source.empty())
        {
            std::fprintf(stderr, "could not read fixtures/swizzle_alphabet.glsl\n");
            failures += 1;
        }
        else
        {
            ShaderRunner xyzw_runner;
            ShaderRunner rgba_runner;
            std::string xyzw_error;
            std::string rgba_error;
            if (!xyzw_runner.compile(xyzw_source, xyzw_error))
            {
                std::fprintf(stderr, "swizzle_alphabet.glsl (.xyzw) failed to compile: %s\n", xyzw_error.c_str());
                failures += 1;
            }
            else if (!rgba_runner.compile(kSwizzleAlphabetRgbaSource, rgba_error))
            {
                std::fprintf(stderr, "swizzle_alphabet.glsl (.rgba recoloring) failed to compile: %s\n", rgba_error.c_str());
                failures += 1;
            }
            else
            {
                OffscreenFramebuffer xyzw_fb;
                OffscreenFramebuffer rgba_fb;
                EquivalenceSampleConfig swizzle_config;
                EquivalenceRunResult swizzle_result = run_equivalence_check(
                    xyzw_runner, rgba_runner, xyzw_fb, rgba_fb, swizzle_config, 640, 360);
                xyzw_fb.destroy();
                rgba_fb.destroy();

                if (!swizzle_result.valid)
                {
                    std::fprintf(stderr, "swizzle_alphabet.glsl equivalence run did not complete\n");
                    failures += 1;
                }
                else if (swizzle_result.samples_failed != 0)
                {
                    std::fprintf(stderr, "swizzle_alphabet.glsl: %d/%d samples differ between .xyzw and .rgba, max delta %d\n",
                        swizzle_result.samples_failed, swizzle_result.samples_total, swizzle_result.worst_max_delta);
                    failures += 1;
                }
                else
                {
                    std::printf("swizzle_alphabet.glsl: %d/%d samples bit-exact between .xyzw and .rgba recoloring\n",
                        swizzle_result.samples_total, swizzle_result.samples_total);
                }
            }
            xyzw_runner.destroy();
            rgba_runner.destroy();
        }
    }

    // golf.md Phase 31.1: folding a loop body's standalone counter
    // increment into the `for(...)` header must never change rendered
    // output. `kLoopHeaderGolfGolfedSource` below is the exact,
    // hand-verified output `golf_loop_headers` produces for
    // `fixtures/loop_header_golf.glsl` with only `loop_header_golf`
    // enabled (both the raymarching-loop and the fractal-iteration-loop
    // idiom named in the fixture's own delivery bar), so this proves
    // pixel-identical rendering the same way the Phase 29.2 swizzle
    // block above does, rather than only a compile/link check.
    {
        const char* kLoopHeaderGolfGolfedSource =
            "void mainImage(out vec4 h,in vec2 i){vec2 d=(i-.5*iResolution.xy)/iResolution.y;"
            "vec3 j=vec3(0.,0.,-3.);vec3 k=normalize(vec3(d,1.));float b=0.;vec3 e=vec3(0.);"
            "for(float l=0.;l++<80.;){vec3 m=j+k*b;float c=length(m)-1.;e+=vec3(.01/max(c,.001));"
            "if(c<.001){break;}b+=c;}vec2 a=d*2.;vec2 n=vec2(-.7,.27015);vec3 g=vec3(0.);"
            "for(float o=0.;o<64.;o++){a=vec2(a.x*a.x-a.y*a.y,2.*a.x*a.y)+n;g+=vec3(.01,.02,.03);"
            "if(dot(a,a)>4.){break;}}vec3 p=e*.1+g+vec3(b*.05,a.x*.1,a.y*.1);h=vec4(p,1.);}\n";

        std::string loop_header_source = read_text_file("fixtures/loop_header_golf.glsl");
        if (loop_header_source.empty())
        {
            std::fprintf(stderr, "could not read fixtures/loop_header_golf.glsl\n");
            failures += 1;
        }
        else
        {
            ShaderRunner ungolfed_runner;
            ShaderRunner loop_golfed_runner;
            std::string ungolfed_error;
            std::string loop_golfed_error;
            if (!ungolfed_runner.compile(loop_header_source, ungolfed_error))
            {
                std::fprintf(stderr, "loop_header_golf.glsl (ungolfed) failed to compile: %s\n", ungolfed_error.c_str());
                failures += 1;
            }
            else if (!loop_golfed_runner.compile(kLoopHeaderGolfGolfedSource, loop_golfed_error))
            {
                std::fprintf(stderr, "loop_header_golf.glsl (header-golfed) failed to compile: %s\n", loop_golfed_error.c_str());
                failures += 1;
            }
            else
            {
                OffscreenFramebuffer ungolfed_fb;
                OffscreenFramebuffer loop_golfed_fb;
                EquivalenceSampleConfig loop_header_config;
                EquivalenceRunResult loop_header_result = run_equivalence_check(
                    ungolfed_runner, loop_golfed_runner, ungolfed_fb, loop_golfed_fb, loop_header_config, 640, 360);
                ungolfed_fb.destroy();
                loop_golfed_fb.destroy();

                if (!loop_header_result.valid)
                {
                    std::fprintf(stderr, "loop_header_golf.glsl equivalence run did not complete\n");
                    failures += 1;
                }
                else if (loop_header_result.samples_failed != 0)
                {
                    std::fprintf(stderr, "loop_header_golf.glsl: %d/%d samples differ between ungolfed and header-golfed loops, max delta %d\n",
                        loop_header_result.samples_failed, loop_header_result.samples_total, loop_header_result.worst_max_delta);
                    failures += 1;
                }
                else
                {
                    std::printf("loop_header_golf.glsl: %d/%d samples bit-exact between ungolfed and header-golfed loops\n",
                        loop_header_result.samples_total, loop_header_result.samples_total);
                }
            }
            ungolfed_runner.destroy();
            loop_golfed_runner.destroy();
        }
    }

    source_fb.destroy();
    golfed_fb.destroy();
    source_runner.destroy();
    golfed_runner.destroy();
    viewport.destroy();

    if (!result.valid)
    {
        std::fprintf(stderr, "equivalence run did not complete under the WGL-hosted context\n");
        failures += 1;
    }
    else if (result.samples_failed != 0)
    {
        std::fprintf(stderr, "%d/%d samples differ under WGL hosting, max delta %d\n",
            result.samples_failed, result.samples_total, result.worst_max_delta);
        failures += 1;
    }
    else
    {
        std::printf("%d/%d samples bit-exact under the WGL-hosted context, matching the Phase 15 SDL-hosted safety net\n",
            result.samples_total, result.samples_total);
    }

    return failures;
}
