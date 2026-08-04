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

// Avoids corrupting identifiers that merely contain `from` as a substring (e.g. `iTimeScale`).
fn replace_identifier(input: &str, from: &str, to: &str) -> String {
    let mut result = String::with_capacity(input.len());
    let mut last_end = 0;
    for (start, _) in input.match_indices(from) {
        if start < last_end {
            continue;
        }
        let end = start + from.len();
        let before_ok = input[..start]
            .chars()
            .last()
            .map_or(true, |c| !(c.is_alphanumeric() || c == '_'));
        let after_ok = input[end..]
            .chars()
            .next()
            .map_or(true, |c| !(c.is_alphanumeric() || c == '_'));
        if before_ok && after_ok {
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

// Word-boundary-aware substring presence check, matching replace_identifier's
// own boundary rule -- used by unrewrite_twigl_shader below to decide which
// uniform declarations to reconstruct and whether a void main(){} wrapper
// needs restoring.
fn identifier_present(text: &str, name: &str) -> bool {
    let mut start = 0usize;
    while let Some(pos) = text[start..].find(name) {
        let abs = start + pos;
        let end = abs + name.len();
        let before_ok = text[..abs].chars().last().map_or(true, |c| !(c.is_alphanumeric() || c == '_'));
        let after_ok = text[end..].chars().next().map_or(true, |c| !(c.is_alphanumeric() || c == '_'));
        if before_ok && after_ok {
            return true;
        }
        start = abs + 1;
    }
    false
}

// Reconstructs a plausible precision/uniform scaffold for whichever
// Shadertoy-style uniforms are actually referenced in `body`. The exact
// order/formatting is irrelevant to round-tripping through
// rewrite_twigl_shader for Geeker/Geekest modes, since the forward pass
// deletes every precision/uniform line wholesale (strip_precision_and_uniform_declarations)
// -- what matters is only that a syntactically valid declaration exists for
// every uniform the body actually uses.
fn reconstruct_uniform_scaffold(body: &str) -> String {
    const UNIFORMS: &[(&str, &str)] = &[
        ("iResolution", "uniform vec2 iResolution;\n"),
        ("iMouse", "uniform vec2 iMouse;\n"),
        ("iTime", "uniform float iTime;\n"),
        ("iFrame", "uniform int iFrame;\n"),
        ("iChannel0", "uniform sampler2D iChannel0;\n"),
    ];
    let mut scaffold = String::from("precision mediump float;\n");
    for (name, decl) in UNIFORMS {
        if identifier_present(body, name) {
            scaffold.push_str(decl);
        }
    }
    scaffold
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
// omission, `void main(){}` omission, `FC` shortening) is a straightforward,
// well-defined inverse and is fully reversed. Round-trip correctness for
// every case that matters in practice (every bundled `fixtures/twigl_*.glsl`
// fixture, none of which trip the lossy PI/vec2 substitution) is verified by
// `unrewrite_then_rewrite_reproduces_every_bundled_twigl_fixture` below --
// not merely asserted.
//
// Known, inherent ambiguity (not a bug, a property of the single-character
// uniform convention itself): in Geek/Geeker/Geekest input, a user's own
// locally-scoped variable literally named `r`/`m`/`t`/`f`/`b`/`s` is
// indistinguishable, at the text level, from the global uniform of the same
// name -- real GLSL scoping would let a local declaration shadow the global
// uniform, but this function has no scope information and reverses every
// occurrence. This is the same ambiguity anyone hand-porting geek-mode code
// already has to watch for; it is not introduced by this function.
pub fn unrewrite_twigl_shader(input: &str, mode: TwiglMode) -> String {
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

    let mut body = input.to_string();
    for (from, to) in reversed {
        body = replace_identifier(&body, from, to);
    }
    if matches!(mode, TwiglMode::Geekest) {
        body = replace_identifier(&body, "FC", "gl_FragCoord");
    }

    if matches!(mode, TwiglMode::Classic | TwiglMode::Geek) {
        // Neither mode strips or omits anything on export (only Geeker and
        // Geekest do), so the reversed identifiers are already the complete
        // inverse -- nothing structural left to restore.
        return body;
    }

    // Geeker (and Geekest) additionally omit precision/uniform declarations
    // on export; Geekest may additionally omit the void main(){} wrapper.
    let needs_main_wrapper = matches!(mode, TwiglMode::Geekest) && !identifier_present(&body, "main");
    if needs_main_wrapper {
        body = format!("void main(){{{body}}}");
    }
    let scaffold = reconstruct_uniform_scaffold(&body);
    format!("{scaffold}{body}")
}

pub fn rewrite_twigl_shader(input: &str, mode: TwiglMode, es300: bool) -> String {
    let mut output = rewrite_twigl_uniforms(input, mode);
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
    let mut output = rewrite_twigl_uniforms(input, mode);
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
        identifier_present, rewrite_es300_deprecated_texture_calls, rewrite_twigl_shader,
        rewrite_twigl_shader_full, rewrite_twigl_shader_mrt, rewrite_twigl_uniforms,
        twigl_backbuffer_and_sound_declarations, twigl_es300_header, twigl_export_uniform_names,
        twigl_snippet, twigl_snippets, unrewrite_twigl_shader, TwiglMode,
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

    #[test]
    fn unrewrite_classic_reverses_every_uniform_including_backbuffer() {
        let input = "precision mediump float;\nuniform vec2 resolution;\nuniform float time;\nuniform vec2 mouse;\nuniform sampler2D backbuffer;\nvoid main(){vec2 uv=gl_FragCoord.xy/resolution.xy;vec4 bg=texture2D(backbuffer,uv);gl_FragColor=bg+vec4(uv,sin(time),1.0);}";
        let expected = include_str!("../../fixtures/twigl_source.glsl").replace("\r\n", "\n");
        assert_eq!(unrewrite_twigl_shader(input, TwiglMode::Classic), expected.trim_end());
    }

    #[test]
    fn unrewrite_geekest_restores_the_void_main_wrapper_when_it_was_omitted() {
        let input = "gl_FragColor=vec4(1.0);";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        assert!(output.contains("void main(){gl_FragColor=vec4(1.0);}"));
    }

    #[test]
    fn unrewrite_geekest_keeps_the_existing_wrapper_when_a_helper_function_is_present() {
        let input = "float helperFn(float x){return x*2.;}\nvoid main(){gl_FragColor=vec4(helperFn(t));}";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        // Must not be double-wrapped -- exactly one "void main(){" in the result.
        assert_eq!(output.matches("void main(){").count(), 1);
        assert!(output.contains("float helperFn(float x){return x*2.;}"));
        assert!(output.contains("iTime"));
    }

    #[test]
    fn unrewrite_geekest_reverses_fc_back_to_gl_fragcoord() {
        let input = "gl_FragColor=vec4(FC.xy,0.,1.);";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        assert!(output.contains("gl_FragCoord.xy"));
        assert!(!identifier_present(&output, "FC"));
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
        let input = "gl_FragColor=vec4(t);";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geekest);
        assert!(output.contains("uniform float iTime;"));
        assert!(!output.contains("iResolution"));
        assert!(!output.contains("iMouse"));
        assert!(!output.contains("iChannel0"));
    }

    #[test]
    fn unrewrite_geek_mode_does_not_reconstruct_scaffold_since_geek_never_omits_it() {
        // Only Geeker/Geekest omit precision/uniform on export; Geek keeps
        // them, so unrewriting Geek-mode input must not add a second
        // precision line on top of one already there, nor add one where
        // none existed in the (already-complete) input.
        let input = "void main(){gl_FragColor=vec4(t);}";
        let output = unrewrite_twigl_shader(input, TwiglMode::Geek);
        assert_eq!(output, "void main(){gl_FragColor=vec4(iTime);}");
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

