// Phase 35 — Fabrice Neyret idiom library (rust-core catalogue slice only).
//
// Everything in this module is READ-ONLY reference/suggestion data and
// detection logic. Nothing here is wired into any always-on golf pass and
// nothing here is auto-applied, per the Phase 11 invariant restated in
// ROADMAP.md section 2: these are approximations/stylistic rewrites, not
// behavior-preserving transforms. Phase 35.4 (surfacing these as dismissible
// cards in the Golf Tips panel) is explicitly NOT part of this module — the
// panel itself (`golf.md` Phase 33) does not exist yet, so this module only
// ships the data + detection functions a future panel would call into.
//
// Sourced from Fabrice Neyret's public Shadertoy Unofficial catalogue
// conventions as a design reference only (Offline-First corollary): no
// network fetch, no scraped text, values reimplemented from first
// principles (basic trigonometry / the well-known fract(sin(dot(...)))
// hash family already used elsewhere in this codebase).

// ---------------------------------------------------------------------
// 35.1 — Short rotation-matrix constant catalogue
// ---------------------------------------------------------------------

/// One entry in the rotation-matrix constant table: an angle (in degrees,
/// for readability) paired with the shortest clean 1-2 decimal-digit
/// literals for its cosine/sine, matching the `mat2(cosA,sinA,-sinA,cosA)`
/// layout already used by the single example this catalogue extends.
pub struct RotationConstant {
    pub angle_degrees: f64,
    pub cos_literal: &'static str,
    pub sin_literal: &'static str,
}

const ROTATION_CONSTANTS: &[RotationConstant] = &[
    RotationConstant { angle_degrees: 15.0, cos_literal: ".97", sin_literal: ".26" },
    RotationConstant { angle_degrees: 22.5, cos_literal: ".92", sin_literal: ".38" },
    RotationConstant { angle_degrees: 30.0, cos_literal: ".87", sin_literal: ".5" },
    // The 3-4-5 triangle angle: the one existing example this table
    // extends. Unlike every other entry it is exact to the stated decimal
    // precision, not a rounded approximation, since .6/.8 satisfy
    // sin^2+cos^2=1 exactly.
    RotationConstant { angle_degrees: 36.869_897_645_844_02, cos_literal: ".8", sin_literal: ".6" },
    RotationConstant { angle_degrees: 45.0, cos_literal: ".71", sin_literal: ".71" },
    // The complementary 3-4-5 angle (90 minus the entry above), also exact.
    RotationConstant { angle_degrees: 53.130_102_354_155_98, cos_literal: ".6", sin_literal: ".8" },
    RotationConstant { angle_degrees: 60.0, cos_literal: ".5", sin_literal: ".87" },
    RotationConstant { angle_degrees: 67.5, cos_literal: ".38", sin_literal: ".92" },
    RotationConstant { angle_degrees: 75.0, cos_literal: ".26", sin_literal: ".97" },
    RotationConstant { angle_degrees: 90.0, cos_literal: "0.", sin_literal: "1." },
];

pub fn rotation_constant_catalogue() -> &'static [RotationConstant] {
    ROTATION_CONSTANTS
}

/// A detected `mat2(cos(a),sin(a),-sin(a),cos(a))`-shaped construction
/// (in any of the four sign/order permutations a hand-written rotation
/// matrix commonly appears in) whose angle argument is a compile-time
/// numeric constant close enough to a catalogue entry to be worth
/// suggesting as a shorter literal replacement.
#[derive(Debug, PartialEq)]
pub struct RotationSuggestion {
    pub matched_text: String,
    pub angle_degrees: f64,
    pub cos_literal: &'static str,
    pub sin_literal: &'static str,
    pub original_bytes: usize,
    pub suggested_bytes: usize,
}

/// Tolerance for matching a source shader's literal angle against a
/// catalogue entry, in degrees. Kept tight (quarter of a degree) so this
/// never suggests visibly wrong rotations, only "this angle already is
/// (near enough to) one of the clean-decimal angles, no need for a
/// runtime cos()/sin() call at all".
const ANGLE_MATCH_TOLERANCE_DEGREES: f64 = 0.25;

