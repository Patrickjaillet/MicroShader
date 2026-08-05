#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TwiglMode {
    Classic,
    Geek,
    Geeker,
    Geekest,
}

pub struct TwiglSnippet {
    pub name: &'static str,
    pub source: &'static str,
}

// Reimplemented from scratch against twigl's documented geekest-mode snippet
// set, not copied from twigl.app or its repository.
const TWIGL_SNIPPETS: &[TwiglSnippet] = &[
    TwiglSnippet { name: "PI", source: "const float PI=3.14159265359;" },
    TwiglSnippet { name: "PI2", source: "const float PI2=6.28318530718;" },
    TwiglSnippet {
        name: "hsv",
        source: "vec3 hsv(vec3 c){vec4 K=vec4(1.,2./3.,1./3.,3.);vec3 p=abs(fract(c.xxx+K.xyz)*6.-K.www);return c.z*mix(K.xxx,clamp(p-K.xxx,0.,1.),c.y);}",
    },
    TwiglSnippet {
        name: "rotate2D",
        source: "mat2 rotate2D(float r){return mat2(cos(r),-sin(r),sin(r),cos(r));}",
    },
    TwiglSnippet {
        name: "rotate3D",
        source: "mat3 rotate3D(float angle,vec3 axis){vec3 a=normalize(axis);float s=sin(angle),c=cos(angle),oc=1.-c;return mat3(oc*a.x*a.x+c,oc*a.x*a.y-a.z*s,oc*a.z*a.x+a.y*s,oc*a.x*a.y+a.z*s,oc*a.y*a.y+c,oc*a.y*a.z-a.x*s,oc*a.z*a.x-a.y*s,oc*a.y*a.z+a.x*s,oc*a.z*a.z+c);}",
    },
    TwiglSnippet {
        name: "fsnoise",
        source: "float fsnoise(vec2 c){return fract(sin(dot(c,vec2(12.9898,78.233)))*43758.5453);}",
    },
    TwiglSnippet {
        name: "fsnoiseDigits",
        source: "float fsnoiseDigits(vec2 c){return fract(sin(dot(c,vec2(12.9898,78.233)))*43758.5453123);}",
    },
    // A self-contained hash-lattice value-noise reimplementation (smoothstep-faded
    // bilinear/trilinear/quadrilinear interpolation of a fract(sin(dot(...))) hash),
    // deliberately distinct in structure from the permutation-polynomial simplex
    // noise algorithm popularized by Ashima Arts/webgl-noise, to avoid reproducing
    // that well-known implementation while still exposing the documented snoise*D
    // names and GLSL call signature.
    TwiglSnippet {
        name: "snoise2D",
        source: "float snoise2D(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);float a=fract(sin(dot(i,vec2(12.9898,78.233)))*43758.5453),b=fract(sin(dot(i+vec2(1.,0.),vec2(12.9898,78.233)))*43758.5453),c=fract(sin(dot(i+vec2(0.,1.),vec2(12.9898,78.233)))*43758.5453),d=fract(sin(dot(i+vec2(1.,1.),vec2(12.9898,78.233)))*43758.5453);return mix(mix(a,b,f.x),mix(c,d,f.x),f.y);}",
    },
    TwiglSnippet {
        name: "snoise3D",
        source: "float snoise3D(vec3 p){vec3 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);vec3 k=vec3(12.9898,78.233,37.719);float n000=fract(sin(dot(i,k))*43758.5453),n100=fract(sin(dot(i+vec3(1.,0.,0.),k))*43758.5453),n010=fract(sin(dot(i+vec3(0.,1.,0.),k))*43758.5453),n110=fract(sin(dot(i+vec3(1.,1.,0.),k))*43758.5453),n001=fract(sin(dot(i+vec3(0.,0.,1.),k))*43758.5453),n101=fract(sin(dot(i+vec3(1.,0.,1.),k))*43758.5453),n011=fract(sin(dot(i+vec3(0.,1.,1.),k))*43758.5453),n111=fract(sin(dot(i+vec3(1.,1.,1.),k))*43758.5453);return mix(mix(mix(n000,n100,f.x),mix(n010,n110,f.x),f.y),mix(mix(n001,n101,f.x),mix(n011,n111,f.x),f.y),f.z);}",
    },
    TwiglSnippet {
        name: "snoise4D",
        source: "float snoise4D(vec4 p){vec4 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);vec4 k=vec4(12.9898,78.233,37.719,19.417);float n0000=fract(sin(dot(i,k))*43758.5453),n1000=fract(sin(dot(i+vec4(1.,0.,0.,0.),k))*43758.5453),n0100=fract(sin(dot(i+vec4(0.,1.,0.,0.),k))*43758.5453),n1100=fract(sin(dot(i+vec4(1.,1.,0.,0.),k))*43758.5453),n0010=fract(sin(dot(i+vec4(0.,0.,1.,0.),k))*43758.5453),n1010=fract(sin(dot(i+vec4(1.,0.,1.,0.),k))*43758.5453),n0110=fract(sin(dot(i+vec4(0.,1.,1.,0.),k))*43758.5453),n1110=fract(sin(dot(i+vec4(1.,1.,1.,0.),k))*43758.5453),n0001=fract(sin(dot(i+vec4(0.,0.,0.,1.),k))*43758.5453),n1001=fract(sin(dot(i+vec4(1.,0.,0.,1.),k))*43758.5453),n0101=fract(sin(dot(i+vec4(0.,1.,0.,1.),k))*43758.5453),n1101=fract(sin(dot(i+vec4(1.,1.,0.,1.),k))*43758.5453),n0011=fract(sin(dot(i+vec4(0.,0.,1.,1.),k))*43758.5453),n1011=fract(sin(dot(i+vec4(1.,0.,1.,1.),k))*43758.5453),n0111=fract(sin(dot(i+vec4(0.,1.,1.,1.),k))*43758.5453),n1111=fract(sin(dot(i+vec4(1.,1.,1.,1.),k))*43758.5453);float x00=mix(n0000,n1000,f.x),x10=mix(n0100,n1100,f.x),x01=mix(n0010,n1010,f.x),x11=mix(n0110,n1110,f.x),y00=mix(n0001,n1001,f.x),y10=mix(n0101,n1101,f.x),y01=mix(n0011,n1011,f.x),y11=mix(n0111,n1111,f.x),z0=mix(mix(x00,x10,f.y),mix(x01,x11,f.y),f.z),z1=mix(mix(y00,y10,f.y),mix(y01,y11,f.y),f.z);return mix(z0,z1,f.w);}",
    },
];

pub fn twigl_snippets() -> &'static [TwiglSnippet] {
    TWIGL_SNIPPETS
}

pub fn twigl_snippet(name: &str) -> Option<&'static str> {
    TWIGL_SNIPPETS.iter().find(|s| s.name == name).map(|s| s.source)
}

// Shared by replace_identifier and identifier_present below: true if the
// `[start, end)` byte range of `text` is not immediately preceded/followed
// by an identifier character, i.e. it's a genuine standalone identifier
// occurrence rather than a substring of a longer one (e.g. `iTime` inside
// `iTimeScale`).
fn is_identifier_boundary_match(text: &str, start: usize, end: usize) -> bool {
    let before_ok = text[..start].chars().last().map_or(true, |c| !(c.is_alphanumeric() || c == '_'));
    let after_ok = text[end..].chars().next().map_or(true, |c| !(c.is_alphanumeric() || c == '_'));
    before_ok && after_ok
}

// Avoids corrupting identifiers that merely contain `from` as a substring (e.g. `iTimeScale`).
fn replace_identifier(input: &str, from: &str, to: &str) -> String {
    let mut result = String::with_capacity(input.len());
    let mut last_end = 0;
    for (start, _) in input.match_indices(from) {
        if start < last_end {
            continue;
        }
        let end = start + from.len();
        if is_identifier_boundary_match(input, start, end) {
            result.push_str(&input[last_end..start]);
            result.push_str(to);
            last_end = end;
        }
    }
    result.push_str(&input[last_end..]);
    result
}

pub fn rewrite_twigl_uniforms(input: &str, mode: TwiglMode) -> String {
    let mut output = input.to_string();
    let replacements: &[(&str, &str)] = match mode {
        TwiglMode::Classic => &[
            ("iResolution", "resolution"),
            ("iMouse", "mouse"),
            ("iTime", "time"),
            ("iFrame", "frame"),
            ("iChannel0", "backbuffer"),
        ],
        TwiglMode::Geek | TwiglMode::Geeker | TwiglMode::Geekest => &[
            ("iResolution", "r"),
            ("iMouse", "m"),
            ("iTime", "t"),
            ("iFrame", "f"),
            ("iChannel0", "b"),
        ],
    };

    for (from, to) in replacements {
        output = replace_identifier(&output, from, to);
    }

    if matches!(mode, TwiglMode::Geekest) {
        output = replace_identifier(&output, "gl_FragCoord", "FC");
    }

    output
}

fn strip_precision_and_uniform_declarations(input: &str) -> String {
    input
        .lines()
        .filter_map(|line| {
            let trimmed = line.trim();
            if trimmed.is_empty() || trimmed.starts_with("precision") || trimmed.starts_with("uniform") {
                None
            } else {
                Some(format!("{line}\n"))
            }
        })
        .collect::<Vec<_>>()
        .concat()
        .trim_end()
        .to_string()
}

