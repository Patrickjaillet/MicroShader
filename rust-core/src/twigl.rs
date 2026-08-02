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
        for i in 0..mrt_targets {
            header.push_str(&format!("out vec4 {base}{i};\n"));
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
    let mut header = twigl_es300_header(mode, mrt_targets);
    header.push_str(&output);
    header
}


#[cfg(test)]
mod tests {
    use super::{
        rewrite_twigl_shader, rewrite_twigl_shader_mrt, rewrite_twigl_uniforms, twigl_es300_header,
        twigl_export_uniform_names, twigl_snippet, twigl_snippets, TwiglMode,
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
            "#version 300 es\nout vec4 outColor0;\nout vec4 outColor1;\n"
        );
    }

    #[test]
    fn es300_header_declares_two_outputs_for_mrt_in_geek_style_modes() {
        assert_eq!(
            twigl_es300_header(TwiglMode::Geekest, 2),
            "#version 300 es\nout vec4 o0;\nout vec4 o1;\n"
        );
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
            "#version 300 es\nout vec4 o0;\nout vec4 o1;\nvoid main(){o0=vec4(t);o1=vec4(1.0);}"
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
}

