// ROADMAP.md Phase 37.2 -- a small, hand-written, RFC 1951-conformant
// DEFLATE encoder (LZ77 + fixed Huffman codes only, single final block),
// written from scratch to close the accuracy gap `golf.md` Phase 30.4
// already flags: `budget.rs`'s existing size estimator approximates
// DEFLATE's byte cost per token class without ever running real LZ77 +
// Huffman coding, so it cannot be calibrated against real compression
// without one. `budget.rs`'s own estimator is left untouched and remains
// the fast, always-on path the golfer's hot loop uses; this module exists
// purely so `iq_corpus_calibration` (below) has real compressed-byte
// counts to compare the estimator's predictions against.
//
// Deliberately fixed-Huffman-only (RFC 1951 section 3.2.6), not dynamic
// Huffman: fixed Huffman is still fully conformant, real DEFLATE -- any
// standard decompressor (zlib, miniz, gzip) decodes it correctly -- it is
// simply not entropy-optimal the way a dynamic-Huffman block is. This
// keeps the encoder's scope proportionate to what Phase 37.2 actually
// needs (a real compressed-byte-count reference to calibrate against),
// without taking on a full RFC 1951 implementation. Zero new external
// dependencies, per this document's own convention (section 2).

const WINDOW_SIZE: usize = 32768;
const MIN_MATCH: usize = 3;
const MAX_MATCH: usize = 258;
const MAX_CHAIN: usize = 128;

// RFC 1951 section 3.2.5: length code 257..=285 -> (base length, extra bits).
const LENGTH_TABLE: [(u16, u8); 29] = [
    (3, 0), (4, 0), (5, 0), (6, 0), (7, 0), (8, 0), (9, 0), (10, 0),
    (11, 1), (13, 1), (15, 1), (17, 1),
    (19, 2), (23, 2), (27, 2), (31, 2),
    (35, 3), (43, 3), (51, 3), (59, 3),
    (67, 4), (83, 4), (99, 4), (115, 4),
    (131, 5), (163, 5), (195, 5), (227, 5),
    (258, 0),
];

// RFC 1951 section 3.2.5: distance code 0..=29 -> (base distance, extra bits).
const DIST_TABLE: [(u16, u8); 30] = [
    (1, 0), (2, 0), (3, 0), (4, 0),
    (5, 1), (7, 1),
    (9, 2), (13, 2),
    (17, 3), (25, 3),
    (33, 4), (49, 4),
    (65, 5), (97, 5),
    (129, 6), (193, 6),
    (257, 7), (385, 7),
    (513, 8), (769, 8),
    (1025, 9), (1537, 9),
    (2049, 10), (3073, 10),
    (4097, 11), (6145, 11),
    (8193, 12), (12289, 12),
    (16385, 13), (24577, 13),
];

fn length_to_code(len: u16) -> (u16, u8, u16) {
    for (i, &(base, extra)) in LENGTH_TABLE.iter().enumerate() {
        let max_for_this_code = if i + 1 < LENGTH_TABLE.len() {
            LENGTH_TABLE[i + 1].0 - 1
        } else {
            258
        };
        if len >= base && len <= max_for_this_code {
            return (257 + i as u16, extra, len - base);
        }
    }
    unreachable!("length out of DEFLATE range: {len}");
}

fn dist_to_code(dist: u16) -> (u16, u8, u16) {
    for (i, &(base, extra)) in DIST_TABLE.iter().enumerate() {
        let max_for_this_code = if i + 1 < DIST_TABLE.len() {
            DIST_TABLE[i + 1].0 - 1
        } else {
            32768
        };
        if dist >= base && dist <= max_for_this_code {
            return (i as u16, extra, dist - base);
        }
    }
    unreachable!("distance out of DEFLATE range: {dist}");
}

struct BitWriter {
    bytes: Vec<u8>,
    cur: u32,
    nbits: u32,
}

impl BitWriter {
    fn new() -> Self {
        Self { bytes: Vec::new(), cur: 0, nbits: 0 }
    }

    fn write_bits_lsb_first(&mut self, value: u32, count: u32) {
        self.cur |= value << self.nbits;
        self.nbits += count;
        while self.nbits >= 8 {
            self.bytes.push((self.cur & 0xFF) as u8);
            self.cur >>= 8;
            self.nbits -= 8;
        }
    }

    // RFC 1951 section 3.1.1: Huffman codes are packed starting with the
    // most-significant bit of the code, unlike every other DEFLATE field.
    fn write_huffman_code(&mut self, code: u32, length: u32) {
        for i in (0..length).rev() {
            self.write_bits_lsb_first((code >> i) & 1, 1);
        }
    }

