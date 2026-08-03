// ROADMAP.md Phase 36 -- Inigo Quilez idiom catalogue (rust-core slice
// only). Same rule as Phase 35's `neyret.rs`: read-only reference/snippet
// data, nothing here is detected in or auto-applied to user source. Design
// reference only, per the Offline-First corollary (`ROADMAP.md` section 2
// / `golf.md` section 2) -- sourced from `iquilezles.org`'s widely
// published, community-standard GLSL idioms as a design reference, never
// fetched, scraped, or linked at build or run time. Numeric constants
// (SDF formulas, palette coefficients) are reproduced since they are not
// copyrightable prose and are already ubiquitous, freely reused across the
// Shadertoy/demoscene community exactly as this catalogue documents; no
// article prose is reproduced anywhere in this module.

// ---------------------------------------------------------------------
// 36.1 -- SDF primitive and operator compaction catalogue
// ---------------------------------------------------------------------

pub struct IqSnippet {
    pub name: &'static str,
    pub source: &'static str,
    pub description: &'static str,
}

const IQ_SDF_SNIPPETS: &[IqSnippet] = &[
    IqSnippet {
        name: "sdSphere",
        source: "float sdSphere(vec3 p,float r){return length(p)-r;}",
        description: "Signed distance to a sphere of radius r centered at the origin.",
    },
    IqSnippet {
        name: "sdBox",
        source: "float sdBox(vec3 p,vec3 b){vec3 q=abs(p)-b;return length(max(q,0.))+min(max(q.x,max(q.y,q.z)),0.);}",
        description: "Signed distance to an axis-aligned box with half-extents b.",
    },
    IqSnippet {
        name: "sdPlane",
        source: "float sdPlane(vec3 p,vec3 n,float h){return dot(p,n)+h;}",
        description: "Signed distance to a plane with unit normal n, offset h from the origin.",
    },
    IqSnippet {
        name: "sdTorus",
        source: "float sdTorus(vec3 p,vec2 t){vec2 q=vec2(length(p.xz)-t.x,p.y);return length(q)-t.y;}",
        description: "Signed distance to a torus: t.x is the ring radius, t.y is the tube radius.",
    },
    IqSnippet {
        name: "sdCapsule",
        source: "float sdCapsule(vec3 p,vec3 a,vec3 b,float r){vec3 pa=p-a,ba=b-a;float h=clamp(dot(pa,ba)/dot(ba,ba),0.,1.);return length(pa-ba*h)-r;}",
        description: "Signed distance to a capsule (rounded cylinder) between points a and b, radius r.",
    },
    IqSnippet {
        name: "opUnion",
        source: "float opUnion(float d1,float d2){return min(d1,d2);}",
        description: "Boolean union of two SDFs.",
    },
    IqSnippet {
        name: "opSubtraction",
        source: "float opSubtraction(float d1,float d2){return max(-d1,d2);}",
        description: "Boolean subtraction of SDF d1 from d2.",
    },
    IqSnippet {
        name: "opIntersection",
        source: "float opIntersection(float d1,float d2){return max(d1,d2);}",
        description: "Boolean intersection of two SDFs.",
    },
    IqSnippet {
        name: "smin (polynomial smooth-min)",
        source: "float smin(float a,float b,float k){float h=clamp(.5+.5*(b-a)/k,0.,1.);return mix(b,a,h)-k*h*(1.-h);}",
        description: "k-parameterized quadratic polynomial smooth-min: a smooth blend between two SDFs, shorter than a naive mix(...,clamp(...)) expansion.",
    },
    IqSnippet {
        name: "smax (polynomial smooth-max)",
        source: "float smax(float a,float b,float k){return -smin(-a,-b,k);}",
        description: "Polynomial smooth-max, defined directly in terms of smin above.",
    },
];

pub fn iq_sdf_snippets() -> &'static [IqSnippet] {
    IQ_SDF_SNIPPETS
}

// ---------------------------------------------------------------------
// 36.2 -- Hash and noise one-liner catalogue (iq's documented style)
// ---------------------------------------------------------------------
//
// Offered alongside, not replacing, Phase 35.2's Neyret-style hashes and
// Phase 34.4's twigl fsnoise/snoise*D set -- a different dot-product
// constant lineage, test-covered below to confirm the catalogues stay
// genuinely distinct.