// Real Shadertoy code -- and every shader this app's own Source tab actually
// compiles, including its own default shader (src/render/default_shader.h)
// -- is ALWAYS written as `void mainImage(out vec4 X, in vec2 Y){...}`,
// never as a literal `void main(){}` using `gl_FragColor`/`gl_FragCoord`
// directly (ShaderRunner, src/render/shader_runner.cpp, wraps user source
// with its own synthesized `void main(){ mainImage(uShaderOutColor,
// gl_FragCoord.xy); }` that calls into the user's mainImage). Every other
// function in this module operates on the literal `gl_FragColor`/
// `gl_FragCoord` builtins, so this normalization -- unwrapping mainImage
// into a plain `void main(){}` with its out/in parameters renamed to those
// builtins -- must run first, before any of that logic, or a genuine
// Shadertoy-style paste is left completely untouched by every later pass
// (the exact bug this fixes: mainImage's parameter names, e.g. golfed to
// single letters, are indistinguishable from ordinary user code to every
// pass that only knows about gl_FragColor/gl_FragCoord).
//
// A no-op (returns input unchanged) whenever no `mainImage` function
// definition is found, so already plain-`main`-style input (e.g. a second
// call in the same pipeline, or hand-written twigl-shorthand code) is left
// alone.
fn normalize_mainimage_to_plain_main(input: &str) -> String {
    let Some(name_pos) = input.find("mainImage") else {
        return input.to_string();
    };
    let after_name = &input[name_pos + "mainImage".len()..];
    let Some(paren_rel) = after_name.find('(') else {
        return input.to_string();
    };
    // Only whitespace may separate the identifier from '(' for this to be a
    // function definition/call rather than an unrelated identifier that
    // merely starts with "mainImage" (e.g. a hypothetical "mainImageBuffer").
    if !after_name[..paren_rel].chars().all(char::is_whitespace) {
        return input.to_string();
    }

    let bytes = input.as_bytes();
    let params_start = name_pos + "mainImage".len() + paren_rel + 1;
    let mut depth = 1i32;
    let mut idx = params_start;
    while idx < bytes.len() && depth > 0 {
        match bytes[idx] {
            b'(' => depth += 1,
            b')' => depth -= 1,
            _ => {}
        }
        idx += 1;
    }
    if depth != 0 {
        return input.to_string();
    }
    let params_end = idx - 1;
    let params = &input[params_start..params_end];

    let parts: Vec<&str> = params.split(',').collect();
    if parts.len() != 2 {
        return input.to_string();
    }
    let Some(out_name) = last_identifier_token(parts[0]) else {
        return input.to_string();
    };
    let Some(coord_name) = last_identifier_token(parts[1]) else {
        return input.to_string();
    };

    let mut brace_search = params_end + 1;
    while brace_search < bytes.len() && bytes[brace_search] != b'{' {
        if !(bytes[brace_search] as char).is_whitespace() {
            return input.to_string();
        }
        brace_search += 1;
    }
    if brace_search >= bytes.len() {
        return input.to_string();
    }
    let body_open = brace_search;
    let mut depth = 1i32;
    let mut idx2 = body_open + 1;
    while idx2 < bytes.len() && depth > 0 {
        match bytes[idx2] {
            b'{' => depth += 1,
            b'}' => depth -= 1,
            _ => {}
        }
        idx2 += 1;
    }
    if depth != 0 {
        return input.to_string();
    }
    let body_close = idx2 - 1;
    let body = &input[body_open + 1..body_close];

    let mut renamed_body = replace_identifier(body, out_name, "gl_FragColor");
    // mainImage's coordinate parameter is `vec2`, but the `gl_FragCoord`
    // builtin is `vec4` -- must keep the `.xy` swizzle or every arithmetic
    // expression involving the renamed identifier becomes a vec4/vec2 type
    // mismatch that fails to compile. If the source already wrote the
    // parameter's own `.xy` explicitly (idiomatic in real Shadertoy code,
    // e.g. `fragCoord.xy`), this substitution would double it up into
    // `gl_FragCoord.xy.xy` -- collapse that back down; still exactly the
    // same value (a `.xy` swizzle of an already-2-component vector is a
    // no-op), just not doubled.
    renamed_body = replace_identifier(&renamed_body, coord_name, "gl_FragCoord.xy");
    renamed_body = renamed_body.replace("gl_FragCoord.xy.xy", "gl_FragCoord.xy");

    let before = input[..name_pos].trim_end();
    let before = before.strip_suffix("void").map(str::trim_end).unwrap_or(before);
    let separator = if before.is_empty() { "" } else { "\n" };
    let after = &input[body_close + 1..];

    format!("{before}{separator}void main(){{{renamed_body}}}{after}")
}

fn last_identifier_token(param: &str) -> Option<&str> {
    let token = param.trim().rsplit(char::is_whitespace).next()?;
    if token.is_empty()
        || !token
            .chars()
            .all(|c| c.is_alphanumeric() || c == '_')
    {
        return None;
    }
    Some(token)
}

// The inverse of normalize_mainimage_to_plain_main above: rewraps a plain
// `void main(){...}` using `gl_FragColor`/`gl_FragCoord` back into the
// `void mainImage(out vec4 fragColor, in vec2 fragCoord){...}` signature
// this app's Source tab (and real Shadertoy) actually requires. Called at
// the very end of unrewrite_twigl_shader, so "Import" always reconstructs
// source that will actually compile via ShaderRunner's own wrapping
// convention, instead of a `void main(){}` that conflicts with it.
fn wrap_plain_main_as_mainimage(input: &str) -> String {
    let Some(main_pos) = input.find("void main(){") else {
        return input.to_string();
    };
    let body_open = main_pos + "void main(){".len() - 1;
    let bytes = input.as_bytes();
    let mut depth = 1i32;
    let mut idx = body_open + 1;
    while idx < bytes.len() && depth > 0 {
        match bytes[idx] {
            b'{' => depth += 1,
            b'}' => depth -= 1,
            _ => {}
        }
        idx += 1;
    }
    if depth != 0 {
        return input.to_string();
    }
    let body_close = idx - 1;
    let body = &input[body_open + 1..body_close];

    let mut renamed_body = replace_identifier(body, "gl_FragColor", "fragColor");
    renamed_body = replace_identifier(&renamed_body, "gl_FragCoord", "fragCoord");

    let before = &input[..main_pos];
    let after = &input[body_close + 1..];
    format!(
        "{before}void mainImage(out vec4 fragColor,in vec2 fragCoord){{{renamed_body}}}{after}"
    )
}

fn strip_main_wrapper(input: &str) -> String {
    let trimmed = input.trim();
    if let Some(body) = trimmed.strip_prefix("void main(){") {
        if let Some(body) = body.strip_suffix('}') {
            return body.trim().to_string();
        }
    }
    trimmed.to_string()
}

fn apply_builtin_snippets(input: &str, mode: TwiglMode) -> String {
    if !matches!(mode, TwiglMode::Geekest) {
        return input.to_string();
    }
    let mut output = input.to_string();
    output = replace_identifier(&output, "PI", "3.14159265359");
    output = output.replace("vec2(0.0,1.0)", "vec2(0.,1.)");
    output
}

// GLSL ES 3.00 removes texture2D/textureCube/texture2DProj/shadow2D (and their
// *Lod variants) as builtins -- only the overloaded texture()/textureProj()/
// textureLod()/textureProjLod() family exists. Every twigl `300 es` export
// must rewrite these or the output will fail to compile under a real
// WebGL2/GLSL-ES-3.00 context, per ROADMAP.md/roadmap_twigl.md Phase 42.1.
// Same arity in every case, so a pure rename is always correct -- no
// argument-list restructuring needed.
const ES300_DEPRECATED_TEXTURE_FNS: &[(&str, &str)] = &[
    ("texture2DProjLod", "textureProjLod"),
    ("texture2DProj", "textureProj"),
    ("texture2DLod", "textureLod"),
    ("texture2D", "texture"),
    ("textureCubeLod", "textureLod"),
    ("textureCube", "texture"),
    ("shadow2DProj", "textureProj"),
    ("shadow2D", "texture"),
];

fn rewrite_es300_deprecated_texture_calls(input: &str) -> String {
    let mut output = input.to_string();
    for (from, to) in ES300_DEPRECATED_TEXTURE_FNS {
        output = replace_identifier(&output, from, to);
    }
    output
}

fn es300_output_name(mode: TwiglMode) -> &'static str {
    match mode {
        TwiglMode::Classic => "outColor",
        TwiglMode::Geek | TwiglMode::Geeker | TwiglMode::Geekest => "o",
    }
}

pub fn twigl_es300_header(mode: TwiglMode, mrt_targets: u8) -> String {
    let base = es300_output_name(mode);
    let mut header = String::from("#version 300 es\n");
    if mrt_targets >= 2 {
        // GLSL ES 3.00 requires an explicit layout(location=N) qualifier
        // whenever more than one fragment output is declared -- unqualified
        // multi-output binding is undefined/implementation-rejected. Confirmed
        // against twigl.app's own generated MRT source, which emits exactly
        // this form. See ROADMAP.md/roadmap_twigl.md Phase 42.2.
        for i in 0..mrt_targets {
            header.push_str(&format!("layout(location={i}) out vec4 {base}{i};\n"));
        }
    } else {
        header.push_str(&format!("out vec4 {base};\n"));
    }
    header
}

