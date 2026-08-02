use crate::aggressive::Item;
use crate::lexer::Tok;

/// GLSL accepts three interchangeable swizzle-letter sets for vector
/// field access — `.xyzw`, `.rgba`, `.stpq` — all three already valid,
/// unrestricted Shadertoy GLSL. Mirrors `Shader Minifier`'s
/// `--field-names` flag (golf.md Phase 29.2).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SwizzleAlphabet {
    /// Try all three alphabets against the DEFLATE estimator and keep
    /// whichever compresses smallest.
    Auto,
    Xyzw,
    Rgba,
    Stpq,
}

impl Default for SwizzleAlphabet {
    fn default() -> Self {
        SwizzleAlphabet::Auto
    }
}

const XYZW: [char; 4] = ['x', 'y', 'z', 'w'];
const RGBA: [char; 4] = ['r', 'g', 'b', 'a'];
const STPQ: [char; 4] = ['s', 't', 'p', 'q'];
const ALL_ALPHABETS: [[char; 4]; 3] = [XYZW, RGBA, STPQ];

fn alphabet_chars(a: SwizzleAlphabet) -> Option<[char; 4]> {
    match a {
        SwizzleAlphabet::Xyzw => Some(XYZW),
        SwizzleAlphabet::Rgba => Some(RGBA),
        SwizzleAlphabet::Stpq => Some(STPQ),
        SwizzleAlphabet::Auto => None,
    }
}

/// An identifier is treated as swizzle text only when every one of its
/// (1-4) characters belongs to exactly one of the three canonical
/// alphabets — this never touches struct member access (reuses the
/// caller's `struct_body_ranges`/`strictly_inside_any` scope-safety
/// machinery, see golfer.rs), except for the same known, accepted edge
/// case `Shader Minifier`'s own `--field-names` flag also accepts: a
/// struct field literally named e.g. `x`, `rgb`, or `stp` is
/// indistinguishable from a real swizzle by spelling alone.
pub fn detect_source_alphabet(text: &str) -> Option<[char; 4]> {
    let len = text.chars().count();
    if len == 0 || len > 4 {
        return None;
    }
    for alpha in ALL_ALPHABETS {
        if text.chars().all(|c| alpha.contains(&c)) {
            return Some(alpha);
        }
    }
    None
}

fn recolor_text(text: &str, from: [char; 4], to: [char; 4]) -> String {
    text.chars()
        .map(|c| {
            let pos = from.iter().position(|&x| x == c).unwrap_or(0);
            to[pos]
        })
        .collect()
}

/// Every `Ident` item immediately preceded by a `.` `Punct` item whose
/// text is pure single-alphabet swizzle text, excluding any index that
/// falls strictly inside one of the caller-supplied struct-body ranges.
fn swizzle_item_indices(items: &[Item], struct_bodies: &[(usize, usize)]) -> Vec<usize> {
    let mut out = Vec::new();
    for i in 1..items.len() {
        if matches!(items[i - 1].tok, Tok::Punct('.'))
            && matches!(items[i].tok, Tok::Ident(_))
            && detect_source_alphabet(&items[i].text).is_some()
            && !struct_bodies.iter().any(|(open, close)| i > *open && i < *close)
        {
            out.push(i);
        }
    }
    out
}

/// Recolors every detected swizzle access to `to`, leaving anything
/// already in that alphabet untouched. Never emits Shadertoy-
/// incompatible syntax: `.xyzw`/`.rgba`/`.stpq` are all standard,
/// unrestricted GLSL swizzle sets, so this never changes shader output
/// (the Phase 11 invariant), only which of the three equivalent letter
/// sets is used.
pub fn recolor_swizzles_fixed(
    mut items: Vec<Item>,
    alphabet: SwizzleAlphabet,
    struct_bodies: &[(usize, usize)],
    count: &mut usize,
) -> Vec<Item> {
    let to = match alphabet_chars(alphabet) {
        Some(a) => a,
        None => return items,
    };
    for idx in swizzle_item_indices(&items, struct_bodies) {
        if let Some(from) = detect_source_alphabet(&items[idx].text) {
            if from != to {
                items[idx].text = recolor_text(&items[idx].text, from, to);
                *count += 1;
            }
        }
    }
    items
}

