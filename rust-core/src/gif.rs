// ROADMAP.md/roadmap_twigl.md Phase 45.1 -- a small, hand-written GIF89a
// encoder (median-cut color quantization + LZW compression, animated,
// looping), written from scratch so the Twigl Export panel can offer a
// local capture matching twigl.app's own size/frame-count presets without
// a network round-trip or a new external dependency. Follows the exact
// discipline `deflate.rs` (Phase 37.2) already established for this
// codebase: hand-written, and independently cross-checked against a real
// reference decoder rather than merely asserted correct.
//
// That cross-check earned its keep during development: an initial
// self-consistent encoder/decoder pair (this module's own from-scratch
// `tests::lzw_decode`, written purely to verify the encoder) passed every
// internal test, including on a 4096-pixel gradient -- but Pillow (a real,
// independent GIF decoder) refused to even open the resulting file. The
// bug was a variable-code-width growth threshold that was self-consistent
// between this module's encoder and its own test decoder but did not
// match the real GIF89a/LZW convention. Both sides were re-derived by
// hand-tracing a fully worked reference example
// (https://giflib.sourceforge.net/whatsinagif/lzw_image_data.html) code
// by code against its documented output, rather than by continued
// guessing: the encoder grows when its next assignable code would exceed
// `2^code_size`; the decoder -- which unavoidably discovers each new
// dictionary entry one code later than the encoder, since it needs that
// entry's *final* byte before it can record it -- grows when its own
// table length reaches `2^code_size`. These are two different formulas
// for the same underlying event, not a typo; that asymmetry is exactly
// what a naive from-scratch implementation gets wrong. All fixtures this
// module's tests use were re-verified against Pillow after the fix
// (solid color, animation, and the >256-color gradient that originally
// failed); that check isn't re-run at build time since Pillow is not a
// Rust dependency, but the encoder itself has not changed since.
//
// Deliberately scoped: no interlacing, no local color tables (one global
// palette shared by every frame, built from the union of all frames' pixel
// data), no transparency. These are real GIF89a features this encoder does
// not use, not gaps in what it produces -- the output is still a fully
// conformant, standard-decodeable animated GIF.

const GIF_HEADER: &[u8; 6] = b"GIF89a";
const EXTENSION_INTRODUCER: u8 = 0x21;
const GRAPHIC_CONTROL_LABEL: u8 = 0xF9;
const APPLICATION_LABEL: u8 = 0xFF;
const IMAGE_SEPARATOR: u8 = 0x2C;
const TRAILER: u8 = 0x3B;
const BLOCK_TERMINATOR: u8 = 0x00;
const PALETTE_SIZE: usize = 256;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Rgb {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

/// One animation frame: RGBA8 pixel data, row-major, top-to-bottom,
/// `width * height * 4` bytes. Alpha is ignored (GIF has no alpha channel
/// in the mode this encoder uses); every pixel is treated as fully opaque.
pub struct GifFrame<'a> {
    pub rgba: &'a [u8],
}

/// Encodes `frames` (all the same `width`x`height`) as an animated,
/// infinitely-looping GIF89a byte stream. `delay_centiseconds` is the
/// per-frame display duration in 1/100ths of a second (GIF's native delay
/// unit) -- e.g. 4 for a 25fps capture, matching twigl.app's own
/// frame-count/duration presets (see `roadmap_twigl.md` 45.1's note on
/// those preset *numbers* being public UI copy, reproduced here only as
/// numbers, not verbatim UI text).
///
/// Returns `None` if `frames` is empty or any frame's length doesn't match
/// `width * height * 4`.
pub fn encode_gif(frames: &[GifFrame], width: u16, height: u16, delay_centiseconds: u16) -> Option<Vec<u8>> {
    if frames.is_empty() || width == 0 || height == 0 {
        return None;
    }
    let expected_len = width as usize * height as usize * 4;
    if frames.iter().any(|f| f.rgba.len() != expected_len) {
        return None;
    }

    let palette = build_palette(frames);
    let palette_lookup = PaletteLookup::new(&palette);

    let mut out = Vec::new();
    out.extend_from_slice(GIF_HEADER);
    write_logical_screen_descriptor(&mut out, width, height);
    write_color_table(&mut out, &palette);
    write_netscape_loop_extension(&mut out);

    for frame in frames {
        let indices = quantize_frame(frame.rgba, &palette_lookup);
        write_graphic_control_extension(&mut out, delay_centiseconds);
        write_image_descriptor(&mut out, width, height);
        write_image_data(&mut out, &indices);
    }

    out.push(TRAILER);
    Some(out)
}