pub fn twigl_export_uniform_names(
    mode: TwiglMode,
    mrt_targets: u8,
    has_backbuffer: bool,
    has_sound: bool,
) -> Vec<String> {
    let out_base = if matches!(mode, TwiglMode::Classic) { "outColor" } else { "o" };
    let back_base = if matches!(mode, TwiglMode::Classic) { "backbuffer" } else { "b" };
    let sound_name = if matches!(mode, TwiglMode::Classic) { "sound" } else { "s" };

    let mut names = Vec::new();
    if mrt_targets >= 2 {
        names.push(format!("{out_base}0"));
        names.push(format!("{out_base}1"));
    } else {
        names.push(out_base.to_string());
    }

    if has_backbuffer {
        if mrt_targets >= 2 {
            names.push(format!("{back_base}0"));
            names.push(format!("{back_base}1"));
        } else {
            names.push(back_base.to_string());
        }
    }

    if has_sound {
        names.push(sound_name.to_string());
    }

    names
}

// Word-boundary-aware substring presence check, sharing
// is_identifier_boundary_match with replace_identifier above -- used by
// unrewrite_twigl_shader below to decide which uniform declarations to
// reconstruct and whether a void main(){} wrapper needs restoring.
fn identifier_present(text: &str, name: &str) -> bool {
    let mut start = 0usize;
    while let Some(pos) = text[start..].find(name) {
        let abs = start + pos;
        let end = abs + name.len();
        if is_identifier_boundary_match(text, abs, end) {
            return true;
        }
        start = abs + 1;
    }
    false
}

// Reconstructs a declaration scaffold for whichever uniforms are actually
// referenced in `body` that this app's own Source tab does NOT already
// provide. ShaderRunner's compilation wrapper (src/render/shader_runner.cpp,
// kFragmentPrefix) always declares iResolution/iMouse/iTime/iFrame/
// iFrameRate/iDate itself -- Source-tab shaders (including this app's own
// default shader) never declare those themselves, and doing so here would
// be a GLSL redefinition error when the reconstructed source is compiled.
// iChannel0 is the one exception: nothing else declares it, so it's
// reconstructed here, still only when actually referenced.
fn reconstruct_uniform_scaffold(body: &str) -> String {
    if identifier_present(body, "iChannel0") {
        "uniform sampler2D iChannel0;\n".to_string()
    } else {
        String::new()
    }
}

// Reverses twigl_es300_header: strips a leading `#version 300 es` directive
// and the output-variable declaration(s) it introduced, returning the
// remaining body plus whether a *single*, unqualified `out vec4 {base};`
// declaration was removed (as opposed to `layout(location=N)`-qualified MRT
// declarations, which are left unrenamed in the body -- see
// unrewrite_twigl_shader's doc comment for why MRT output names don't map
// back to gl_FragColor).
fn strip_es300_output_header(input: &str, mode: TwiglMode) -> (String, bool) {
    let base = es300_output_name(mode);
    let Some(rest) = input.strip_prefix("#version 300 es\n") else {
        return (input.to_string(), false);
    };

    let single_target_header = format!("out vec4 {base};\n");
    if let Some(after_header) = rest.strip_prefix(&single_target_header) {
        return (after_header.to_string(), true);
    }

    let mut remaining = rest;
    let mut index: u8 = 0;
    loop {
        let layout_line = format!("layout(location={index}) out vec4 {base}{index};\n");
        match remaining.strip_prefix(&layout_line) {
            Some(after) => {
                remaining = after;
                index += 1;
            }
            None => break,
        }
    }
    (remaining.to_string(), false)
}

// The inverse of rewrite_twigl_shader: takes twigl-mode source (as typed or
// pasted directly into twigl.app, or copied from it) and reconstructs a
// Shadertoy-compatible `mainImage`-scaffold-free... no -- a plain `void
// main(){}` fragment shader with the standard `iXxx` uniform names, so it can
// be dropped into µShader's Source editor and golfed/analyzed like any other
// shader. This is a best-effort reconstruction, not a lossless inverse in
// general: Geekest mode's builtin-constant substitution (`apply_builtin_snippets`,
// e.g. `PI` -> `3.14159265359`) is inherently lossy (many different inputs
// produce the same numeric literal), so text that happens to already contain
// that literal cannot be distinguished from text where a user's own `PI`
// reference was expanded -- this function does not attempt to reverse that
// one substitution. Every other step (uniform renaming, precision/uniform
// omission, `void main(){}` omission, `FC` shortening, ES300 `#version`/`out
// vec4` header removal) is a straightforward, well-defined inverse and is
// fully reversed. Round-trip correctness for every case that matters in
// practice (every bundled `fixtures/twigl_*.glsl` fixture, none of which trip
// the lossy PI/vec2 substitution) is verified by
// `unrewrite_then_rewrite_reproduces_every_bundled_twigl_fixture` below --
// not merely asserted.
//
// ES300 MRT is a special case: rewrite_twigl_shader_mrt never renames
// anything to `o0`/`o1` (or `outColor0`/`outColor1`) -- the source shader is
// expected to already reference those output variable names directly, since
// multiple render targets have no Shadertoy/gl_FragColor equivalent. So
// unrewriting MRT output strips the `#version 300 es` + `layout(location=N)
// out vec4 ...;` header lines but leaves `o0`/`o1` untouched in the body.
//
// Known, inherent ambiguity (not a bug, a property of the single-character
// uniform convention itself): in Geek/Geeker/Geekest input, a user's own
// locally-scoped variable literally named `r`/`m`/`t`/`f`/`b`/`s`/`o` is
// indistinguishable, at the text level, from the global uniform of the same
// name -- real GLSL scoping would let a local declaration shadow the global
// uniform, but this function has no scope information and reverses every
// occurrence. This is the same ambiguity anyone hand-porting geek-mode code
// already has to watch for; it is not introduced by this function.
pub fn unrewrite_twigl_shader(input: &str, mode: TwiglMode) -> String {
    let (stripped, rename_output_to_frag_color) = strip_es300_output_header(input, mode);

    let reversed: &[(&str, &str)] = match mode {
        TwiglMode::Classic => &[
            ("resolution", "iResolution"),
            ("mouse", "iMouse"),
            ("time", "iTime"),
            ("frame", "iFrame"),
            ("backbuffer", "iChannel0"),
        ],
        TwiglMode::Geek | TwiglMode::Geeker | TwiglMode::Geekest => &[
            ("r", "iResolution"),
            ("m", "iMouse"),
            ("t", "iTime"),
            ("f", "iFrame"),
            ("b", "iChannel0"),
        ],
    };

    let mut body = stripped;
    for (from, to) in reversed {
        body = replace_identifier(&body, from, to);
    }
    if rename_output_to_frag_color {
        body = replace_identifier(&body, es300_output_name(mode), "gl_FragColor");
    }
    if matches!(mode, TwiglMode::Geekest) {
        body = replace_identifier(&body, "FC", "gl_FragCoord");
    }

    if matches!(mode, TwiglMode::Classic | TwiglMode::Geek) {
        // Neither mode strips or omits precision/uniform declarations on
        // export (only Geeker and Geekest do); the reversed identifiers plus
        // the ES300 header removal above are already the complete inverse.
        return wrap_plain_main_as_mainimage(&body);
    }

    // Geeker (and Geekest) additionally omit precision/uniform declarations
    // on export; Geekest may additionally omit the void main(){} wrapper.
    let needs_main_wrapper = matches!(mode, TwiglMode::Geekest) && !identifier_present(&body, "main");
    if needs_main_wrapper {
        body = format!("void main(){{{body}}}");
    }
    body = wrap_plain_main_as_mainimage(&body);
    let scaffold = reconstruct_uniform_scaffold(&body);
    format!("{scaffold}{body}")
}

pub fn rewrite_twigl_shader(input: &str, mode: TwiglMode, es300: bool) -> String {
    let input = normalize_mainimage_to_plain_main(input);
    let (input, _renames) = resolve_rename_collisions(&input, mode, es300);
    let mut output = rewrite_twigl_uniforms(&input, mode);
    if matches!(mode, TwiglMode::Geeker | TwiglMode::Geekest) {
        output = strip_precision_and_uniform_declarations(&output);
    }
    if matches!(mode, TwiglMode::Geekest) {
        output = strip_main_wrapper(&output);
        output = apply_builtin_snippets(&output, mode);
    }
    if es300 {
        output = rewrite_es300_deprecated_texture_calls(&output);
        output = replace_identifier(&output, "gl_FragColor", es300_output_name(mode));
        let mut header = twigl_es300_header(mode, 1);
        header.push_str(&output);
        output = header;
    }
    output
}

pub fn rewrite_twigl_shader_mrt(input: &str, mode: TwiglMode, mrt_targets: u8) -> String {
    let input = normalize_mainimage_to_plain_main(input);
    // es300=false here (even though MRT is always an ES300 export, see the
    // comment below) -- unlike the single-target path, MRT never renames
    // gl_FragColor to o/outColor at all; the source is expected to already
    // reference o0/o1 (or outColor0/outColor1) directly, so there is no
    // later substitution step for a pre-existing bare "o"/"outColor" to
    // collide with, and resolve_rename_collisions's ES300 output-name check
    // would incorrectly "resolve" a collision that doesn't exist. The r/m/
    // t/f/b checks (mode-gated) and the Geekest FC check still apply.
    let (input, _renames) = resolve_rename_collisions(&input, mode, false);
    let mut output = rewrite_twigl_uniforms(&input, mode);
    if matches!(mode, TwiglMode::Geeker | TwiglMode::Geekest) {
        output = strip_precision_and_uniform_declarations(&output);
    }
    if matches!(mode, TwiglMode::Geekest) {
        output = strip_main_wrapper(&output);
        output = apply_builtin_snippets(&output, mode);
    }
    // rewrite_twigl_shader_mrt is always a #version 300 es export (MRT is an
    // ES300-only twigl feature), so the deprecated-texture-call rewrite from
    // 42.1 always applies here, unconditionally.
    output = rewrite_es300_deprecated_texture_calls(&output);
    let mut header = twigl_es300_header(mode, mrt_targets);
    header.push_str(&output);
    header
}