const IQ_HASH_SNIPPETS: &[IqSnippet] = &[
    IqSnippet {
        name: "hash11",
        source: "float hash11(float p){p=fract(p*.1031);p*=p+33.33;p*=p+p;return fract(p);}",
        description: "1D->1D hash, iq's fract-multiply-self style (distinct from the Neyret fract-sin lineage).",
    },
    IqSnippet {
        name: "hash12",
        source: "float hash12(vec2 p){vec3 p3=fract(vec3(p.xyx)*.1031);p3+=dot(p3,p3.yzx+33.33);return fract((p3.x+p3.y)*p3.z);}",
        description: "2D->1D hash, iq's fract-multiply-self style.",
    },
    IqSnippet {
        name: "hash21",
        source: "vec2 hash21(float p){vec3 p3=fract(vec3(p)*vec3(.1031,.1030,.0973));p3+=dot(p3,p3.yzx+33.33);return fract((p3.xx+p3.yz)*p3.zy);}",
        description: "1D->2D hash, iq's fract-multiply-self style.",
    },
    IqSnippet {
        name: "hash22",
        source: "vec2 hash22(vec2 p){vec3 p3=fract(vec3(p.xyx)*vec3(.1031,.1030,.0973));p3+=dot(p3,p3.yzx+33.33);return fract((p3.xx+p3.yz)*p3.zy);}",
        description: "2D->2D hash, iq's fract-multiply-self style.",
    },
    IqSnippet {
        name: "hash33",
        source: "vec3 hash33(vec3 p3){p3=fract(p3*vec3(.1031,.1030,.0973));p3+=dot(p3,p3.yxz+33.33);return fract((p3.xxy+p3.yxx)*p3.zyx);}",
        description: "3D->3D hash, iq's fract-multiply-self style -- complements Neyret's 3D->1D hash13.",
    },
];

pub fn iq_hash_snippets() -> &'static [IqSnippet] {
    IQ_HASH_SNIPPETS
}

// ---------------------------------------------------------------------
// 36.3 -- Cosine palette generator
// ---------------------------------------------------------------------

pub const IQ_PALETTE_FUNCTION: &str =
    "vec3 palette(float t,vec3 a,vec3 b,vec3 c,vec3 d){return a+b*cos(6.28318*(c*t+d));}";

pub struct IqPalettePreset {
    pub name: &'static str,
    pub call_site: &'static str,
    pub description: &'static str,
}

const IQ_PALETTE_PRESETS: &[IqPalettePreset] = &[
    IqPalettePreset {
        name: "rainbow",
        call_site: "vec3 col=palette(t,vec3(.5,.5,.5),vec3(.5,.5,.5),vec3(1.,1.,1.),vec3(0.,.33,.67));",
        description: "Even hue cycling through the full rainbow as t goes from 0 to 1.",
    },
    IqPalettePreset {
        name: "warm",
        call_site: "vec3 col=palette(t,vec3(.5,.5,.5),vec3(.5,.5,.5),vec3(1.,.7,.4),vec3(0.,.15,.2));",
        description: "Warm reds/oranges/yellows cycle.",
    },
    IqPalettePreset {
        name: "cool blue-green",
        call_site: "vec3 col=palette(t,vec3(.5,.5,.5),vec3(.5,.5,.5),vec3(1.,1.,.5),vec3(.8,.9,.3));",
        description: "Cool blue-to-green cycle.",
    },
    IqPalettePreset {
        name: "high-contrast red-cyan",
        call_site: "vec3 col=palette(t,vec3(.5,.5,.5),vec3(.5,.5,.5),vec3(2.,1.,0.),vec3(.5,.2,.25));",
        description: "High-contrast cycling between red and cyan.",
    },
];

pub fn iq_palette_presets() -> &'static [IqPalettePreset] {
    IQ_PALETTE_PRESETS
}

// ---------------------------------------------------------------------
// 36.4 -- Tonemap / gamma one-liner catalogue
// ---------------------------------------------------------------------