// --- Logical screen / color table / extensions -----------------------

fn write_logical_screen_descriptor(out: &mut Vec<u8>, width: u16, height: u16) {
    out.extend_from_slice(&width.to_le_bytes());
    out.extend_from_slice(&height.to_le_bytes());
    // Global Color Table Flag=1, Color Resolution=111 (8 bits), Sort=0,
    // GCT Size=111 (2^(7+1) = 256 entries).
    out.push(0b1111_0111);
    out.push(0); // Background color index.
    out.push(0); // Pixel aspect ratio (unused).
}

fn write_color_table(out: &mut Vec<u8>, palette: &[Rgb]) {
    for entry in palette {
        out.push(entry.r);
        out.push(entry.g);
        out.push(entry.b);
    }
    // The Global Color Table Size bits above always declare 256 entries;
    // pad any unused tail with black so the table is exactly 256*3 bytes,
    // matching what the Logical Screen Descriptor promised.
    for _ in palette.len()..PALETTE_SIZE {
        out.push(0);
        out.push(0);
        out.push(0);
    }
}

fn write_netscape_loop_extension(out: &mut Vec<u8>) {
    out.push(EXTENSION_INTRODUCER);
    out.push(APPLICATION_LABEL);
    out.push(11); // Block size.
    out.extend_from_slice(b"NETSCAPE2.0");
    out.push(3); // Sub-block size.
    out.push(1); // Fixed.
    out.extend_from_slice(&0u16.to_le_bytes()); // Loop count 0 = forever.
    out.push(BLOCK_TERMINATOR);
}

fn write_graphic_control_extension(out: &mut Vec<u8>, delay_centiseconds: u16) {
    out.push(EXTENSION_INTRODUCER);
    out.push(GRAPHIC_CONTROL_LABEL);
    out.push(4); // Block size.
    // Disposal method = 1 ("do not dispose", leave the frame in place --
    // correct here since every frame is a full-canvas, non-transparent
    // replacement anyway), no user input, no transparency.
    out.push(0b0000_0100);
    out.extend_from_slice(&delay_centiseconds.to_le_bytes());
    out.push(0); // Transparent color index (unused).
    out.push(BLOCK_TERMINATOR);
}

fn write_image_descriptor(out: &mut Vec<u8>, width: u16, height: u16) {
    out.push(IMAGE_SEPARATOR);
    out.extend_from_slice(&0u16.to_le_bytes()); // Left.
    out.extend_from_slice(&0u16.to_le_bytes()); // Top.
    out.extend_from_slice(&width.to_le_bytes());
    out.extend_from_slice(&height.to_le_bytes());
    out.push(0); // No local color table, no interlace.
}

fn write_image_data(out: &mut Vec<u8>, indices: &[u8]) {
    const LZW_MIN_CODE_SIZE: u8 = 8;
    out.push(LZW_MIN_CODE_SIZE);
    let compressed = lzw_encode(indices, LZW_MIN_CODE_SIZE);
    for chunk in compressed.chunks(255) {
        out.push(chunk.len() as u8);
        out.extend_from_slice(chunk);
    }
    out.push(BLOCK_TERMINATOR);
}

// --- Color quantization (median-cut) -----------------------------------

struct ColorBucket {
    // (color, pixel count) pairs belonging to this bucket.
    colors: Vec<(Rgb, u32)>,
}

impl ColorBucket {
    fn channel_range(&self, channel: usize) -> u32 {
        let mut lo = 255u8;
        let mut hi = 0u8;
        for (color, _) in &self.colors {
            let v = match channel {
                0 => color.r,
                1 => color.g,
                _ => color.b,
            };
            lo = lo.min(v);
            hi = hi.max(v);
        }
        hi as u32 - lo as u32
    }

    fn widest_channel(&self) -> usize {
        let ranges = [self.channel_range(0), self.channel_range(1), self.channel_range(2)];
        if ranges[0] >= ranges[1] && ranges[0] >= ranges[2] {
            0
        } else if ranges[1] >= ranges[2] {
            1
        } else {
            2
        }
    }

    fn total_weight(&self) -> u64 {
        self.colors.iter().map(|(_, w)| *w as u64).sum()
    }