// GLSL global declarations (uniforms) may appear anywhere before their first
// use, so it's always correct to insert them immediately after the mandatory
// `#version 300 es` directive (which itself must stay the file's first line)
// or, when there is no `#version` line, at the very start of the file.
fn insert_after_version_directive(text: &str, insertion: &str) -> String {
    if insertion.is_empty() {
        return text.to_string();
    }
    if let Some(rest) = text.strip_prefix("#version 300 es\n") {
        format!("#version 300 es\n{insertion}{rest}")
    } else {
        format!("{insertion}{text}")
    }
}

// Classic/Geek modes require every uniform to be spelled out explicitly.
// Geeker/Geekest auto-complement the entire uniform block on twigl.app's own
// implementation side (per its documented "no need to declare precision and
// uniform" rule), which -- per that same rule -- covers backbuffer/sound too,
// so nothing needs to be emitted for those two modes.
pub fn twigl_backbuffer_and_sound_declarations(
    mode: TwiglMode,
    mrt_targets: u8,
    has_backbuffer: bool,
    has_sound: bool,
) -> String {
    if matches!(mode, TwiglMode::Geeker | TwiglMode::Geekest) {
        return String::new();
    }
    let (back_name, sound_name) = if matches!(mode, TwiglMode::Classic) {
        ("backbuffer", "sound")
    } else {
        ("b", "s")
    };

    let mut out = String::new();
    if has_backbuffer {
        if mrt_targets >= 2 {
            out.push_str(&format!("uniform sampler2D {back_name}0;\nuniform sampler2D {back_name}1;\n"));
        } else {
            out.push_str(&format!("uniform sampler2D {back_name};\n"));
        }
    }
    if has_sound {
        out.push_str(&format!("uniform float {sound_name};\n"));
    }
    out
}

// Detects local-identifier collisions that a pure text-substitution rewrite
// cannot safely avoid: Geek/Geeker/Geekest mode renames iResolution/iMouse/
// iTime/iFrame/iChannel0 to r/m/t/f/b (and, in ES300 modes, gl_FragColor to
// o/outColor, and in Geekest, gl_FragCoord to FC) wherever those identifiers
// appear -- with no GLSL scope information, so if the shader *already* has
// its own local variable/parameter named e.g. `r` for something unrelated
// (a real, observed case: a raymarching shader's own grid-cell-size
// variable), the rewrite doesn't fail loudly -- it silently merges two
// different meanings under one name. GLSL variable shadowing means this
// often still *compiles*, but produces visually wrong output wherever the
// renamed uniform is referenced after the local variable's declaration
// point (see unrewrite_twigl_shader's doc comment for the same, inherent
// ambiguity in the reverse direction). This is not something a text-level
// rewrite without full scope tracking can safely auto-resolve, so instead
// of guessing, this surfaces the risk as a warning for the UI to show, so
// the user can rename their own conflicting identifier before exporting.
fn generate_free_identifier(input: &str, base: &str) -> String {
    for i in 0u32.. {
        let candidate = format!("{base}_{i}");
        if !identifier_present(input, &candidate) {
            return candidate;
        }
    }
    unreachable!("identifier_present is finite-time; this loop always terminates well before u32 overflow")
}

// Automatically resolves every rename-target collision described above by
// renaming the shader's own *pre-existing* conflicting identifier out of
// the way (to a fresh, guaranteed-unused name, e.g. a local `r` variable
// becomes `r_0`) before the uniform/output/coordinate substitution runs.
// This is safe without any GLSL scope tracking: renaming a distinct
// identifier to a name that appears nowhere else in the source never
// changes what it refers to, and once it's out of the way there is no
// longer any text-level ambiguity for the later substitution passes (which
// rename by blind, scope-unaware text substitution) to trip over. Returns
// the modified source plus a human-readable list of the renames performed
// (for the UI to inform the user what changed), empty when none were
// needed. Called automatically by rewrite_twigl_shader/rewrite_twigl_shader_mrt,
// so every twigl export is collision-free by construction; also callable
// directly for diagnostics/tests.
pub fn resolve_rename_collisions(input: &str, mode: TwiglMode, es300: bool) -> (String, Vec<String>) {
    let mut output = input.to_string();
    let mut applied = Vec::new();

    if !matches!(mode, TwiglMode::Classic) {
        // Classic mode's targets (resolution/mouse/time/frame/backbuffer)
        // are long, ordinary-looking English words -- a user's own local
        // variable coincidentally sharing one of those exact names is far
        // less likely, and this mode was never implicated in the observed
        // bug. Scoped out to avoid noisy, unnecessary renames of common words.
        const UNIFORM_TARGETS: &[(&str, &str)] = &[
            ("iResolution", "r"),
            ("iMouse", "m"),
            ("iTime", "t"),
            ("iFrame", "f"),
            ("iChannel0", "b"),
        ];
        for (long_name, short_name) in UNIFORM_TARGETS {
            if identifier_present(&output, long_name) && identifier_present(&output, short_name) {
                let fresh = generate_free_identifier(&output, short_name);
                output = replace_identifier(&output, short_name, &fresh);
                applied.push(format!(
                    "renamed your '{short_name}' to '{fresh}' (it would otherwise collide with the {long_name} uniform)"
                ));
            }
        }
    }

    if es300 {
        let out_name = es300_output_name(mode);
        if identifier_present(&output, out_name) {
            let fresh = generate_free_identifier(&output, out_name);
            output = replace_identifier(&output, out_name, &fresh);
            applied.push(format!(
                "renamed your '{out_name}' to '{fresh}' (it would otherwise collide with the ES 3.00 output variable)"
            ));
        }
    }

    if matches!(mode, TwiglMode::Geekest) && identifier_present(&output, "FC") {
        let fresh = generate_free_identifier(&output, "FC");
        output = replace_identifier(&output, "FC", &fresh);
        applied.push(format!(
            "renamed your 'FC' to '{fresh}' (it would otherwise collide with Geekest's gl_FragCoord shorthand)"
        ));
    }

    (output, applied)
}

// The single entry point the C++ shell should call for every twigl-related
// output -- the live Export-panel preview, the budget badge, and the
// clipboard "Copy for twigl.app" action all call this same function so they
// can never diverge (closes ROADMAP.md/roadmap_twigl.md Phase 42.3 and 42.4
// together, since both bugs traced back to the absence of exactly this
// shared path). `mrt_targets >= 2` selects the MRT rewrite path; `es300` is
// only consulted for the single-target path (MRT is always ES300 in twigl).
pub fn rewrite_twigl_shader_full(
    input: &str,
    mode: TwiglMode,
    es300: bool,
    mrt_targets: u8,
    has_backbuffer: bool,
    has_sound: bool,
) -> String {
    let mut output = if mrt_targets >= 2 {
        rewrite_twigl_shader_mrt(input, mode, mrt_targets)
    } else {
        rewrite_twigl_shader(input, mode, es300)
    };

    let declarations = twigl_backbuffer_and_sound_declarations(mode, mrt_targets, has_backbuffer, has_sound);
    if !declarations.is_empty() {
        output = insert_after_version_directive(&output, &declarations);
    }
    output
}

#[cfg(test)]
mod tests {
    use super::{
        identifier_present, normalize_mainimage_to_plain_main, resolve_rename_collisions,
        rewrite_es300_deprecated_texture_calls, rewrite_twigl_shader, rewrite_twigl_shader_full,
        rewrite_twigl_shader_mrt, rewrite_twigl_uniforms, twigl_backbuffer_and_sound_declarations,
        twigl_es300_header, twigl_export_uniform_names, twigl_snippet, twigl_snippets,
        unrewrite_twigl_shader, TwiglMode,
    };

    #[test]
    fn rewrites_classic_uniform_names() {
        let input = "vec4 f(vec2 p){return vec4(iResolution.xy,iTime,iMouse.x);}";
        let output = rewrite_twigl_uniforms(input, TwiglMode::Classic);
        assert_eq!(output, "vec4 f(vec2 p){return vec4(resolution.xy,time,mouse.x);}");
    }

    #[test]
    fn rewrites_geek_uniform_names() {
        let input = "vec4 f(vec2 p){return vec4(iResolution.xy,iTime,iMouse.x);}";
        let output = rewrite_twigl_uniforms(input, TwiglMode::Geek);
        assert_eq!(output, "vec4 f(vec2 p){return vec4(r.xy,t,m.x);}");
    }

    #[test]
    fn rewrites_backbuffer_sampler_for_classic_mode() {
        let input = "vec4 f(vec2 p){return texture2D(iChannel0,p);}";
        let output = rewrite_twigl_uniforms(input, TwiglMode::Classic);
        assert_eq!(output, "vec4 f(vec2 p){return texture2D(backbuffer,p);}");
    }

    #[test]
    fn rewrites_backbuffer_sampler_for_geek_style_modes() {
        let input = "vec4 f(vec2 p){return texture2D(iChannel0,p);}";
        let output = rewrite_twigl_uniforms(input, TwiglMode::Geekest);
        assert_eq!(output, "vec4 f(vec2 p){return texture2D(b,p);}");
    }

    #[test]
    fn rewrites_es300_fragcolor_for_classic_mode() {
        let input = "void main(){gl_FragColor=vec4(1.0);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Classic, true);
        assert_eq!(output, "#version 300 es\nout vec4 outColor;\nvoid main(){outColor=vec4(1.0);}");
    }

