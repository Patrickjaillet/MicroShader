#[derive(Debug, Clone, PartialEq)]
pub enum Tok {
    Preproc(String),
    Ident(String),
    Number(String),
    Punct(char),
}

pub fn tokenize_spaced(src: &str) -> Vec<(Tok, bool)> {
    let bytes: Vec<char> = src.chars().collect();
    let n = bytes.len();
    let mut i = 0usize;
    let mut out = Vec::new();
    let mut had_space = true;

    while i < n {
        let c = bytes[i];

        if c == '/' && i + 1 < n && bytes[i + 1] == '/' {
            while i < n && bytes[i] != '\n' {
                i += 1;
            }
            had_space = true;
            continue;
        }
        if c == '/' && i + 1 < n && bytes[i + 1] == '*' {
            i += 2;
            while i + 1 < n && !(bytes[i] == '*' && bytes[i + 1] == '/') {
                i += 1;
            }
            i += 2;
            had_space = true;
            continue;
        }
        if c == '#' {
            let start = i;
            while i < n && bytes[i] != '\n' {
                i += 1;
            }
            let line: String = bytes[start..i].iter().collect();
            out.push((Tok::Preproc(line.trim().to_string()), true));
            had_space = true;
            continue;
        }
        if c.is_whitespace() {
            i += 1;
            had_space = true;
            continue;
        }
        if c.is_ascii_digit() || (c == '.' && i + 1 < n && bytes[i + 1].is_ascii_digit()) {
            let start = i;
            if c == '0' && i + 1 < n && (bytes[i + 1] == 'x' || bytes[i + 1] == 'X') {
                i += 2;
                while i < n && bytes[i].is_ascii_hexdigit() {
                    i += 1;
                }
            } else {
                while i < n && bytes[i].is_ascii_digit() {
                    i += 1;
                }
                if i < n && bytes[i] == '.' {
                    i += 1;
                    while i < n && bytes[i].is_ascii_digit() {
                        i += 1;
                    }
                }
                if i < n && (bytes[i] == 'e' || bytes[i] == 'E') {
                    let save = i;
                    let mut j = i + 1;
                    if j < n && (bytes[j] == '+' || bytes[j] == '-') {
                        j += 1;
                    }
                    if j < n && bytes[j].is_ascii_digit() {
                        i = j;
                        while i < n && bytes[i].is_ascii_digit() {
                            i += 1;
                        }
                    } else {
                        i = save;
                    }
                }
            }
            while i < n && (bytes[i] == 'u' || bytes[i] == 'U' || bytes[i] == 'f' || bytes[i] == 'F') {
                i += 1;
            }
            let text: String = bytes[start..i].iter().collect();
            out.push((Tok::Number(text), had_space));
            had_space = false;
            continue;
        }
        if c.is_ascii_alphabetic() || c == '_' {
            let start = i;
            while i < n && (bytes[i].is_ascii_alphanumeric() || bytes[i] == '_') {
                i += 1;
            }
            let text: String = bytes[start..i].iter().collect();
            out.push((Tok::Ident(text), had_space));
            had_space = false;
            continue;
        }
        out.push((Tok::Punct(c), had_space));
        had_space = false;
        i += 1;
    }

    out
}