/// Evaluates a tiny compile-time-constant arithmetic subset of GLSL:
/// float literals, `+ - * /`, unary minus, parens, and the identifier
/// `PI`. Returns `None` for anything else (variables, function calls,
/// swizzles, ...), since only a provably constant angle can be matched
/// against the catalogue at all.
fn eval_constant_expr(expr: &str) -> Option<f64> {
    struct P<'a> {
        b: &'a [u8],
        i: usize,
    }
    impl<'a> P<'a> {
        fn skip_ws(&mut self) {
            while self.i < self.b.len() && self.b[self.i].is_ascii_whitespace() {
                self.i += 1;
            }
        }
        fn peek(&mut self) -> Option<u8> {
            self.skip_ws();
            self.b.get(self.i).copied()
        }
        fn expr(&mut self) -> Option<f64> {
            let mut v = self.term()?;
            loop {
                match self.peek() {
                    Some(b'+') => {
                        self.i += 1;
                        v += self.term()?;
                    }
                    Some(b'-') => {
                        self.i += 1;
                        v -= self.term()?;
                    }
                    _ => break,
                }
            }
            Some(v)
        }
        fn term(&mut self) -> Option<f64> {
            let mut v = self.unary()?;
            loop {
                match self.peek() {
                    Some(b'*') => {
                        self.i += 1;
                        v *= self.unary()?;
                    }
                    Some(b'/') => {
                        self.i += 1;
                        let d = self.unary()?;
                        if d == 0.0 {
                            return None;
                        }
                        v /= d;
                    }
                    _ => break,
                }
            }
            Some(v)
        }
        fn unary(&mut self) -> Option<f64> {
            if self.peek() == Some(b'-') {
                self.i += 1;
                return Some(-self.unary()?);
            }
            if self.peek() == Some(b'+') {
                self.i += 1;
                return self.unary();
            }
            self.atom()
        }
        fn atom(&mut self) -> Option<f64> {
            match self.peek()? {
                b'(' => {
                    self.i += 1;
                    let v = self.expr()?;
                    self.skip_ws();
                    if self.b.get(self.i) != Some(&b')') {
                        return None;
                    }
                    self.i += 1;
                    Some(v)
                }
                c if c.is_ascii_digit() || c == b'.' => {
                    let start = self.i;
                    while self.i < self.b.len()
                        && (self.b[self.i].is_ascii_digit() || self.b[self.i] == b'.')
                    {
                        self.i += 1;
                    }
                    std::str::from_utf8(&self.b[start..self.i]).ok()?.parse::<f64>().ok()
                }
                c if c.is_ascii_alphabetic() || c == b'_' => {
                    let start = self.i;
                    while self.i < self.b.len()
                        && (self.b[self.i].is_ascii_alphanumeric() || self.b[self.i] == b'_')
                    {
                        self.i += 1;
                    }
                    let ident = std::str::from_utf8(&self.b[start..self.i]).ok()?;
                    if ident == "PI" {
                        Some(std::f64::consts::PI)
                    } else {
                        None
                    }
                }
                _ => None,
            }
        }
    }
    let mut p = P { b: expr.as_bytes(), i: 0 };
    let v = p.expr()?;
    p.skip_ws();
    if p.i == p.b.len() {
        Some(v)
    } else {
        None
    }
}

/// Splits `mat2(`'s argument list on top-level commas (i.e. not inside a
/// nested `(`/`)` pair), mirroring the bracket-depth tracking already used
/// elsewhere in this codebase for argument-list splitting.
fn split_top_level_args(args: &str) -> Vec<&str> {
    let mut out = Vec::new();
    let mut depth = 0i32;
    let mut start = 0usize;
    for (idx, ch) in args.char_indices() {
        match ch {
            '(' => depth += 1,
            ')' => depth -= 1,
            ',' if depth == 0 => {
                out.push(args[start..idx].trim());
                start = idx + 1;
            }
            _ => {}
        }
    }
    out.push(args[start..].trim());
    out
}