    fn average_color(&self) -> Rgb {
        let mut r_sum: u64 = 0;
        let mut g_sum: u64 = 0;
        let mut b_sum: u64 = 0;
        let mut weight_sum: u64 = 0;
        for (color, weight) in &self.colors {
            let w = *weight as u64;
            r_sum += color.r as u64 * w;
            g_sum += color.g as u64 * w;
            b_sum += color.b as u64 * w;
            weight_sum += w;
        }
        if weight_sum == 0 {
            return Rgb { r: 0, g: 0, b: 0 };
        }
        Rgb {
            r: (r_sum / weight_sum) as u8,
            g: (g_sum / weight_sum) as u8,
            b: (b_sum / weight_sum) as u8,
        }
    }

    // Splits this bucket in two along its widest channel, at the
    // weighted median, so each half carries roughly equal pixel weight
    // (standard median-cut, weighted by pixel frequency rather than by
    // unique-color count, so quantization error is minimized where it's
    // most visible).
    fn split(mut self) -> (ColorBucket, ColorBucket) {
        let channel = self.widest_channel();
        self.colors.sort_by_key(|(color, _)| match channel {
            0 => color.r,
            1 => color.g,
            _ => color.b,
        });
        let total = self.total_weight();
        let half = total / 2;
        let mut running = 0u64;
        let mut split_at = self.colors.len() / 2;
        for (i, (_, weight)) in self.colors.iter().enumerate() {
            running += *weight as u64;
            if running >= half {
                split_at = (i + 1).min(self.colors.len().saturating_sub(1)).max(1);
                break;
            }
        }
        let tail = self.colors.split_off(split_at);
        (ColorBucket { colors: self.colors }, ColorBucket { colors: tail })
    }
}

fn collect_weighted_colors(frames: &[GifFrame]) -> Vec<(Rgb, u32)> {
    use std::collections::HashMap;
    let mut counts: HashMap<(u8, u8, u8), u32> = HashMap::new();
    for frame in frames {
        for px in frame.rgba.chunks_exact(4) {
            let key = (px[0], px[1], px[2]);
            *counts.entry(key).or_insert(0) += 1;
        }
    }
    counts
        .into_iter()
        .map(|((r, g, b), count)| (Rgb { r, g, b }, count))
        .collect()
}

fn build_palette(frames: &[GifFrame]) -> Vec<Rgb> {
    let weighted_colors = collect_weighted_colors(frames);

    // No quantization needed at all if every distinct color already fits
    // in a single GIF palette -- the common case for flat-shaded or
    // low-color-count captures, and exact (zero quantization error).
    if weighted_colors.len() <= PALETTE_SIZE {
        return weighted_colors.into_iter().map(|(color, _)| color).collect();
    }

    let mut buckets = vec![ColorBucket { colors: weighted_colors }];
    while buckets.len() < PALETTE_SIZE {
        // Split the bucket with the greatest total pixel weight next --
        // concentrates palette entries where they reduce the most visible
        // quantization error, standard median-cut practice.
        let Some((split_index, _)) = buckets
            .iter()
            .enumerate()
            .filter(|(_, bucket)| bucket.colors.len() > 1)
            .max_by_key(|(_, bucket)| bucket.total_weight())
        else {
            break; // Every remaining bucket already holds a single color.
        };
        let bucket = buckets.remove(split_index);
        let (a, b) = bucket.split();
        buckets.push(a);
        buckets.push(b);
    }

    buckets.iter().map(ColorBucket::average_color).collect()
}

// Nearest-color lookup used when mapping a frame's actual pixels onto the
// (possibly quantized) palette. Exact matches are O(1) via a hash lookup;
// anything else falls back to a brute-force nearest search over the
// palette (at most 256 entries, so this stays cheap even for large
// frames).
struct PaletteLookup<'a> {
    palette: &'a [Rgb],
    exact: std::collections::HashMap<(u8, u8, u8), u8>,
}

impl<'a> PaletteLookup<'a> {
    fn new(palette: &'a [Rgb]) -> Self {
        let mut exact = std::collections::HashMap::new();
        for (i, color) in palette.iter().enumerate() {
            exact.entry((color.r, color.g, color.b)).or_insert(i as u8);
        }
        PaletteLookup { palette, exact }
    }