    #[test]
    fn rewrites_es300_fragcolor_for_geek_mode() {
        let input = "void main(){gl_FragColor=vec4(1.0);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Geek, true);
        assert_eq!(output, "#version 300 es\nout vec4 o;\nvoid main(){o=vec4(1.0);}");
    }

    #[test]
    fn prefixes_es300_output_name_for_classic_mode() {
        let input = "void main(){gl_FragColor=vec4(1.0);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Classic, true);
        assert!(output.starts_with("#version 300 es\nout vec4 outColor;\n"));
    }

    #[test]
    fn strips_precision_and_uniform_declarations_for_geeker_mode() {
        let input = "precision mediump float;\nuniform vec2 iResolution;\nuniform vec3 iMouse;\nvoid main(){gl_FragColor=vec4(1.0);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Geeker, false);
        assert_eq!(output, "void main(){gl_FragColor=vec4(1.0);}");
    }

    #[test]
    fn omits_void_main_for_geekest_mode_without_helpers() {
        let input = "void main(){gl_FragColor=vec4(1.0);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Geekest, false);
        assert_eq!(output, "gl_FragColor=vec4(1.0);");
    }

    #[test]
    fn keeps_void_main_wrapper_for_geekest_mode_when_a_helper_function_exists() {
        let input = "float f(float x){return x*2.;}\nvoid main(){gl_FragColor=vec4(f(1.0));}";
        let output = rewrite_twigl_shader(input, TwiglMode::Geekest, false);
        assert_eq!(
            output,
            "float f(float x){return x*2.;}\nvoid main(){gl_FragColor=vec4(f(1.0));}"
        );
    }

    #[test]
    fn applies_builtin_snippets_for_geekest_mode() {
        let input = "void main(){gl_FragColor=vec4(PI,vec2(0.0,1.0),1.0);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Geekest, false);
        assert_eq!(output, "gl_FragColor=vec4(3.14159265359,vec2(0.,1.),1.0);");
    }

    #[test]
    fn substitutes_builtin_constants_for_geekest_mode() {
        let input = "void main(){gl_FragColor=vec4(PI);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Geekest, false);
        assert_eq!(output, "gl_FragColor=vec4(3.14159265359);");
    }

    #[test]
    fn does_not_rewrite_identifiers_that_merely_contain_a_uniform_name() {
        let input = "void main(){float iTimeScale=2.0;gl_FragColor=vec4(iTime*iTimeScale);}";
        let output = rewrite_twigl_uniforms(input, TwiglMode::Classic);
        assert_eq!(output, "void main(){float iTimeScale=2.0;gl_FragColor=vec4(time*iTimeScale);}");
    }

    #[test]
    fn single_target_classic_export_metadata_has_only_outcolor() {
        let names = twigl_export_uniform_names(TwiglMode::Classic, 1, false, false);
        assert_eq!(names, vec!["outColor".to_string()]);
    }

    #[test]
    fn mrt_backbuffer_and_sound_export_metadata_for_classic_mode() {
        let names = twigl_export_uniform_names(TwiglMode::Classic, 2, true, true);
        assert_eq!(
            names,
            vec![
                "outColor0".to_string(),
                "outColor1".to_string(),
                "backbuffer0".to_string(),
                "backbuffer1".to_string(),
                "sound".to_string(),
            ]
        );
    }

    #[test]
    fn mrt_backbuffer_and_sound_export_metadata_for_geek_style_modes() {
        let names = twigl_export_uniform_names(TwiglMode::Geekest, 2, true, true);
        assert_eq!(
            names,
            vec!["o0".to_string(), "o1".to_string(), "b0".to_string(), "b1".to_string(), "s".to_string()]
        );
    }

    #[test]
    fn single_target_backbuffer_only_export_metadata_for_geek_mode() {
        let names = twigl_export_uniform_names(TwiglMode::Geek, 1, true, false);
        assert_eq!(names, vec!["o".to_string(), "b".to_string()]);
    }

    #[test]
    fn snippet_library_exposes_the_documented_geekest_helper_set() {
        let names: Vec<&str> = twigl_snippets().iter().map(|s| s.name).collect();
        assert_eq!(
            names,
            vec![
                "PI", "PI2", "hsv", "rotate2D", "rotate3D", "fsnoise", "fsnoiseDigits", "snoise2D",
                "snoise3D", "snoise4D",
            ]
        );
    }

    #[test]
    fn snippet_lookup_returns_the_matching_source_and_none_for_unknown_names() {
        assert_eq!(twigl_snippet("PI"), Some("const float PI=3.14159265359;"));
        assert!(twigl_snippet("rotate2D").unwrap().contains("mat2 rotate2D(float r)"));
        assert_eq!(twigl_snippet("doesNotExist"), None);
    }

    #[test]
    fn snoise_snippets_expose_the_documented_2d_3d_and_4d_signatures() {
        assert!(twigl_snippet("snoise2D").unwrap().starts_with("float snoise2D(vec2 p)"));
        assert!(twigl_snippet("snoise3D").unwrap().starts_with("float snoise3D(vec3 p)"));
        assert!(twigl_snippet("snoise4D").unwrap().starts_with("float snoise4D(vec4 p)"));
    }

    #[test]
    fn every_snippet_has_balanced_parentheses_and_braces() {
        for snippet in twigl_snippets() {
            let mut parens = 0i32;
            let mut braces = 0i32;
            for ch in snippet.source.chars() {
                match ch {
                    '(' => parens += 1,
                    ')' => parens -= 1,
                    '{' => braces += 1,
                    '}' => braces -= 1,
                    _ => {}
                }
                assert!(parens >= 0 && braces >= 0, "{} has unbalanced delimiters", snippet.name);
            }
            assert_eq!(parens, 0, "{} has unbalanced parentheses", snippet.name);
            assert_eq!(braces, 0, "{} has unbalanced braces", snippet.name);
        }
    }

    #[test]
    fn es300_header_declares_a_single_output_for_classic_mode() {
        assert_eq!(twigl_es300_header(TwiglMode::Classic, 1), "#version 300 es\nout vec4 outColor;\n");
    }

    #[test]
    fn es300_header_declares_two_outputs_for_mrt_in_classic_mode() {
        assert_eq!(
            twigl_es300_header(TwiglMode::Classic, 2),
            "#version 300 es\nlayout(location=0) out vec4 outColor0;\nlayout(location=1) out vec4 outColor1;\n"
        );
    }

    #[test]
    fn es300_header_declares_two_outputs_for_mrt_in_geek_style_modes() {
        assert_eq!(
            twigl_es300_header(TwiglMode::Geekest, 2),
            "#version 300 es\nlayout(location=0) out vec4 o0;\nlayout(location=1) out vec4 o1;\n"
        );
    }

    // Phase 42.2 regression guard: single-target output must stay unqualified
    // (matching twigl.app's own single-target convention) -- only multi-output
    // declarations require an explicit layout(location=N).
    #[test]
    fn es300_header_single_target_output_has_no_layout_qualifier() {
        assert!(!twigl_es300_header(TwiglMode::Classic, 1).contains("layout("));
        assert!(!twigl_es300_header(TwiglMode::Geekest, 1).contains("layout("));
    }

    #[test]
    fn rewrite_twigl_shader_mrt_supports_the_single_target_case() {
        let input = "void main(){outColor=vec4(1.0);}";
        let output = rewrite_twigl_shader_mrt(input, TwiglMode::Classic, 1);
        assert_eq!(output, "#version 300 es\nout vec4 outColor;\nvoid main(){outColor=vec4(1.0);}");
    }

    #[test]
    fn rewrite_twigl_shader_mrt_prefixes_the_correct_number_of_output_declarations() {
        let input = "void main(){o0=vec4(iTime);o1=vec4(1.0);}";
        let output = rewrite_twigl_shader_mrt(input, TwiglMode::Geek, 2);
        assert_eq!(
            output,
            "#version 300 es\nlayout(location=0) out vec4 o0;\nlayout(location=1) out vec4 o1;\nvoid main(){o0=vec4(t);o1=vec4(1.0);}"
        );
    }

    #[test]
    fn twigl_classic_fixture_matches_the_classic_mode_rewrite_of_the_shared_source_fixture() {
        let source = include_str!("../../fixtures/twigl_source.glsl").replace("\r\n", "\n");
        let expected = include_str!("../../fixtures/twigl_classic.glsl").replace("\r\n", "\n");
        let output = rewrite_twigl_shader(&source, TwiglMode::Classic, false);
        assert_eq!(output.trim_end(), expected.trim_end());
    }

    #[test]
    fn twigl_geekest_fixture_matches_the_geekest_mode_rewrite_of_the_shared_source_fixture() {
        let source = include_str!("../../fixtures/twigl_source.glsl").replace("\r\n", "\n");
        let expected = include_str!("../../fixtures/twigl_geekest.glsl").replace("\r\n", "\n");
        let output = rewrite_twigl_shader(&source, TwiglMode::Geekest, false);
        assert_eq!(output.trim_end(), expected.trim_end());
    }

    #[test]
    fn twigl_300es_fixture_matches_the_classic_mode_es300_rewrite_of_the_shared_source_fixture() {
        let source = include_str!("../../fixtures/twigl_source.glsl").replace("\r\n", "\n");
        let expected = include_str!("../../fixtures/twigl_300es.glsl").replace("\r\n", "\n");
        let output = rewrite_twigl_shader(&source, TwiglMode::Classic, true);
        assert_eq!(output.trim_end(), expected.trim_end());
    }

    // --- Phase 42.1 -----------------------------------------------------

