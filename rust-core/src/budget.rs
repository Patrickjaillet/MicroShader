use std::collections::HashMap;

const WINDOW_SIZE: usize = 32768;
const MIN_MATCH: usize = 3;
const MAX_MATCH: usize = 258;
const MAX_CHAIN: usize = 32;

struct LzToken {
    length: usize,
    distance: usize,
}

enum Symbol {
    Literal(u8),
    Match(LzToken),
}

fn hash3(bytes: &[u8], pos: usize) -> u32 {
    let a = bytes[pos] as u32;
    let b = bytes[pos + 1] as u32;
    let c = bytes[pos + 2] as u32;
    (a.wrapping_mul(506832829) ^ b.wrapping_mul(2654435761) ^ c.wrapping_mul(40503))
        & 0x7FFF_FFFF
}

fn find_longest_match(
    bytes: &[u8],
    pos: usize,
    chains: &HashMap<u32, Vec<usize>>,
) -> Option<LzToken> {
    if pos + MIN_MATCH > bytes.len() {
        return None;
    }
    let key = hash3(bytes, pos);
    let candidates = chains.get(&key)?;
    let window_start = pos.saturating_sub(WINDOW_SIZE);
    let max_len = (bytes.len() - pos).min(MAX_MATCH);
    let mut best_length = 0usize;
    let mut best_distance = 0usize;
    let mut examined = 0usize;
    for &candidate in candidates.iter().rev() {
        if candidate < window_start || candidate >= pos {
            continue;
        }
        examined += 1;
        if examined > MAX_CHAIN {
            break;
        }
        let mut length = 0usize;
        while length < max_len && bytes[candidate + length] == bytes[pos + length] {
            length += 1;
        }
        if length >= MIN_MATCH && length > best_length {
            best_length = length;
            best_distance = pos - candidate;
            if best_length == max_len {
                break;
            }
        }
    }
    if best_length >= MIN_MATCH {
        Some(LzToken {
            length: best_length,
            distance: best_distance,
        })
    } else {
        None
    }
}

fn lz77_parse(bytes: &[u8]) -> Vec<Symbol> {
    let mut symbols = Vec::new();
    let mut chains: HashMap<u32, Vec<usize>> = HashMap::new();
    let mut pos = 0usize;
    while pos < bytes.len() {
        let matched = find_longest_match(bytes, pos, &chains);
        match matched {
            Some(token) => {
                let end = pos + token.length;
                let insert_end = end.min(bytes.len().saturating_sub(MIN_MATCH - 1));
                let mut i = pos;
                while i < insert_end {
                    let key = hash3(bytes, i);
                    chains.entry(key).or_default().push(i);
                    i += 1;
                }
                symbols.push(Symbol::Match(token));
                pos = end;
            }
            None => {
                if pos + MIN_MATCH <= bytes.len() {
                    let key = hash3(bytes, pos);
                    chains.entry(key).or_default().push(pos);
                }
                symbols.push(Symbol::Literal(bytes[pos]));
                pos += 1;
            }
        }
    }
    symbols
}

fn length_extra_bits(length: usize) -> u32 {
    match length {
        3..=10 => 0,
        11..=18 => 1,
        19..=34 => 2,
        35..=66 => 3,
        67..=130 => 4,
        131..=257 => 5,
        258 => 0,
        _ => 0,
    }
}

fn length_symbol_bits(length: usize) -> u32 {
    if length <= 114 {
        7
    } else {
        8
    }
}

fn distance_extra_bits(distance: usize) -> u32 {
    if distance <= 4 {
        0
    } else {
        let mut extra = 1u32;
        let mut lo = 5usize;
        let mut span = 2usize;
        loop {
            let hi = lo + span * 2 - 1;
            if distance <= hi {
                return extra;
            }
            lo = hi + 1;
            span *= 2;
            extra += 1;
        }
    }
}

fn literal_bits(byte: u8) -> u32 {
    if byte < 144 {
        8
    } else {
        9
    }
}

pub fn estimate_deflate_bytes(input: &[u8]) -> usize {
    if input.is_empty() {
        return 2;
    }
    let symbols = lz77_parse(input);
    let mut total_bits: u64 = 3;
    for symbol in &symbols {
        match symbol {
            Symbol::Literal(byte) => {
                total_bits += literal_bits(*byte) as u64;
            }
            Symbol::Match(token) => {
                total_bits += length_symbol_bits(token.length) as u64;
                total_bits += length_extra_bits(token.length) as u64;
                total_bits += 5;
                total_bits += distance_extra_bits(token.distance) as u64;
            }
        }
    }
    total_bits += 7;
    ((total_bits + 7) / 8) as usize
}

pub struct BudgetResult {
    pub raw_bytes: usize,
    pub deflate_bytes: usize,
}

pub fn estimate_budget(source: &str) -> BudgetResult {
    let bytes = source.as_bytes();
    BudgetResult {
        raw_bytes: bytes.len(),
        deflate_bytes: estimate_deflate_bytes(bytes),
    }
}

pub struct BudgetPreset {
    pub name: &'static str,
    pub raw_limit: Option<usize>,
    pub deflate_limit: Option<usize>,
}

pub fn presets() -> &'static [BudgetPreset] {
    &[
        BudgetPreset {
            name: "Shadertoy",
            raw_limit: Some(65536),
            deflate_limit: None,
        },
        BudgetPreset {
            name: "X/Twitter shader",
            raw_limit: Some(280),
            deflate_limit: None,
        },
        BudgetPreset {
            name: "JS13K-style 13KB",
            raw_limit: None,
            deflate_limit: Some(13312),
        },
        BudgetPreset {
            name: "4KB intro",
            raw_limit: None,
            deflate_limit: Some(4096),
        },
        BudgetPreset {
            name: "8KB intro",
            raw_limit: None,
            deflate_limit: Some(8192),
        },
        BudgetPreset {
            name: "64KB intro",
            raw_limit: None,
            deflate_limit: Some(65536),
        },
    ]
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn empty_input_has_minimal_size() {
        assert_eq!(estimate_deflate_bytes(b""), 2);
    }

    #[test]
    fn repeated_content_compresses_smaller_than_raw() {
        let input = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa".repeat(20);
        let compressed = estimate_deflate_bytes(input.as_bytes());
        assert!(compressed < input.len());
    }

    #[test]
    fn budget_result_reports_raw_and_deflate_bytes() {
        let result = estimate_budget("void mainImage(out vec4 O,vec2 U){O=vec4(1);}");
        assert!(result.raw_bytes > 0);
        assert!(result.deflate_bytes > 0);
    }

    #[test]
    fn deflate_size_is_deterministic() {
        let source = "float a=dot(p,p),b=dot(p,p);vec3 c=vec3(a,b,a+b);";
        let first = estimate_deflate_bytes(source.as_bytes());
        let second = estimate_deflate_bytes(source.as_bytes());
        assert_eq!(first, second);
    }

    #[test]
    fn presets_are_named_and_non_empty() {
        assert!(presets().iter().any(|p| p.name == "4KB intro"));
        assert!(presets().iter().any(|p| p.name == "Shadertoy"));
    }
}