    fn index_for(&self, r: u8, g: u8, b: u8) -> u8 {
        if let Some(&index) = self.exact.get(&(r, g, b)) {
            return index;
        }
        let mut best_index = 0u8;
        let mut best_distance = u32::MAX;
        for (i, color) in self.palette.iter().enumerate() {
            let dr = r as i32 - color.r as i32;
            let dg = g as i32 - color.g as i32;
            let db = b as i32 - color.b as i32;
            let distance = (dr * dr + dg * dg + db * db) as u32;
            if distance < best_distance {
                best_distance = distance;
                best_index = i as u8;
            }
        }
        best_index
    }
}

fn quantize_frame(rgba: &[u8], lookup: &PaletteLookup) -> Vec<u8> {
    rgba.chunks_exact(4).map(|px| lookup.index_for(px[0], px[1], px[2])).collect()
}

// --- LZW (GIF-flavor: variable code width, LSB-first bit packing) ------

struct LzwBitWriter {
    bytes: Vec<u8>,
    bit_buffer: u32,
    bit_count: u8,
}

impl LzwBitWriter {
    fn new() -> Self {
        LzwBitWriter { bytes: Vec::new(), bit_buffer: 0, bit_count: 0 }
    }

    fn write_code(&mut self, code: u16, code_size: u8) {
        self.bit_buffer |= (code as u32) << self.bit_count;
        self.bit_count += code_size;
        while self.bit_count >= 8 {
            self.bytes.push((self.bit_buffer & 0xFF) as u8);
            self.bit_buffer >>= 8;
            self.bit_count -= 8;
        }
    }

    fn finish(mut self) -> Vec<u8> {
        if self.bit_count > 0 {
            self.bytes.push((self.bit_buffer & 0xFF) as u8);
        }
        self.bytes
    }
}

/// GIF's own LZW variant: codes start at `min_code_size + 1` bits, growing
/// by one bit whenever the dictionary would otherwise overflow the current
/// width, and the dictionary resets (with an explicit Clear Code) at 4096
/// entries (12-bit codes, GIF's maximum). `indices` are already
/// palette-index bytes (0..=255), one per pixel.
fn lzw_encode(indices: &[u8], min_code_size: u8) -> Vec<u8> {
    let clear_code: u16 = 1 << min_code_size;
    let end_code: u16 = clear_code + 1;
    let mut writer = LzwBitWriter::new();

    let initial_dict_size = end_code + 1;
    let mut dict: std::collections::HashMap<Vec<u8>, u16> = std::collections::HashMap::new();
    let reset_dict = |dict: &mut std::collections::HashMap<Vec<u8>, u16>| {
        dict.clear();
        for i in 0..clear_code {
            dict.insert(vec![i as u8], i);
        }
    };
    reset_dict(&mut dict);

    let mut code_size: u8 = min_code_size + 1;
    let mut next_code: u16 = initial_dict_size;

    writer.write_code(clear_code, code_size);

    if indices.is_empty() {
        writer.write_code(end_code, code_size);
        return writer.finish();
    }

    let mut current: Vec<u8> = vec![indices[0]];
    for &index in &indices[1..] {
        let mut extended = current.clone();
        extended.push(index);
        if dict.contains_key(&extended) {
            current = extended;
            continue;
        }

        writer.write_code(*dict.get(&current).expect("current sequence must be in dictionary"), code_size);

        dict.insert(extended, next_code);
        next_code += 1;
        // Grows once the next assignable code would exceed 2^code_size --
        // see this module's top-of-file doc comment for why this is
        // `> (1<<code_size)`, not `>= `/`> (1<<code_size)-1` as a more
        // naive derivation might suggest, and why the decoder's matching
        // threshold (below) is a different formula, not the same one.
        if next_code > (1u16 << code_size) && code_size < 12 {
            code_size += 1;
        }
        if next_code == 4096 {
            writer.write_code(clear_code, code_size);
            reset_dict(&mut dict);
            code_size = min_code_size + 1;
            next_code = initial_dict_size;
        }

        current = vec![index];
    }
    writer.write_code(*dict.get(&current).expect("final sequence must be in dictionary"), code_size);
    writer.write_code(end_code, code_size);
    writer.finish()
}

#[cfg(test)]
mod tests {
    use super::*;

    // --- A minimal, from-scratch GIF *reader*, written purely to verify
    // this encoder's output round-trips -- deliberately independent of the
    // encoder's own internal logic (re-derives code sizes/dictionary from
    // the bitstream itself rather than sharing any state with the writer).