const IQ_TONEMAP_SNIPPETS: &[IqSnippet] = &[
    IqSnippet {
        name: "gamma 2.2 correction",
        source: "col=pow(col,vec3(1./2.2));",
        description: "Short linear-to-sRGB-ish gamma correction, applied at the very end of mainImage.",
    },
    IqSnippet {
        name: "ACES-approximation tonemap",
        source: "col=clamp((col*(2.51*col+.03))/(col*(2.43*col+.59)+.14),0.,1.);",
        description: "Compact single-expression approximation of the ACES filmic tonemap curve.",
    },
];

pub fn iq_tonemap_snippets() -> &'static [IqSnippet] {
    IQ_TONEMAP_SNIPPETS
}

#[cfg(test)]
mod tests {
    use super::*;

    fn balanced(text: &str) -> bool {
        let mut paren_depth = 0i32;
        let mut brace_depth = 0i32;
        for c in text.chars() {
            match c {
                '(' => paren_depth += 1,
                ')' => paren_depth -= 1,
                '{' => brace_depth += 1,
                '}' => brace_depth -= 1,
                _ => {}
            }
        }
        paren_depth == 0 && brace_depth == 0
    }

    #[test]
    fn every_sdf_snippet_has_balanced_parentheses_and_braces() {
        for snippet in iq_sdf_snippets() {
            assert!(balanced(snippet.source), "{}", snippet.name);
        }
    }

    #[test]
    fn sdf_snippet_names_match_the_documented_catalogue() {
        let names: Vec<&str> = iq_sdf_snippets().iter().map(|s| s.name).collect();
        assert_eq!(
            names,
            vec![
                "sdSphere", "sdBox", "sdPlane", "sdTorus", "sdCapsule", "opUnion",
                "opSubtraction", "opIntersection", "smin (polynomial smooth-min)",
                "smax (polynomial smooth-max)",
            ]
        );
    }

    #[test]
    fn every_hash_snippet_has_balanced_parentheses_and_braces() {
        for snippet in iq_hash_snippets() {
            assert!(balanced(snippet.source), "{}", snippet.name);
        }
    }

    #[test]
    fn hash_snippet_names_match_the_documented_catalogue() {
        let names: Vec<&str> = iq_hash_snippets().iter().map(|s| s.name).collect();
        assert_eq!(names, vec!["hash11", "hash12", "hash21", "hash22", "hash33"]);
    }

    #[test]
    fn iq_hash_snippets_use_different_source_than_neyret_hash_snippets() {
        for iq_snippet in iq_hash_snippets() {
            for neyret_snippet in crate::neyret_hash_snippets() {
                if iq_snippet.name == neyret_snippet.name {
                    assert_ne!(
                        iq_snippet.source, neyret_snippet.source,
                        "{} should differ between the iq and Neyret lineages",
                        iq_snippet.name
                    );
                }
            }
        }
    }

    #[test]
    fn iq_hash_snippets_use_different_source_than_twigl_fsnoise() {
        let fsnoise = crate::twigl::twigl_snippet("fsnoise").unwrap();
        for snippet in iq_hash_snippets() {
            assert_ne!(snippet.source, fsnoise);
        }
    }

    #[test]
    fn palette_function_is_balanced_and_every_preset_call_site_is_balanced() {
        assert!(balanced(IQ_PALETTE_FUNCTION));
        for preset in iq_palette_presets() {
            assert!(balanced(preset.call_site), "{}", preset.name);
            assert!(preset.call_site.contains("palette("), "{}", preset.name);
        }
    }

    #[test]
    fn palette_preset_names_match_the_documented_catalogue() {
        let names: Vec<&str> = iq_palette_presets().iter().map(|p| p.name).collect();
        assert_eq!(names, vec!["rainbow", "warm", "cool blue-green", "high-contrast red-cyan"]);
    }

    #[test]
    fn every_tonemap_snippet_has_balanced_parentheses() {
        for snippet in iq_tonemap_snippets() {
            assert!(balanced(snippet.source), "{}", snippet.name);
        }
    }

    #[test]
    fn tonemap_snippet_names_match_the_documented_catalogue() {
        let names: Vec<&str> = iq_tonemap_snippets().iter().map(|s| s.name).collect();
        assert_eq!(names, vec!["gamma 2.2 correction", "ACES-approximation tonemap"]);
    }
}