/// Parses a single `mat2(...)` argument as an optionally-negated
/// `cos(EXPR)` or `sin(EXPR)` call. Returns `(is_cos, negated, expr)`.
fn parse_trig_arg(arg: &str) -> Option<(bool, bool, &str)> {
    let (negated, rest) = if let Some(r) = arg.strip_prefix('-') {
        (true, r.trim_start())
    } else {
        (false, arg)
    };
    let (is_cos, rest) = if let Some(r) = rest.strip_prefix("cos(") {
        (true, r)
    } else if let Some(r) = rest.strip_prefix("sin(") {
        (false, r)
    } else {
        return None;
    };
    let inner = rest.strip_suffix(')')?;
    Some((is_cos, negated, inner.trim()))
}

fn find_matching_paren(bytes: &[u8], open_idx: usize) -> Option<usize> {
    let mut depth = 0i32;
    for (offset, &b) in bytes[open_idx..].iter().enumerate() {
        match b {
            b'(' => depth += 1,
            b')' => {
                depth -= 1;
                if depth == 0 {
                    return Some(open_idx + offset);
                }
            }
            _ => {}
        }
    }
    None
}

fn nearest_catalogue_match(angle_degrees: f64) -> Option<&'static RotationConstant> {
    let normalized = angle_degrees.rem_euclid(360.0);
    ROTATION_CONSTANTS
        .iter()
        .map(|c| (c, (c.angle_degrees - normalized).abs()))
        .filter(|(_, diff)| *diff <= ANGLE_MATCH_TOLERANCE_DEGREES)
        .min_by(|(_, a), (_, b)| a.partial_cmp(b).unwrap())
        .map(|(c, _)| c)
}

/// Scans `source` for `mat2(...)` rotation-matrix constructions whose four
/// arguments are `cos(a)`/`sin(a)` (in either of the two sign/order
/// arrangements a hand-written rotation matrix normally appears in) with
/// an identical, compile-time-constant angle expression `a`, and returns
/// one suggestion per match whose angle lands within
/// [`ANGLE_MATCH_TOLERANCE_DEGREES`] of a catalogue entry.
///
/// This never modifies `source` — it only reports matches for an external
/// caller (a future Golf Tips panel, per Phase 35.4) to render as an
/// opt-in, click-to-apply suggestion.
pub fn suggest_rotation_matrix_constants(source: &str) -> Vec<RotationSuggestion> {
    let mut out = Vec::new();
    let bytes = source.as_bytes();
    let mut search_from = 0usize;
    while let Some(rel) = source[search_from..].find("mat2(") {
        let open = search_from + rel + "mat2".len();
        let Some(close) = find_matching_paren(bytes, open) else {
            break;
        };
        let matched_text = &source[search_from + rel..=close];
        let args_text = &source[open + 1..close];
        let args = split_top_level_args(args_text);
        if args.len() == 4 {
            if let (Some(a0), Some(a1), Some(a2), Some(a3)) = (
                parse_trig_arg(args[0]),
                parse_trig_arg(args[1]),
                parse_trig_arg(args[2]),
                parse_trig_arg(args[3]),
            ) {
                let same_expr = a0.2 == a1.2 && a1.2 == a2.2 && a2.2 == a3.2;
                let cos_sin_sin_cos = a0.0 && !a0.1 && !a1.0 && !a1.1 && !a2.0 && a2.1 && a3.0 && !a3.1;
                let cos_negsin_sin_cos = a0.0 && !a0.1 && !a1.0 && a1.1 && !a2.0 && !a2.1 && a3.0 && !a3.1;
                if same_expr && (cos_sin_sin_cos || cos_negsin_sin_cos) {
                    if let Some(angle_radians) = eval_constant_expr(a0.2) {
                        let angle_degrees = angle_radians.to_degrees();
                        if let Some(entry) = nearest_catalogue_match(angle_degrees) {
                            let suggested = format!(
                                "mat2({},{},-{},{})",
                                entry.cos_literal, entry.sin_literal, entry.sin_literal, entry.cos_literal
                            );
                            out.push(RotationSuggestion {
                                matched_text: matched_text.to_string(),
                                angle_degrees,
                                cos_literal: entry.cos_literal,
                                sin_literal: entry.sin_literal,
                                original_bytes: matched_text.len(),
                                suggested_bytes: suggested.len(),
                            });
                        }
                    }
                }
            }
        }
        search_from = close + 1;
    }
    out
}