    #[test]
    fn es300_rewrite_replaces_every_deprecated_texture_call_variant() {
        let input = "vec4 f(sampler2D s,samplerCube c,vec2 p,vec3 q,vec4 r){return texture2D(s,p)+textureCube(c,q)+texture2DProj(s,r)+shadow2D(s,r)+texture2DLod(s,p,0.)+textureCubeLod(c,q,0.);}";
        let output = rewrite_es300_deprecated_texture_calls(input);
        assert!(!output.contains("texture2D"));
        assert!(!output.contains("textureCube("));
        assert!(!output.contains("shadow2D"));
        assert!(output.contains("texture(s,p)"));
        assert!(output.contains("texture(c,q)"));
        assert!(output.contains("textureProj(s,r)"));
        assert!(output.contains("textureLod(s,p,0.)"));
        assert!(output.contains("textureLod(c,q,0.)"));
    }

    #[test]
    fn es300_rewrite_of_single_target_export_never_leaves_a_deprecated_texture_call() {
        let input = "void main(){gl_FragColor=texture2D(iChannel0,gl_FragCoord.xy);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Classic, true);
        assert!(!output.contains("texture2D"), "ES300 export still contains texture2D: {output}");
        assert!(output.contains("texture(backbuffer,"));
    }

    #[test]
    fn es300_rewrite_of_mrt_export_never_leaves_a_deprecated_texture_call() {
        let input = "void main(){o0=texture2D(b,FC.xy);o1=vec4(1.0);}";
        let output = rewrite_twigl_shader_mrt(input, TwiglMode::Geekest, 2);
        assert!(!output.contains("texture2D"), "MRT export still contains texture2D: {output}");
        assert!(output.contains("texture(b,"));
    }

    #[test]
    fn es300_rewrite_never_touches_identifiers_that_merely_contain_texture2d_as_a_substring() {
        let input = "float texture2Dish=1.;";
        let output = rewrite_es300_deprecated_texture_calls(input);
        assert_eq!(output, input);
    }

    #[test]
    fn non_es300_export_keeps_texture2d_unchanged() {
        let input = "void main(){gl_FragColor=texture2D(iChannel0,gl_FragCoord.xy);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Classic, false);
        assert!(output.contains("texture2D("), "non-ES300 export must keep GLSL ES 1.00's texture2D");
    }

    // --- Phase 42.3 -------------------------------------------------------

    #[test]
    fn backbuffer_and_sound_declarations_empty_when_neither_requested() {
        assert_eq!(
            twigl_backbuffer_and_sound_declarations(TwiglMode::Classic, 1, false, false),
            ""
        );
    }

    #[test]
    fn backbuffer_and_sound_declarations_for_classic_single_target() {
        assert_eq!(
            twigl_backbuffer_and_sound_declarations(TwiglMode::Classic, 1, true, true),
            "uniform sampler2D backbuffer;\nuniform float sound;\n"
        );
    }

    #[test]
    fn backbuffer_and_sound_declarations_for_geek_style_mrt() {
        assert_eq!(
            twigl_backbuffer_and_sound_declarations(TwiglMode::Geek, 2, true, true),
            "uniform sampler2D b0;\nuniform sampler2D b1;\nuniform float s;\n"
        );
    }

    // Geeker/Geekest auto-complement the entire uniform block on twigl.app's
    // own implementation side, so no declaration should ever be emitted.
    #[test]
    fn backbuffer_and_sound_declarations_empty_for_auto_complemented_modes() {
        assert_eq!(twigl_backbuffer_and_sound_declarations(TwiglMode::Geeker, 1, true, true), "");
        assert_eq!(twigl_backbuffer_and_sound_declarations(TwiglMode::Geekest, 2, true, true), "");
    }

    // --- Phase 42.3 / 42.4 combined entry point ----------------------------

    #[test]
    fn rewrite_full_actually_applies_the_backbuffer_and_sound_toggles() {
        let input = "void main(){gl_FragColor=vec4(1.0);}";
        let without = rewrite_twigl_shader_full(input, TwiglMode::Classic, false, 1, false, false);
        let with_both = rewrite_twigl_shader_full(input, TwiglMode::Classic, false, 1, true, true);
        assert!(!without.contains("uniform sampler2D backbuffer"));
        assert!(with_both.contains("uniform sampler2D backbuffer;"));
        assert!(with_both.contains("uniform float sound;"));
        assert_ne!(without, with_both, "toggling backbuffer/sound must change the exported text");
    }

    #[test]
    fn rewrite_full_places_declarations_after_the_version_directive_for_es300() {
        let input = "void main(){gl_FragColor=vec4(1.0);}";
        let output = rewrite_twigl_shader_full(input, TwiglMode::Classic, true, 1, true, false);
        assert!(output.starts_with("#version 300 es\nuniform sampler2D backbuffer;\n"));
    }

    #[test]
    fn rewrite_full_routes_to_the_mrt_path_when_two_targets_are_selected() {
        let input = "void main(){o0=vec4(iTime);o1=vec4(1.0);}";
        let via_full = rewrite_twigl_shader_full(input, TwiglMode::Geek, false, 2, false, false);
        let via_mrt_directly = rewrite_twigl_shader_mrt(input, TwiglMode::Geek, 2);
        assert_eq!(via_full, via_mrt_directly);
    }

    #[test]
    fn rewrite_full_matches_a_manually_composed_single_target_plus_declarations_result() {
        let input = "void main(){gl_FragColor=vec4(1.0);}";
        let base = rewrite_twigl_shader(input, TwiglMode::Classic, false);
        let expected = format!("uniform sampler2D backbuffer;\n{base}");
        let output = rewrite_twigl_shader_full(input, TwiglMode::Classic, false, 1, true, false);
        assert_eq!(output, expected);
    }

    // --- mainImage normalization (regression coverage for the bug where a
    // genuine Shadertoy-style `void mainImage(out vec4 X, in vec2 Y){...}`
    // source -- which is what this app's own Source tab actually requires,
    // see ShaderRunner/default_shader.h -- was never unwrapped at all, so
    // every downstream gl_FragColor/gl_FragCoord/iResolution substitution
    // pass silently did nothing and the twigl export was just the untouched
    // mainImage wrapper) -----------------------------------------------

    #[test]
    fn rewrite_unwraps_a_genuine_mainimage_shader_with_golfed_parameter_names() {
        // Mirrors a real user report: mainImage's own out/in parameters
        // golfed down to single letters, exactly as this app's own golfer
        // would produce, referencing iResolution/iTime as normal.
        let input = "void mainImage(out vec4 I,in vec2 B){vec2 C=(B-.5*iResolution.xy)/iResolution.y;float a=iTime*.6;I=vec4(C,a,1.);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Geekest, false);
        assert!(!output.contains("mainImage"), "the mainImage wrapper must be fully unwrapped: {output}");
        assert!(output.contains("FC.xy"), "the vec2 coordinate parameter must become FC.xy (not bare FC, which is vec4): {output}");
        assert!(output.contains("gl_FragColor=vec4(C,a,1.);"), "the out parameter must become gl_FragColor: {output}");
        assert!(output.contains("r.xy") && output.contains("r.y"), "iResolution must still shorten to r: {output}");
        assert!(output.contains('t'), "iTime must still shorten to t: {output}");
    }

    #[test]
    fn rewrite_unwraps_mainimage_for_es300_output_naming_too() {
        let input = "void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(fragCoord,0.,1.);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Classic, true);
        assert_eq!(
            output,
            "#version 300 es\nout vec4 outColor;\nvoid main(){outColor=vec4(gl_FragCoord.xy,0.,1.);}"
        );
    }

    #[test]
    fn rewrite_leaves_helper_functions_declared_outside_mainimage_untouched() {
        let input = "float helperFn(float x){return x*2.;}\nvoid mainImage(out vec4 I,in vec2 B){I=vec4(helperFn(iTime));}";
        let output = rewrite_twigl_shader(input, TwiglMode::Geekest, false);
        assert!(output.contains("float helperFn(float x){return x*2.;}"));
        assert!(output.contains("gl_FragColor=vec4(helperFn(t));"));
    }

    #[test]
    fn rewrite_is_a_no_op_normalization_when_mainimage_is_absent() {
        // Already plain-`main`-style input (e.g. hand-written twigl-shorthand,
        // or a second pass over already-unwrapped text) must be left exactly
        // as every other rewrite pass already handles it.
        let input = "void main(){gl_FragColor=vec4(1.0);}";
        assert_eq!(rewrite_twigl_shader(input, TwiglMode::Classic, false), "void main(){gl_FragColor=vec4(1.0);}");
    }

    #[test]
    fn unrewrite_always_rewraps_into_a_mainimage_signature_this_apps_source_tab_can_compile() {
        // This app's Source tab (src/render/shader_runner.cpp) always wraps
        // user source with its own `void main(){ mainImage(...); }` calling
        // into a user-defined mainImage -- a plain `void main(){}` produced
        // by Import would silently fail to compile (duplicate main, no
        // mainImage to call). Covers every mode, since the bug applied to
        // all of them equally.
        for mode in [TwiglMode::Classic, TwiglMode::Geek, TwiglMode::Geeker, TwiglMode::Geekest] {
            let twigl_text = rewrite_twigl_shader("void main(){gl_FragColor=vec4(1.0);}", mode, false);
            let imported = unrewrite_twigl_shader(&twigl_text, mode);
            assert!(
                imported.contains("void mainImage(out vec4 fragColor,in vec2 fragCoord)"),
                "{mode:?}: {imported}"
            );
            assert!(!imported.contains("void main("), "{mode:?}: {imported}");
        }
    }