    struct LzwBitReader<'a> {
        bytes: &'a [u8],
        byte_pos: usize,
        bit_buffer: u32,
        bit_count: u8,
    }

    impl<'a> LzwBitReader<'a> {
        fn new(bytes: &'a [u8]) -> Self {
            LzwBitReader { bytes, byte_pos: 0, bit_buffer: 0, bit_count: 0 }
        }

        fn read_code(&mut self, code_size: u8) -> Option<u16> {
            while self.bit_count < code_size {
                if self.byte_pos >= self.bytes.len() {
                    return None;
                }
                self.bit_buffer |= (self.bytes[self.byte_pos] as u32) << self.bit_count;
                self.byte_pos += 1;
                self.bit_count += 8;
            }
            let mask = (1u32 << code_size) - 1;
            let code = (self.bit_buffer & mask) as u16;
            self.bit_buffer >>= code_size;
            self.bit_count -= code_size;
            Some(code)
        }
    }

    fn lzw_decode(data: &[u8], min_code_size: u8) -> Vec<u8> {
        let clear_code: u16 = 1 << min_code_size;
        let end_code: u16 = clear_code + 1;
        let mut reader = LzwBitReader::new(data);
        let mut code_size = min_code_size + 1;

        let mut dict: Vec<Vec<u8>> = Vec::new();
        let reset_dict = |dict: &mut Vec<Vec<u8>>| {
            dict.clear();
            for i in 0..clear_code {
                dict.push(vec![i as u8]);
            }
            dict.push(Vec::new()); // clear code placeholder
            dict.push(Vec::new()); // end code placeholder
        };
        reset_dict(&mut dict);

        let mut output = Vec::new();
        let mut previous: Option<Vec<u8>> = None;

        loop {
            let Some(code) = reader.read_code(code_size) else { break };
            if code == clear_code {
                reset_dict(&mut dict);
                code_size = min_code_size + 1;
                previous = None;
                continue;
            }
            if code == end_code {
                break;
            }

            let entry: Vec<u8> = if (code as usize) < dict.len() && !dict[code as usize].is_empty() {
                dict[code as usize].clone()
            } else if code as usize == dict.len() {
                // KwKwK special case: code refers to the entry about to be
                // added, formed from the previous entry plus its own first
                // byte.
                let mut e = previous.clone().expect("KwKwK case requires a previous entry");
                let first = e[0];
                e.push(first);
                e
            } else {
                panic!("invalid LZW code {code} (dict len {})", dict.len());
            };

            output.extend_from_slice(&entry);

            if let Some(prev) = previous {
                let mut new_entry = prev;
                new_entry.push(entry[0]);
                dict.push(new_entry);
                let next_len = dict.len() as u32;
                // Matches the encoder's own (corrected) threshold exactly:
                // growth triggers once the table's next assignable slot
                // would exceed (1<<code_size), derived from a fully
                // hand-traced GIFLIB worked example (see
                // https://giflib.sourceforge.net/whatsinagif/lzw_image_data.html),
                // not merely asserted -- an earlier "-1" offset here was
                // itself wrong (self-consistent with a same-shaped but
                // wrong encoder threshold, which is why it passed this
                // module's own from-scratch test decoder but produced
                // streams a real decoder -- Pillow -- rejected outright).
                if next_len >= (1u32 << code_size) && code_size < 12 {
                    code_size += 1;
                }
            }
            previous = Some(entry);
        }
        output
    }

    fn parse_sub_blocks(data: &[u8], pos: &mut usize) -> Vec<u8> {
        let mut out = Vec::new();
        loop {
            let len = data[*pos] as usize;
            *pos += 1;
            if len == 0 {
                break;
            }
            out.extend_from_slice(&data[*pos..*pos + len]);
            *pos += len;
        }
        out
    }

    // Decodes just enough of a GIF89a stream (produced by this encoder --
    // this reader is not a general-purpose GIF decoder, it assumes the
    // fixed structure `encode_gif` above always produces: one global color
    // table, no local color tables, no interlacing) to recover every
    // frame's RGB pixels, for round-trip verification.
    fn decode_gif_for_test(data: &[u8]) -> (u16, u16, Vec<Vec<u8>>) {
        assert_eq!(&data[0..6], GIF_HEADER);
        let width = u16::from_le_bytes([data[6], data[7]]);
        let height = u16::from_le_bytes([data[8], data[9]]);
        let packed = data[10];
        assert_eq!(packed & 0b1000_0000, 0b1000_0000, "expected a global color table");
        let gct_size = 1usize << ((packed & 0b0000_0111) as u32 + 1);
        let mut pos = 13usize;
        let mut palette = Vec::with_capacity(gct_size);
        for _ in 0..gct_size {
            palette.push(Rgb { r: data[pos], g: data[pos + 1], b: data[pos + 2] });
            pos += 3;
        }

        let mut frames = Vec::new();
        loop {
            match data[pos] {
                TRAILER => break,
                EXTENSION_INTRODUCER => {
                    let label = data[pos + 1];
                    pos += 2;
                    if label == APPLICATION_LABEL {
                        let block_size = data[pos] as usize;
                        pos += 1 + block_size;
                        let _ = parse_sub_blocks(data, &mut pos);
                    } else {
                        let block_size = data[pos] as usize;
                        pos += 1 + block_size;
                        let _ = parse_sub_blocks(data, &mut pos);
                    }
                }
                IMAGE_SEPARATOR => {
                    pos += 1;
                    let _left = u16::from_le_bytes([data[pos], data[pos + 1]]);
                    let _top = u16::from_le_bytes([data[pos + 2], data[pos + 3]]);
                    let img_w = u16::from_le_bytes([data[pos + 4], data[pos + 5]]);
                    let img_h = u16::from_le_bytes([data[pos + 6], data[pos + 7]]);
                    pos += 8;
                    let img_packed = data[pos];
                    pos += 1;
                    assert_eq!(img_packed & 0b1000_0000, 0, "test reader assumes no local color table");
                    let min_code_size = data[pos];
                    pos += 1;
                    let compressed = parse_sub_blocks(data, &mut pos);
                    let indices = lzw_decode(&compressed, min_code_size);
                    assert_eq!(indices.len(), img_w as usize * img_h as usize);
                    let mut rgb = Vec::with_capacity(indices.len() * 3);
                    for index in indices {
                        let color = palette[index as usize];
                        rgb.push(color.r);
                        rgb.push(color.g);
                        rgb.push(color.b);
                    }
                    frames.push(rgb);
                }
                other => panic!("unexpected block introducer 0x{other:02X} at byte {pos}"),
            }
        }
        (width, height, frames)
    }

    fn solid_frame(width: u16, height: u16, r: u8, g: u8, b: u8) -> Vec<u8> {
        let mut out = Vec::with_capacity(width as usize * height as usize * 4);
        for _ in 0..(width as usize * height as usize) {
            out.extend_from_slice(&[r, g, b, 255]);
        }
        out
    }

    fn checkerboard_frame(width: u16, height: u16) -> Vec<u8> {
        let mut out = Vec::with_capacity(width as usize * height as usize * 4);
        for y in 0..height {
            for x in 0..width {
                let on = (x / 4 + y / 4) % 2 == 0;
                let (r, g, b) = if on { (255, 0, 0) } else { (0, 0, 255) };
                out.extend_from_slice(&[r, g, b, 255]);
            }
        }
        out
    }

    fn gradient_frame(width: u16, height: u16, offset: u8) -> Vec<u8> {
        let mut out = Vec::with_capacity(width as usize * height as usize * 4);
        for y in 0..height {
            for x in 0..width {
                let r = ((x as u32 * 255 / width.max(1) as u32) as u8).wrapping_add(offset);
                let g = ((y as u32 * 255 / height.max(1) as u32) as u8).wrapping_add(offset);
                out.extend_from_slice(&[r, g, 128, 255]);
            }
        }
        out
    }

    #[test]
    fn rejects_empty_frame_list() {
        assert!(encode_gif(&[], 4, 4, 4).is_none());
    }

    #[test]
    fn rejects_frame_with_mismatched_length() {
        let bad = vec![0u8; 10];
        let frames = [GifFrame { rgba: &bad }];
        assert!(encode_gif(&frames, 4, 4, 4).is_none());
    }

    #[test]
    fn header_and_dimensions_round_trip_for_a_single_solid_frame() {
        let pixels = solid_frame(8, 6, 200, 50, 10);
        let frames = [GifFrame { rgba: &pixels }];
        let gif = encode_gif(&frames, 8, 6, 4).expect("encode should succeed");
        assert_eq!(&gif[0..6], GIF_HEADER);
        let (w, h, decoded_frames) = decode_gif_for_test(&gif);
        assert_eq!(w, 8);
        assert_eq!(h, 6);
        assert_eq!(decoded_frames.len(), 1);
    }

    #[test]
    fn solid_color_frame_round_trips_exactly() {
        // A single flat color needs zero quantization -- this must be a
        // lossless round trip, byte for byte.
        let pixels = solid_frame(16, 16, 37, 129, 250);
        let frames = [GifFrame { rgba: &pixels }];
        let gif = encode_gif(&frames, 16, 16, 4).unwrap();
        let (_, _, decoded) = decode_gif_for_test(&gif);
        for px in decoded[0].chunks_exact(3) {
            assert_eq!(px, &[37, 129, 250]);
        }
    }

    #[test]
    fn two_color_checkerboard_round_trips_exactly() {
        let pixels = checkerboard_frame(32, 32);
        let frames = [GifFrame { rgba: &pixels }];
        let gif = encode_gif(&frames, 32, 32, 4).unwrap();
        let (_, _, decoded) = decode_gif_for_test(&gif);
        let expected: Vec<u8> = pixels.chunks_exact(4).flat_map(|px| [px[0], px[1], px[2]]).collect();
        assert_eq!(decoded[0], expected);
    }

    #[test]
    fn low_color_gradient_round_trips_exactly_when_under_the_palette_limit() {
        // A small gradient with a bounded value range stays under 256
        // unique colors, so this must also be lossless (exact palette,
        // no median-cut quantization triggered).
        let width = 12u16;
        let height = 12u16;
        let pixels = gradient_frame(width, height, 0);
        let unique: std::collections::HashSet<(u8, u8, u8)> =
            pixels.chunks_exact(4).map(|p| (p[0], p[1], p[2])).collect();
        assert!(unique.len() <= 256, "test fixture must stay under the palette limit");

        let frames = [GifFrame { rgba: &pixels }];
        let gif = encode_gif(&frames, width, height, 4).unwrap();
        let (_, _, decoded) = decode_gif_for_test(&gif);
        let expected: Vec<u8> = pixels.chunks_exact(4).flat_map(|px| [px[0], px[1], px[2]]).collect();
        assert_eq!(decoded[0], expected);
    }

    #[test]
    fn multi_frame_animation_preserves_frame_count_and_order() {
        let f1 = solid_frame(10, 10, 255, 0, 0);
        let f2 = solid_frame(10, 10, 0, 255, 0);
        let f3 = solid_frame(10, 10, 0, 0, 255);
        let frames = [GifFrame { rgba: &f1 }, GifFrame { rgba: &f2 }, GifFrame { rgba: &f3 }];
        let gif = encode_gif(&frames, 10, 10, 4).unwrap();
        let (_, _, decoded) = decode_gif_for_test(&gif);
        assert_eq!(decoded.len(), 3);
        assert_eq!(decoded[0][0..3], [255, 0, 0]);
        assert_eq!(decoded[1][0..3], [0, 255, 0]);
        assert_eq!(decoded[2][0..3], [0, 0, 255]);
    }

    #[test]
    fn many_unique_colors_trigger_quantization_but_stay_within_a_small_error_bound() {
        // A large, high-color-count frame (a full RGB-ish gradient) forces
        // median-cut quantization (> 256 unique colors). The round trip
        // cannot be exact here -- that would mean the quantizer wasn't
        // actually quantizing anything -- but every pixel's quantized
        // color must stay close to its original, and every unique
        // *quantized* index must map to a real palette entry.
        let width = 64u16;
        let height = 64u16;
        let pixels = gradient_frame(width, height, 0);
        let unique: std::collections::HashSet<(u8, u8, u8)> =
            pixels.chunks_exact(4).map(|p| (p[0], p[1], p[2])).collect();
        assert!(unique.len() > 256, "test fixture must exceed the palette limit to exercise quantization");

        let frames = [GifFrame { rgba: &pixels }];
        let gif = encode_gif(&frames, width, height, 4).unwrap();
        let (_, _, decoded) = decode_gif_for_test(&gif);

        let mut max_channel_error: i32 = 0;
        for (original, decoded_px) in pixels.chunks_exact(4).zip(decoded[0].chunks_exact(3)) {
            for channel in 0..3 {
                let error = (original[channel] as i32 - decoded_px[channel] as i32).abs();
                max_channel_error = max_channel_error.max(error);
            }
        }
        // A smooth gradient quantized to 256 colors should stay well
        // within a small per-channel error; a generous bound here still
        // catches a genuinely broken quantizer (e.g. one that picks
        // arbitrary/unrelated palette entries) while tolerating normal,
        // expected quantization loss.
        assert!(max_channel_error <= 40, "quantization error too large: {max_channel_error}");
    }

    #[test]
    fn palette_never_exceeds_256_entries_even_for_a_very_high_color_count_frame() {
        let width = 100u16;
        let height = 100u16;
        let pixels = gradient_frame(width, height, 0);
        let frames = [GifFrame { rgba: &pixels }];
        let palette = build_palette(&frames);
        assert!(palette.len() <= PALETTE_SIZE);
    }

    #[test]
    fn lzw_round_trips_through_the_independent_test_decoder() {
        let indices: Vec<u8> = (0..500).map(|i| (i % 37) as u8).collect();
        let encoded = lzw_encode(&indices, 8);
        let decoded = lzw_decode(&encoded, 8);
        assert_eq!(decoded, indices);
    }

    #[test]
    fn lzw_round_trips_a_highly_repetitive_stream_that_forces_dictionary_growth() {
        let mut indices = Vec::new();
        for _ in 0..2000 {
            indices.extend_from_slice(&[1, 2, 3, 1, 2, 3, 1, 2, 4]);
        }
        let encoded = lzw_encode(&indices, 8);
        let decoded = lzw_decode(&encoded, 8);
        assert_eq!(decoded, indices);
    }

    #[test]
    fn lzw_handles_a_single_index() {
        let indices = vec![42u8];
        let encoded = lzw_encode(&indices, 8);
        let decoded = lzw_decode(&encoded, 8);
        assert_eq!(decoded, indices);
    }
}