// ---------------------------------------------------------------------
// 35.2 — Cheap-noise and cheap-hash one-liner catalogue
// ---------------------------------------------------------------------
//
// Distinct from and complementary to twigl's own `fsnoise`/`fsnoiseDigits`
// (Phase 34.4, `twigl.rs`): different dot-product constants and a wider
// signature spread (1D/2D/3D input, scalar and vector output), matching
// the variety of `fract(sin(dot(...)))`-family idioms actually seen
// across Shadertoy golf entries and cited on Shadertoy Unofficial, rather
// than a single canonical form. Reimplemented from first principles, not
// copied from any single source.

pub struct NeyretHashSnippet {
    pub name: &'static str,
    pub source: &'static str,
    pub description: &'static str,
}

const NEYRET_HASH_SNIPPETS: &[NeyretHashSnippet] = &[
    NeyretHashSnippet {
        name: "hash11",
        source: "float hash11(float p){return fract(sin(p*127.1)*43758.5453);}",
        description: "1D->1D fract-sin hash, single-multiply variant",
    },
    NeyretHashSnippet {
        name: "hash12",
        source: "float hash12(vec2 p){return fract(sin(dot(p,vec2(41.,289.)))*43758.5453);}",
        description: "2D->1D fract-sin-dot hash with small-integer dot constants",
    },
    NeyretHashSnippet {
        name: "hash21",
        source: "vec2 hash21(float p){return fract(sin(p+vec2(0.,52.7))*vec2(43758.5453,28001.8384));}",
        description: "1D->2D fract-sin hash, two independent channels from one input",
    },
    NeyretHashSnippet {
        name: "hash22",
        source: "vec2 hash22(vec2 p){return fract(sin(vec2(dot(p,vec2(41.,289.)),dot(p,vec2(127.1,311.7))))*43758.5453);}",
        description: "2D->2D fract-sin-dot hash, two independent dot products",
    },
    NeyretHashSnippet {
        name: "hash13",
        source: "float hash13(vec3 p){return fract(sin(dot(p,vec3(41.,289.,157.)))*43758.5453);}",
        description: "3D->1D fract-sin-dot hash, complements twigl's 2D-only fsnoise",
    },
];

pub fn neyret_hash_snippets() -> &'static [NeyretHashSnippet] {
    NEYRET_HASH_SNIPPETS
}