    #[test]
    fn shadertoy_style_source_round_trips_through_export_and_import_semantically() {
        // Full loop: genuine Shadertoy-style Source -> twigl export -> Import
        // back into Source -> re-export -> must match the first export
        // exactly, proving the mainImage unwrap/rewrap pair are true inverses
        // of each other (not just individually plausible).
        let original = "void mainImage(out vec4 fragColor,in vec2 fragCoord){vec2 uv=fragCoord.xy/iResolution.xy;fragColor=vec4(uv,sin(iTime),1.0);}";
        for (mode, es300) in [
            (TwiglMode::Classic, false),
            (TwiglMode::Classic, true),
            (TwiglMode::Geek, false),
            (TwiglMode::Geekest, false),
            (TwiglMode::Geekest, true),
        ] {
            let exported = rewrite_twigl_shader(original, mode, es300);
            let imported = unrewrite_twigl_shader(&exported, mode);
            let reexported = rewrite_twigl_shader(&imported, mode, es300);
            assert_eq!(reexported, exported, "mode={mode:?} es300={es300}");
        }
    }

    // --- Automatic rename-collision resolution (regression coverage for a
    // real, user-reported case: a raymarching shader's own `r` local
    // variable, used for grid cell size, silently merging with the
    // iResolution->r rename and corrupting a later screen-space
    // normalization). Per the user's explicit request, this is resolved
    // automatically (the shader's own conflicting identifier is renamed out
    // of the way) rather than merely flagged for the user to fix by hand. --

    #[test]
    fn auto_renames_a_local_identifier_that_collides_with_a_geek_family_uniform_target() {
        // Mirrors the actual reported shader: iResolution used early, then
        // a local `r` declared and used for something unrelated later.
        let input = "vec2 C=(B-.5*iResolution.xy)/iResolution.y;vec3 r=vec3(1.8,1.8,2.2);vec3 k=floor(c/r);";
        let (output, applied) = resolve_rename_collisions(input, TwiglMode::Geekest, false);
        assert_eq!(applied.len(), 1);
        assert!(applied[0].contains('r'));
        assert!(applied[0].contains("iResolution"));
        // The local variable's own declaration and every use of it must now
        // share the SAME fresh name (not "r"), and "iResolution" must still
        // be present, untouched, ready for the normal r-substitution pass
        // that runs after this one.
        assert!(output.contains("iResolution"));
        assert!(output.contains("vec3 r_0=vec3(1.8,1.8,2.2);"));
        assert!(output.contains("floor(c/r_0)"));
        assert!(!identifier_present(&output, "r"));
    }

    #[test]
    fn rewrite_twigl_shader_produces_collision_free_geekest_output_for_the_reported_shader() {
        // End-to-end: the exported text must unambiguously use `r` for
        // iResolution everywhere, with the original local variable renamed
        // out of the way -- not a mix of both meanings under one name.
        let input = "void mainImage(out vec4 I,in vec2 B){vec2 C=(B-.5*iResolution.xy)/iResolution.y;vec3 r=vec3(1.8,1.8,2.2);vec3 k=floor(C.xyy/r);I=vec4(k+C.xyy/iResolution.xy,1.);}";
        let output = rewrite_twigl_shader(input, TwiglMode::Geekest, false);
        assert!(output.contains("r_0"), "the local grid variable must survive under a fresh name: {output}");
        // Every remaining bare `r` must mean iResolution: check the count of
        // the *whole-word* "r" matches the number of original iResolution
        // occurrences (2), not 0 and not mixed with the local variable's.
        let bare_r_count = output.matches("r.xy").count() + output.matches("r);").count();
        assert_eq!(bare_r_count, 2, "expected exactly the 2 original iResolution.xy/iResolution.y uses to become bare r: {output}");
    }

    #[test]
    fn no_rename_needed_when_the_only_short_name_present_is_the_uniform_itself() {
        let input = "vec2 C=(B-.5*iResolution.xy)/iResolution.y;";
        assert!(resolve_rename_collisions(input, TwiglMode::Geekest, false).1.is_empty());
    }

    #[test]
    fn no_renames_applied_for_classic_mode() {
        // Classic mode's targets are long words (resolution/mouse/...), not
        // single letters -- scoped out to avoid noisy, unnecessary renames.
        let input = "vec2 C=(B-.5*iResolution.xy)/iResolution.y;float resolution=1.;";
        let (output, applied) = resolve_rename_collisions(input, TwiglMode::Classic, false);
        assert!(applied.is_empty());
        assert_eq!(output, input);
    }

    #[test]
    fn resolves_multiple_distinct_collisions_independently() {
        let input = "float t=1.;float m=2.;vec4 x=vec4(iTime,iMouse.x,0.,1.);";
        let (output, applied) = resolve_rename_collisions(input, TwiglMode::Geek, false);
        assert_eq!(applied.len(), 2);
        assert!(applied.iter().any(|w| w.contains('t') && w.contains("iTime")));
        assert!(applied.iter().any(|w| w.contains('m') && w.contains("iMouse")));
        assert!(output.contains("t_0=1.") || output.contains("t_0 = 1."));
        assert!(output.contains("m_0=2.") || output.contains("m_0 = 2."));
    }

    #[test]
    fn resolves_es300_output_name_collision_only_when_es300_is_active() {
        let input = "void mainImage(out vec4 fragColor,in vec2 fragCoord){float o=1.;fragColor=vec4(o);}";
        let normalized = normalize_mainimage_to_plain_main(input);
        assert!(resolve_rename_collisions(&normalized, TwiglMode::Geekest, false).1.is_empty());
        let (output, applied) = resolve_rename_collisions(&normalized, TwiglMode::Geekest, true);
        assert_eq!(applied.len(), 1);
        assert!(applied[0].contains('o'));
        assert!(output.contains("o_0"));
    }

    #[test]
    fn resolves_fc_collision_only_for_geekest_mode() {
        let input = "float FC=1.;gl_FragColor=vec4(FC);";
        assert!(resolve_rename_collisions(input, TwiglMode::Geek, false).1.is_empty());
        let (output, applied) = resolve_rename_collisions(input, TwiglMode::Geekest, false);
        assert_eq!(applied.len(), 1);
        assert!(applied[0].contains("FC"));
        assert!(output.contains("FC_0"));
    }

    #[test]
    fn generated_fresh_names_never_collide_with_an_already_used_numbered_variant() {
        // If the shader already happens to use "r_0" for something else,
        // the generator must skip past it rather than reuse it.
        let input = "vec2 C=(B-.5*iResolution.xy)/iResolution.y;float r_0=1.;float r=2.;float x=r+r_0;";
        let (output, applied) = resolve_rename_collisions(input, TwiglMode::Geekest, false);
        assert_eq!(applied.len(), 1);
        assert!(output.contains("r_1"), "must skip the already-used r_0: {output}");
        assert!(output.contains("float r_0=1.;"), "the pre-existing r_0 must survive untouched: {output}");
    }

    // --- Phase 43.2 -- round-trip import (unrewrite_twigl_shader) --------

    #[test]
    fn unrewrite_then_rewrite_reproduces_every_bundled_twigl_fixture() {
        let classic = include_str!("../../fixtures/twigl_classic.glsl").replace("\r\n", "\n");
        let geekest = include_str!("../../fixtures/twigl_geekest.glsl").replace("\r\n", "\n");

        let classic_trimmed = classic.trim_end();
        let roundtrip_classic = rewrite_twigl_shader(
            &unrewrite_twigl_shader(classic_trimmed, TwiglMode::Classic),
            TwiglMode::Classic,
            false,
        );
        assert_eq!(roundtrip_classic.trim_end(), classic_trimmed);

        let geekest_trimmed = geekest.trim_end();
        let roundtrip_geekest = rewrite_twigl_shader(
            &unrewrite_twigl_shader(geekest_trimmed, TwiglMode::Geekest),
            TwiglMode::Geekest,
            false,
        );
        assert_eq!(roundtrip_geekest.trim_end(), geekest_trimmed);
    }

    // --- ES300/MRT unrewrite (regression coverage for the previously-missing
    // `#version 300 es` / `out vec4 ...;` structural reversal) -------------

    #[test]
    fn unrewrite_reverses_es300_single_target_header_and_output_name_for_classic_mode() {
        let input = "#version 300 es\nout vec4 outColor;\nvoid main(){outColor=vec4(1.0);}";
        let output = unrewrite_twigl_shader(input, TwiglMode::Classic);
        assert!(!output.contains("#version 300 es"));
        assert!(!output.contains("out vec4 outColor;"));
        // Rewrapped into this app's Source-tab convention (see
        // normalize_mainimage_to_plain_main/wrap_plain_main_as_mainimage's
        // own doc comments): a plain `void main(){}` using gl_FragColor
        // directly would never compile via ShaderRunner's own wrapping.
        assert!(output.contains("void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}"));
    }

    #[test]
    fn unrewrite_reverses_es300_single_target_header_and_output_name_for_geekest_mode() {
        let input = "#version 300 es\nout vec4 o;\nvoid main(){o=vec4(t);}";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        assert!(!output.contains("#version 300 es"));
        assert!(!output.contains("out vec4 o;"));
        assert!(output.contains("void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(iTime);}"));
    }