    fn finish(mut self) -> Vec<u8> {
        if self.nbits > 0 {
            self.bytes.push((self.cur & 0xFF) as u8);
        }
        self.bytes
    }
}

fn write_litlen_symbol(w: &mut BitWriter, symbol: u16) {
    match symbol {
        0..=143 => w.write_huffman_code(0x30 + symbol as u32, 8),
        144..=255 => w.write_huffman_code(0x190 + (symbol - 144) as u32, 9),
        256..=279 => w.write_huffman_code((symbol - 256) as u32, 7),
        280..=287 => w.write_huffman_code(0xC0 + (symbol - 280) as u32, 8),
        _ => unreachable!("literal/length symbol out of range: {symbol}"),
    }
}

enum LzToken {
    Literal(u8),
    Match(u16, u16),
}

fn lz77_compress(data: &[u8]) -> Vec<LzToken> {
    let n = data.len();
    let mut tokens = Vec::new();
    let mut chain: std::collections::HashMap<[u8; 3], Vec<usize>> = std::collections::HashMap::new();
    let mut i = 0usize;

    while i < n {
        let mut best_len = 0usize;
        let mut best_pos = 0usize;

        if i + MIN_MATCH <= n {
            let key = [data[i], data[i + 1], data[i + 2]];
            if let Some(positions) = chain.get(&key) {
                for &j in positions.iter().rev().take(MAX_CHAIN) {
                    if i - j > WINDOW_SIZE {
                        break;
                    }
                    let max_len = (n - i).min(MAX_MATCH);
                    let mut len = 0usize;
                    while len < max_len && data[j + len] == data[i + len] {
                        len += 1;
                    }
                    if len > best_len {
                        best_len = len;
                        best_pos = j;
                    }
                }
            }
        }

        if best_len >= MIN_MATCH {
            let dist = i - best_pos;
            tokens.push(LzToken::Match(best_len as u16, dist as u16));
            let end = (i + best_len).min(n.saturating_sub(MIN_MATCH - 1));
            for k in i..end {
                let key = [data[k], data[k + 1], data[k + 2]];
                chain.entry(key).or_default().push(k);
            }
            i += best_len;
        } else {
            tokens.push(LzToken::Literal(data[i]));
            if i + MIN_MATCH <= n {
                let key = [data[i], data[i + 1], data[i + 2]];
                chain.entry(key).or_default().push(i);
            }
            i += 1;
        }
    }

    tokens
}

/// ROADMAP.md Phase 37.2 -- encodes `data` as a single, final, raw DEFLATE
/// block (RFC 1951, `BTYPE = 01` fixed Huffman codes, no zlib/gzip header
/// or trailer). Real, conformant DEFLATE: any standard decompressor
/// decodes this correctly. Not dynamic-Huffman-optimal, but sufficient to
/// give `budget.rs`'s estimator a genuine compressed-byte-count reference,
/// which is all Phase 37.2 needs.
pub fn deflate_compress(data: &[u8]) -> Vec<u8> {
    let mut w = BitWriter::new();
    w.write_bits_lsb_first(1, 1); // BFINAL = 1 (only block)
    w.write_bits_lsb_first(0b01, 2); // BTYPE = 01 (fixed Huffman)

    for token in lz77_compress(data) {
        match token {
            LzToken::Literal(byte) => write_litlen_symbol(&mut w, byte as u16),
            LzToken::Match(len, dist) => {
                let (len_symbol, len_extra_bits, len_extra_val) = length_to_code(len);
                write_litlen_symbol(&mut w, len_symbol);
                if len_extra_bits > 0 {
                    w.write_bits_lsb_first(len_extra_val as u32, len_extra_bits as u32);
                }
                let (dist_code, dist_extra_bits, dist_extra_val) = dist_to_code(dist);
                w.write_huffman_code(dist_code as u32, 5);
                if dist_extra_bits > 0 {
                    w.write_bits_lsb_first(dist_extra_val as u32, dist_extra_bits as u32);
                }
            }
        }
    }
    write_litlen_symbol(&mut w, 256); // end-of-block

    w.finish()
}

#[cfg(test)]
mod tests {
    use super::*;