pub fn neyret_hash_snippet(name: &str) -> Option<&'static str> {
    NEYRET_HASH_SNIPPETS.iter().find(|s| s.name == name).map(|s| s.source)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rotation_catalogue_entries_are_within_tolerance_of_their_stated_angle() {
        for entry in rotation_constant_catalogue() {
            let cos_v: f64 = entry.cos_literal.parse().unwrap();
            let sin_v: f64 = entry.sin_literal.parse().unwrap();
            let expected_cos = entry.angle_degrees.to_radians().cos();
            let expected_sin = entry.angle_degrees.to_radians().sin();
            assert!((cos_v - expected_cos).abs() < 0.02, "{}", entry.angle_degrees);
            assert!((sin_v - expected_sin).abs() < 0.02, "{}", entry.angle_degrees);
        }
    }

    #[test]
    fn the_three_four_five_triangle_entries_are_exact() {
        let catalogue = rotation_constant_catalogue();
        let a = catalogue.iter().find(|c| c.cos_literal == ".8" && c.sin_literal == ".6").unwrap();
        let b = catalogue.iter().find(|c| c.cos_literal == ".6" && c.sin_literal == ".8").unwrap();
        assert!((a.angle_degrees - 36.869_897_645_844_02).abs() < 1e-9);
        assert!((b.angle_degrees - 53.130_102_354_155_98).abs() < 1e-9);
    }

    #[test]
    fn detects_cos_sin_negsin_cos_arrangement_with_a_numeric_literal_angle() {
        let source = "mat2 m=mat2(cos(.5236),sin(.5236),-sin(.5236),cos(.5236));";
        let hits = suggest_rotation_matrix_constants(source);
        assert_eq!(hits.len(), 1);
        assert!((hits[0].angle_degrees - 30.0).abs() < 0.25);
        assert_eq!(hits[0].cos_literal, ".87");
        assert_eq!(hits[0].sin_literal, ".5");
    }

    #[test]
    fn detects_cos_negsin_sin_cos_arrangement() {
        let source = "mat2(cos(.7854),-sin(.7854),sin(.7854),cos(.7854))";
        let hits = suggest_rotation_matrix_constants(source);
        assert_eq!(hits.len(), 1);
        assert!((hits[0].angle_degrees - 45.0).abs() < 0.25);
    }

    #[test]
    fn detects_angle_expressed_as_a_constant_expression_with_pi() {
        let source = "mat2(cos(PI/6.),sin(PI/6.),-sin(PI/6.),cos(PI/6.))";
        let hits = suggest_rotation_matrix_constants(source);
        assert_eq!(hits.len(), 1);
        assert!((hits[0].angle_degrees - 30.0).abs() < 0.25);
    }

    #[test]
    fn does_not_match_a_variable_angle_since_it_is_not_a_compile_time_constant() {
        let source = "mat2(cos(a),sin(a),-sin(a),cos(a))";
        assert!(suggest_rotation_matrix_constants(source).is_empty());
    }

    #[test]
    fn does_not_match_when_the_four_angle_expressions_are_not_identical() {
        let source = "mat2(cos(.5236),sin(.5),-sin(.5236),cos(.5236))";
        assert!(suggest_rotation_matrix_constants(source).is_empty());
    }

    #[test]
    fn does_not_match_an_angle_far_from_every_catalogue_entry() {
        let source = "mat2(cos(1.0),sin(1.0),-sin(1.0),cos(1.0))";
        assert!(suggest_rotation_matrix_constants(source).is_empty());
    }

    #[test]
    fn suggested_replacement_is_never_longer_than_the_original() {
        let source = "mat2(cos(.5236),sin(.5236),-sin(.5236),cos(.5236))";
        let hits = suggest_rotation_matrix_constants(source);
        assert_eq!(hits.len(), 1);
        assert!(hits[0].suggested_bytes < hits[0].original_bytes);
    }

    #[test]
    fn finds_multiple_independent_matches_in_the_same_source() {
        let source = "mat2(cos(.5236),sin(.5236),-sin(.5236),cos(.5236));mat2(cos(.7854),sin(.7854),-sin(.7854),cos(.7854));";
        let hits = suggest_rotation_matrix_constants(source);
        assert_eq!(hits.len(), 2);
    }

    #[test]
    fn every_hash_snippet_has_balanced_parentheses_and_braces() {
        for snippet in neyret_hash_snippets() {
            let mut paren_depth = 0i32;
            let mut brace_depth = 0i32;
            for c in snippet.source.chars() {
                match c {
                    '(' => paren_depth += 1,
                    ')' => paren_depth -= 1,
                    '{' => brace_depth += 1,
                    '}' => brace_depth -= 1,
                    _ => {}
                }
            }
            assert_eq!(paren_depth, 0, "{}", snippet.name);
            assert_eq!(brace_depth, 0, "{}", snippet.name);
        }
    }

    #[test]
    fn hash_snippet_lookup_returns_the_matching_source_and_none_for_unknown_names() {
        assert!(neyret_hash_snippet("hash12").unwrap().contains("fn hash12") == false);
        assert!(neyret_hash_snippet("hash12").unwrap().contains("hash12"));
        assert!(neyret_hash_snippet("does-not-exist").is_none());
    }

    #[test]
    fn hash_snippets_use_different_dot_constants_than_twigl_fsnoise() {
        let fsnoise = crate::twigl::twigl_snippet("fsnoise").unwrap();
        for snippet in neyret_hash_snippets() {
            assert_ne!(snippet.source, fsnoise);
        }
    }

    #[test]
    fn hash_snippet_names_match_the_documented_catalogue() {
        let names: Vec<&str> = neyret_hash_snippets().iter().map(|s| s.name).collect();
        assert_eq!(names, vec!["hash11", "hash12", "hash21", "hash22", "hash13"]);
    }
}