/// `Auto` mode: try all three alphabets against the caller-supplied
/// DEFLATE estimator (`layout_fn` + `estimate_fn`, kept as closures so
/// this module never needs to depend on `golfer`'s `layout()` or
/// `budget::estimate_budget` directly) and keep whichever compresses
/// smallest, deterministically preferring the current/no-op choice on
/// an exact tie (golf.md Phase 29.2 — same "no randomized search"
/// determinism requirement as Phase 32.1's pass-order search).
pub fn apply_swizzle_alphabet(
    items: Vec<Item>,
    alphabet: SwizzleAlphabet,
    struct_bodies: &[(usize, usize)],
    count: &mut usize,
    layout_fn: impl Fn(&[Item]) -> String,
    estimate_fn: impl Fn(&str) -> usize,
) -> Vec<Item> {
    match alphabet {
        SwizzleAlphabet::Auto => {
            let mut best = items.clone();
            let mut best_size = estimate_fn(&layout_fn(&items));
            let mut best_count = 0usize;
            for alpha in [SwizzleAlphabet::Xyzw, SwizzleAlphabet::Rgba, SwizzleAlphabet::Stpq] {
                let mut c = 0usize;
                let candidate = recolor_swizzles_fixed(items.clone(), alpha, struct_bodies, &mut c);
                let size = estimate_fn(&layout_fn(&candidate));
                if size < best_size {
                    best_size = size;
                    best = candidate;
                    best_count = c;
                }
            }
            *count += best_count;
            best
        }
        fixed => recolor_swizzles_fixed(items, fixed, struct_bodies, count),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn item(text: &str, tok: Tok) -> Item {
        Item {
            tok,
            text: text.to_string(),
            space_before: false,
        }
    }

    fn dot() -> Item {
        item(".", Tok::Punct('.'))
    }

    #[test]
    fn detects_all_three_alphabets_and_rejects_mixed_or_oversized_text() {
        assert_eq!(detect_source_alphabet("xyz"), Some(XYZW));
        assert_eq!(detect_source_alphabet("rgba"), Some(RGBA));
        assert_eq!(detect_source_alphabet("stp"), Some(STPQ));
        assert_eq!(detect_source_alphabet("xr"), None);
        assert_eq!(detect_source_alphabet("xyzwx"), None);
        assert_eq!(detect_source_alphabet(""), None);
    }

    #[test]
    fn recolors_xyzw_to_rgba_preserving_position_mapping() {
        let items = vec![
            item("p", Tok::Ident("p".to_string())),
            dot(),
            item("xyz", Tok::Ident("xyz".to_string())),
        ];
        let mut count = 0;
        let out = recolor_swizzles_fixed(items, SwizzleAlphabet::Rgba, &[], &mut count);
        assert_eq!(out[2].text, "rgb");
        assert_eq!(count, 1);
    }

    #[test]
    fn leaves_already_matching_alphabet_untouched() {
        let items = vec![
            item("p", Tok::Ident("p".to_string())),
            dot(),
            item("rgb", Tok::Ident("rgb".to_string())),
        ];
        let mut count = 0;
        let out = recolor_swizzles_fixed(items, SwizzleAlphabet::Rgba, &[], &mut count);
        assert_eq!(out[2].text, "rgb");
        assert_eq!(count, 0);
    }

    #[test]
    fn never_recolors_a_struct_body_member_declaration() {
        // Index 2 ("xyz") sits strictly inside the struct body range
        // (0, 3) supplied by the caller, so it must be left alone even
        // though its spelling is pure swizzle text.
        let items = vec![
            item("{", Tok::Punct('{')),
            dot(),
            item("xyz", Tok::Ident("xyz".to_string())),
            item("}", Tok::Punct('}')),
        ];
        let mut count = 0;
        let out = recolor_swizzles_fixed(items, SwizzleAlphabet::Rgba, &[(0, 3)], &mut count);
        assert_eq!(out[2].text, "xyz");
        assert_eq!(count, 0);
    }

    #[test]
    fn auto_mode_picks_smaller_candidate_and_falls_back_on_no_improvement() {
        let items = vec![
            item("p", Tok::Ident("p".to_string())),
            dot(),
            item("xyz", Tok::Ident("xyz".to_string())),
        ];
        let mut count = 0;
        // Trivial estimator: fewer bytes always wins; here every
        // candidate renders the same length, so `Auto` must keep the
        // original (no-op) on the tie.
        let out = apply_swizzle_alphabet(
            items,
            SwizzleAlphabet::Auto,
            &[],
            &mut count,
            |its| its.iter().map(|i| i.text.clone()).collect::<Vec<_>>().join(""),
            |code| code.len(),
        );
        assert_eq!(out[2].text, "xyz");
        assert_eq!(count, 0);
    }

    #[test]
    fn swizzle_alphabet_fixture_recolors_to_rgba_exactly_as_hand_verified_in_the_wgl_equivalence_test() {
        // Golden-master check for `kSwizzleAlphabetRgbaSource` in
        // `tests/wgl_equivalence_test.cpp` (golf.md Phase 29.2): builds real
        // `Item`s from the actual fixture via the crate's own lexer (not a
        // hand-rolled scanner) and asserts every `.xyzw`-alphabet swizzle
        // recolors to `.rgba` in the exact order the C++ equivalence test's
        // hand-written constant assumes, so the two files can never
        // silently drift out of sync.
        let source = include_str!("../../fixtures/swizzle_alphabet.glsl");
        let spaced = crate::lexer::tokenize_spaced(source);
        let items: Vec<Item> = spaced
            .into_iter()
            .map(|(tok, space_before)| {
                let text = match &tok {
                    Tok::Preproc(s) => s.clone(),
                    Tok::Ident(s) => s.clone(),
                    Tok::Number(s) => s.clone(),
                    Tok::Punct(c) => c.to_string(),
                };
                Item { tok, text, space_before }
            })
            .collect();

        let mut count = 0;
        let out = recolor_swizzles_fixed(items, SwizzleAlphabet::Rgba, &[], &mut count);
        let recolored_swizzles: Vec<&str> = swizzle_item_indices(&out, &[])
            .into_iter()
            .map(|i| out[i].text.as_str())
            .collect();

        assert_eq!(
            recolored_swizzles,
            vec!["rg", "rg", "r", "g", "r", "g", "rgb", "rg", "gr", "rg", "b"]
        );
        assert_eq!(count, 11);
    }
}