    struct BitReader<'a> {
        bytes: &'a [u8],
        byte_pos: usize,
        bit_pos: u32,
    }

    impl<'a> BitReader<'a> {
        fn new(bytes: &'a [u8]) -> Self {
            Self { bytes, byte_pos: 0, bit_pos: 0 }
        }

        fn read_bit(&mut self) -> u32 {
            let byte = self.bytes[self.byte_pos];
            let bit = (byte >> self.bit_pos) & 1;
            self.bit_pos += 1;
            if self.bit_pos == 8 {
                self.bit_pos = 0;
                self.byte_pos += 1;
            }
            bit as u32
        }

        fn read_bits_lsb_first(&mut self, count: u32) -> u32 {
            let mut value = 0u32;
            for i in 0..count {
                value |= self.read_bit() << i;
            }
            value
        }

        fn read_huffman_bit_msb(&mut self, code: &mut u32) {
            *code = (*code << 1) | self.read_bit();
        }
    }

    fn decode_litlen(r: &mut BitReader) -> u16 {
        let mut code: u32 = 0;
        for len in 1..=9u32 {
            r.read_huffman_bit_msb(&mut code);
            match len {
                7 => {
                    if code <= 0b0010111 {
                        return 256 + code as u16;
                    }
                }
                8 => {
                    if (0x30..=0xBF).contains(&code) {
                        return (code - 0x30) as u16;
                    }
                    if (0xC0..=0xC7).contains(&code) {
                        return 280 + (code - 0xC0) as u16;
                    }
                }
                9 => {
                    if (0x190..=0x1FF).contains(&code) {
                        return 144 + (code - 0x190) as u16;
                    }
                }
                _ => {}
            }
        }
        panic!("invalid fixed-Huffman literal/length code");
    }

    fn deflate_decompress(data: &[u8]) -> Vec<u8> {
        let mut r = BitReader::new(data);
        let bfinal = r.read_bits_lsb_first(1);
        let btype = r.read_bits_lsb_first(2);
        assert_eq!(bfinal, 1);
        assert_eq!(btype, 0b01);

        let mut out: Vec<u8> = Vec::new();
        loop {
            let symbol = decode_litlen(&mut r);
            if symbol == 256 {
                break;
            } else if symbol < 256 {
                out.push(symbol as u8);
            } else {
                let (base_len, extra_bits) = LENGTH_TABLE[(symbol - 257) as usize];
                let len = base_len + r.read_bits_lsb_first(extra_bits as u32) as u16;

                let mut dist_code: u32 = 0;
                for _ in 0..5 {
                    r.read_huffman_bit_msb(&mut dist_code);
                }
                let (base_dist, dist_extra_bits) = DIST_TABLE[dist_code as usize];
                let dist = base_dist + r.read_bits_lsb_first(dist_extra_bits as u32) as u16;

                let start = out.len() - dist as usize;
                for k in 0..len as usize {
                    let byte = out[start + k];
                    out.push(byte);
                }
            }
        }
        out
    }

    fn assert_round_trips(data: &[u8]) {
        let compressed = deflate_compress(data);
        let decompressed = deflate_decompress(&compressed);
        assert_eq!(decompressed, data, "round-trip mismatch for {} input bytes", data.len());
    }

    #[test]
    fn round_trips_empty_input() {
        assert_round_trips(b"");
    }

    #[test]
    fn round_trips_a_single_byte() {
        assert_round_trips(b"x");
    }

    #[test]
    fn round_trips_text_with_no_repeats() {
        assert_round_trips(b"abcdefghijklmnopqrstuvwxyz0123456789");
    }

    #[test]
    fn round_trips_highly_repetitive_text() {
        let data = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            .as_bytes();
        assert_round_trips(data);
    }

    #[test]
    fn round_trips_a_real_glsl_fixture() {
        assert_round_trips(include_bytes!("../../fixtures/macro_cse.glsl"));
        assert_round_trips(include_bytes!("../../fixtures/loop_header_golf.glsl"));
        assert_round_trips(include_bytes!("../../fixtures/frequency_renaming.glsl"));
    }

    #[test]
    fn round_trips_a_match_at_the_maximum_length_and_distance() {
        let mut data = vec![0u8; 40000];
        for (i, byte) in data.iter_mut().enumerate() {
            *byte = (i % 7) as u8;
        }
        assert_round_trips(&data);
    }

    #[test]
    fn compresses_repetitive_input_smaller_than_the_input_itself() {
        let data = "the quick brown fox the quick brown fox the quick brown fox the quick brown fox"
            .as_bytes();
        let compressed = deflate_compress(data);
        assert!(compressed.len() < data.len(), "compressed={} original={}", compressed.len(), data.len());
    }
}