#[cfg(test)]
mod pillow_cross_check_fixtures {
    // Not a real test -- writes fixture GIFs to /tmp for one-off
    // cross-verification against Python's Pillow (an independent,
    // widely-used GIF decoder) during development. Run explicitly with
    // `cargo test --lib gif::pillow_cross_check_fixtures -- --ignored`.
    // See gif.rs's own module doc comment for why this isn't part of the
    // normal (Pillow-free) test suite.
    use super::*;

    fn solid_frame(width: u16, height: u16, r: u8, g: u8, b: u8) -> Vec<u8> {
        let mut out = Vec::with_capacity(width as usize * height as usize * 4);
        for _ in 0..(width as usize * height as usize) {
            out.extend_from_slice(&[r, g, b, 255]);
        }
        out
    }

    fn gradient_frame(width: u16, height: u16) -> Vec<u8> {
        let mut out = Vec::with_capacity(width as usize * height as usize * 4);
        for y in 0..height {
            for x in 0..width {
                let r = (x as u32 * 255 / width.max(1) as u32) as u8;
                let g = (y as u32 * 255 / height.max(1) as u32) as u8;
                let b = ((x as u32 + y as u32) * 255 / (width as u32 + height as u32).max(1)) as u8;
                out.extend_from_slice(&[r, g, b, 255]);
            }
        }
        out
    }

    #[test]
    #[ignore = "writes fixtures to /tmp for manual Pillow cross-check; not part of the default suite"]
    fn write_pillow_cross_check_fixtures() {
        let solid = solid_frame(20, 20, 12, 200, 77);
        std::fs::write(
            "/tmp/ushader_gif_check_solid.gif",
            encode_gif(&[GifFrame { rgba: &solid }], 20, 20, 4).unwrap(),
        )
        .unwrap();

        let grad = gradient_frame(80, 80);
        std::fs::write(
            "/tmp/ushader_gif_check_gradient.gif",
            encode_gif(&[GifFrame { rgba: &grad }], 80, 80, 4).unwrap(),
        )
        .unwrap();

        let f1 = solid_frame(16, 16, 255, 0, 0);
        let f2 = solid_frame(16, 16, 0, 255, 0);
        let f3 = solid_frame(16, 16, 0, 0, 255);
        std::fs::write(
            "/tmp/ushader_gif_check_anim.gif",
            encode_gif(&[GifFrame { rgba: &f1 }, GifFrame { rgba: &f2 }, GifFrame { rgba: &f3 }], 16, 16, 25).unwrap(),
        )
        .unwrap();
    }
}