    #[test]
    fn unrewrite_reverses_es300_mrt_header_but_leaves_output_names_untouched_for_classic_mode() {
        let input = "#version 300 es\nlayout(location=0) out vec4 outColor0;\nlayout(location=1) out vec4 outColor1;\nvoid main(){outColor0=vec4(time);outColor1=vec4(1.0);}";
        let output = unrewrite_twigl_shader(input, TwiglMode::Classic);
        assert!(!output.contains("#version 300 es"));
        assert!(!output.contains("layout(location="));
        // MRT output names have no Shadertoy/gl_FragColor equivalent -- they
        // must survive unrewriting exactly as they appeared in the input.
        assert!(output.contains("outColor0=vec4(iTime);outColor1=vec4(1.0);"));
        assert!(!output.contains("gl_FragColor"));
    }

    #[test]
    fn unrewrite_reverses_es300_mrt_header_but_leaves_output_names_untouched_for_geekest_mode() {
        let input = "#version 300 es\nlayout(location=0) out vec4 o0;\nlayout(location=1) out vec4 o1;\nvoid main(){o0=vec4(t);o1=vec4(1.0);}";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        assert!(!output.contains("#version 300 es"));
        assert!(!output.contains("layout(location="));
        assert!(output.contains("o0=vec4(iTime);o1=vec4(1.0);"));
        assert!(!output.contains("gl_FragColor"));
    }

    #[test]
    fn unrewrite_then_rewrite_round_trips_the_es300_fixture() {
        // twigl_300es.glsl is the classic-mode ES300 rewrite of the shared
        // source fixture (see twigl_300es_fixture_matches_the_classic_mode_es300_rewrite_of_the_shared_source_fixture
        // above) -- round-tripping it through unrewrite -> rewrite must
        // reproduce it exactly now that the ES300 header is actually reversed.
        let es300_fixture = include_str!("../../fixtures/twigl_300es.glsl").replace("\r\n", "\n");
        let trimmed = es300_fixture.trim_end();
        let roundtrip = rewrite_twigl_shader(
            &unrewrite_twigl_shader(trimmed, TwiglMode::Classic),
            TwiglMode::Classic,
            true,
        );
        assert_eq!(roundtrip.trim_end(), trimmed);
    }

    #[test]
    fn unrewrite_then_rewrite_mrt_round_trips_a_two_target_geek_shader() {
        let input = "#version 300 es\nlayout(location=0) out vec4 o0;\nlayout(location=1) out vec4 o1;\nvoid main(){o0=vec4(t,r,0.,1.);o1=vec4(m,0.,1.);}";
        let unrewritten = unrewrite_twigl_shader(input, TwiglMode::Geek);
        let roundtrip = rewrite_twigl_shader_mrt(&unrewritten, TwiglMode::Geek, 2);
        assert_eq!(roundtrip, input);
    }

    #[test]
    fn unrewrite_classic_reverses_every_uniform_including_backbuffer() {
        let input = "precision mediump float;\nuniform vec2 resolution;\nuniform float time;\nuniform vec2 mouse;\nuniform sampler2D backbuffer;\nvoid main(){vec2 uv=gl_FragCoord.xy/resolution.xy;vec4 bg=texture2D(backbuffer,uv);gl_FragColor=bg+vec4(uv,sin(time),1.0);}";
        // Not compared against fixtures/twigl_source.glsl (a plain-`main`-style
        // fixture shared by the *forward*-direction tests below, which is
        // still valid input there since normalize_mainimage_to_plain_main is
        // a no-op on already-plain-main text) -- unrewrite's actual output
        // convention is this app's Source-tab mainImage signature instead.
        let expected = "precision mediump float;\nuniform vec2 iResolution;\nuniform float iTime;\nuniform vec2 iMouse;\nuniform sampler2D iChannel0;\nvoid mainImage(out vec4 fragColor,in vec2 fragCoord){vec2 uv=fragCoord.xy/iResolution.xy;vec4 bg=texture2D(iChannel0,uv);fragColor=bg+vec4(uv,sin(iTime),1.0);}";
        assert_eq!(unrewrite_twigl_shader(input, TwiglMode::Classic), expected);
    }

    #[test]
    fn unrewrite_geekest_restores_the_void_main_wrapper_when_it_was_omitted() {
        let input = "gl_FragColor=vec4(1.0);";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        assert!(output.contains("void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(1.0);}"));
    }

    #[test]
    fn unrewrite_geekest_keeps_the_existing_wrapper_when_a_helper_function_is_present() {
        let input = "float helperFn(float x){return x*2.;}\nvoid main(){gl_FragColor=vec4(helperFn(t));}";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        // Must not be double-wrapped -- exactly one "void mainImage(" in the result.
        assert_eq!(output.matches("void mainImage(").count(), 1);
        assert!(output.contains("float helperFn(float x){return x*2.;}"));
        assert!(output.contains("iTime"));
    }

    #[test]
    fn unrewrite_geekest_reverses_fc_back_to_gl_fragcoord() {
        let input = "gl_FragColor=vec4(FC.xy,0.,1.);";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        // FC -> gl_FragCoord -> (rewrapped as the mainImage coordinate param) -> fragCoord.
        assert!(output.contains("fragCoord.xy"));
        assert!(!identifier_present(&output, "FC"));
        assert!(!identifier_present(&output, "gl_FragCoord"));
    }

    #[test]
    fn unrewrite_geek_mode_never_reverses_fc_since_geek_never_shortens_it() {
        // Geek mode's forward rewrite never touches gl_FragCoord (only
        // Geekest does), so a literal "FC" in Geek-mode input is just a
        // user identifier, not a shortened builtin, and must survive
        // untouched.
        let input = "float FC=1.;gl_FragColor=vec4(FC);";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geek);
        assert!(output.contains("float FC=1.;"));
    }

    #[test]
    fn unrewrite_only_declares_uniforms_that_are_actually_referenced() {
        // iResolution/iMouse/iTime/iFrame are never reconstructed here at all
        // (this app's own Source-tab compilation wrapper, ShaderRunner's
        // kFragmentPrefix, always declares them itself -- redeclaring them in
        // reconstructed Source would be a GLSL redefinition error). Only
        // iChannel0 needs reconstructing, since nothing else declares it.
        let input = "gl_FragColor=vec4(t);";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        assert!(!output.contains("uniform float iTime;"));
        assert!(!output.contains("iResolution"));
        assert!(!output.contains("iMouse"));
        assert!(!output.contains("uniform sampler2D iChannel0;"));
        assert!(output.contains("iTime"));

        let input_with_channel = "gl_FragColor=texture2D(b,vec2(t));";
        let output_with_channel = unrewrite_twigl_shader(input_with_channel, TwiglMode::Geekest);
        assert!(output_with_channel.contains("uniform sampler2D iChannel0;"));
    }

    #[test]
    fn unrewrite_geek_mode_does_not_reconstruct_scaffold_since_geek_never_omits_it() {
        // Only Geeker/Geekest omit precision/uniform on export; Geek keeps
        // them, so unrewriting Geek-mode input must not add a second
        // precision line on top of one already there, nor add one where
        // none existed in the (already-complete) input.
        let input = "void main(){gl_FragColor=vec4(t);}";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geek);
        assert_eq!(output, "void mainImage(out vec4 fragColor,in vec2 fragCoord){fragColor=vec4(iTime);}");
    }

    #[test]
    fn identifier_present_is_word_boundary_aware() {
        assert!(!identifier_present("float iTimeScale=1.;", "iTime"));
        assert!(identifier_present("float x=iTime;", "iTime"));
    }
    // Not a real GLSL parser (see roadmap_twigl.md Phase 45.2 for why a real
    // one is out of scope), but enough to have caught 42.1/42.2 immediately:
    // every ES300 export must never contain a deprecated texture call, and
    // every multi-output declaration line must carry a layout qualifier.

    fn assert_es300_output_is_lint_clean(output: &str) {
        assert!(
            !output.contains("texture2D") && !output.contains("textureCube(") && !output.contains("shadow2D"),
            "ES300 output still contains a deprecated GLSL ES 1.00 texture call: {output}"
        );
        for line in output.lines() {
            let trimmed = line.trim_start();
            if let Some(rest) = trimmed.strip_prefix("out vec4 ") {
                // "vec4" itself contains a digit, so the digit check must be
                // scoped to the *output variable name* (before the `;`), not
                // the line as a whole -- e.g. "out vec4 outColor;" (single
                // target, no digit suffix, no layout needed) versus
                // "out vec4 outColor0;" (MRT, digit suffix, layout required).
                let name = rest.trim_end_matches(';').trim();
                let ends_with_digit = name.chars().next_back().is_some_and(|c| c.is_ascii_digit());
                if ends_with_digit {
                    assert!(
                        trimmed.contains("layout("),
                        "multi-output ES300 declaration missing layout(location=N): {line}"
                    );
                }
            }
        }
    }

    #[test]
    fn every_bundled_es300_fixture_is_lint_clean() {
        let source = include_str!("../../fixtures/twigl_source.glsl").replace("\r\n", "\n");
        assert_es300_output_is_lint_clean(&rewrite_twigl_shader(&source, TwiglMode::Classic, true));
        assert_es300_output_is_lint_clean(&rewrite_twigl_shader(&source, TwiglMode::Geekest, true));
        assert_es300_output_is_lint_clean(&rewrite_twigl_shader_mrt(&source, TwiglMode::Classic, 2));
        assert_es300_output_is_lint_clean(&rewrite_twigl_shader_mrt(&source, TwiglMode::Geekest, 2));
    }

    #[test]
    fn the_committed_300es_fixture_itself_is_lint_clean() {
        let fixture = include_str!("../../fixtures/twigl_300es.glsl").replace("\r\n", "\n");
        assert_es300_output_is_lint_clean(&fixture);
    }
}

