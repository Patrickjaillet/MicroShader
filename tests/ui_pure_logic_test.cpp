// Regression coverage for pure-logic UI modules that had zero dedicated
// tests before this file, despite containing real algorithmic logic (not
// just D2D rendering) -- see TODO.md, "Couverture de tests quasi nulle sur
// plusieurs modules de logique pure, faciles a tester". Modeled on
// tests/twigl_golf_collision_test.cpp's plain main()-based harness (no
// framework dependency).
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../src/ui/fuzzy_match.h"
#include "../src/ui/unified_diff.h"
#include "../src/ui/glsl_format.h"
#include "../src/ui/keybindings_storage.h"
#include "../src/ui/golf_controls.h"
#include "../src/ui/glsl_token_rules.h"
#include "../src/ui/glsl_syntax_colors.h"
#include "../src/ui/recent_files.h"
#include "../src/ui/glsl_numeric_literals.h"

namespace
{
    int failures = 0;

    void expect_true(bool condition, const char* what)
    {
        if (!condition)
        {
            std::fprintf(stderr, "FAIL %s: expected true\n", what);
            ++failures;
        }
    }

    void expect_false(bool condition, const char* what)
    {
        if (condition)
        {
            std::fprintf(stderr, "FAIL %s: expected false\n", what);
            ++failures;
        }
    }

    void expect_eq(const std::string& actual, const std::string& expected, const char* what)
    {
        if (actual != expected)
        {
            std::fprintf(stderr, "FAIL %s: expected \"%s\", got \"%s\"\n", what, expected.c_str(), actual.c_str());
            ++failures;
        }
    }

    void expect_eq_double(double actual, double expected, const char* what)
    {
        if (std::fabs(actual - expected) > 1e-9)
        {
            std::fprintf(stderr, "FAIL %s: expected %f, got %f\n", what, expected, actual);
            ++failures;
        }
    }

    void expect_eq_int(long long actual, long long expected, const char* what)
    {
        if (actual != expected)
        {
            std::fprintf(stderr, "FAIL %s: expected %lld, got %lld\n", what, expected, actual);
            ++failures;
        }
    }

    // --- fuzzy_match.cpp ---------------------------------------------------

    void test_fuzzy_match()
    {
        expect_true(fuzzy_match("", "anything"), "empty query matches everything");
        expect_true(fuzzy_match("cp", "Command Palette"), "subsequence match, case-insensitive");
        expect_true(fuzzy_match("CMDPAL", "command palette"), "uppercase query against lowercase label");
        expect_false(fuzzy_match("zzz", "Command Palette"), "characters absent from label");
        expect_false(fuzzy_match("ba", "ab"), "out-of-order characters must not match");
        expect_true(fuzzy_match("abc", "abc"), "exact match");
        expect_false(fuzzy_match("abcd", "abc"), "query longer than label");
        expect_true(fuzzy_match("dif", "Diff View"), "prefix subsequence match");
    }

    // --- unified_diff.cpp ----------------------------------------------------

    void test_unified_diff()
    {
        {
            std::vector<DiffSpan> spans = compute_unified_diff("abc", "abc");
            expect_true(!spans.empty(), "identical text produces at least one span");
            for (const DiffSpan& s : spans)
            {
                expect_true(s.kind == DiffSpanKind::Unchanged, "identical text has no added/removed spans");
            }
        }
        {
            std::vector<DiffSpan> spans = compute_unified_diff("", "hello");
            bool saw_added = false;
            for (const DiffSpan& s : spans)
            {
                expect_true(s.kind != DiffSpanKind::Removed, "nothing to remove from empty before-text");
                if (s.kind == DiffSpanKind::Added)
                {
                    saw_added = true;
                }
            }
            expect_true(saw_added, "text added to empty before-text must appear as Added spans");
        }
        {
            std::vector<DiffSpan> spans = compute_unified_diff("hello", "");
            bool saw_removed = false;
            for (const DiffSpan& s : spans)
            {
                expect_true(s.kind != DiffSpanKind::Added, "nothing to add when after-text is empty");
                if (s.kind == DiffSpanKind::Removed)
                {
                    saw_removed = true;
                }
            }
            expect_true(saw_removed, "text removed down to empty after-text must appear as Removed spans");
        }
        {
            // A single changed token in the middle of otherwise-identical text
            // must not blow away the unchanged spans around it.
            std::vector<DiffSpan> spans = compute_unified_diff("float a=1.0;", "float a=2.0;");
            bool saw_unchanged = false;
            bool saw_removed = false;
            bool saw_added = false;
            for (const DiffSpan& s : spans)
            {
                if (s.kind == DiffSpanKind::Unchanged) saw_unchanged = true;
                if (s.kind == DiffSpanKind::Removed) saw_removed = true;
                if (s.kind == DiffSpanKind::Added) saw_added = true;
            }
            expect_true(saw_unchanged, "shared prefix/suffix tokens must be Unchanged");
            expect_true(saw_removed, "the old literal must be Removed");
            expect_true(saw_added, "the new literal must be Added");
        }
        {
            // Reassembling every span's text (in order) must reproduce the
            // "after" text when Unchanged+Added spans are concatenated.
            std::string before = "vec3 c=vec3(1.0,2.0,3.0);";
            std::string after = "vec3 col=vec3(1.0,4.0,3.0);";
            std::vector<DiffSpan> spans = compute_unified_diff(before, after);
            std::string reconstructed_after;
            for (const DiffSpan& s : spans)
            {
                if (s.kind != DiffSpanKind::Removed)
                {
                    reconstructed_after += s.text;
                }
            }
            expect_eq(reconstructed_after, after, "Unchanged+Added spans must reconstruct the after-text exactly");
        }
    }

    // --- glsl_format.cpp -----------------------------------------------------

    void test_glsl_format()
    {
        {
            std::string result = format_glsl("void main(){float a=1.;}");
            expect_true(result.find('\n') != std::string::npos, "braces introduce newlines");
            expect_true(result.find("{") != std::string::npos, "opening brace preserved");
            expect_true(result.find("}") != std::string::npos, "closing brace preserved");
        }
        {
            // Semicolons inside parens (e.g. a for-loop header) must not be
            // split onto their own lines -- only top-level statement ends.
            std::string result = format_glsl("for(int i=0;i<10;i++){x+=1.;}");
            std::size_t for_pos = result.find("for(int i=0;i<10;i++)");
            expect_true(for_pos != std::string::npos, "for-loop header parens/semicolons stay on one line");
        }
        {
            // Formatting must never lose any non-whitespace character.
            std::string source = "void f(){int a=1;int b=2;}";
            std::string result = format_glsl(source);
            std::string stripped_source;
            std::string stripped_result;
            for (char c : source) { if (c != ' ' && c != '\n') stripped_source += c; }
            for (char c : result) { if (c != ' ' && c != '\n') stripped_result += c; }
            expect_eq(stripped_result, stripped_source, "formatting must preserve every non-whitespace character");
        }
        {
            std::string result = format_glsl("");
            expect_eq(result, "", "empty source formats to empty output");
        }
    }

    // --- keybindings_storage.cpp ----------------------------------------------

    void test_keybindings_storage()
    {
        std::string json =
            "{\n"
            "  \"save_key\": \"S\",\n"
            "  \"save_ctrl\": true,\n"
            "  \"save_shift\": false,\n"
            "  \"save_alt\": false\n"
            "}\n";

        expect_eq(find_string_field(json, "save_key"), "S", "find_string_field extracts the quoted value");
        expect_eq(find_string_field(json, "missing_key"), "", "find_string_field returns empty for an absent key");
        expect_true(find_bool_field(json, "save_ctrl", false), "find_bool_field reads true");
        expect_false(find_bool_field(json, "save_shift", true), "find_bool_field reads false");
        expect_true(find_bool_field(json, "missing_bool", true), "find_bool_field falls back to the default when absent");

        RawKeyChord fallback;
        fallback.key_name = "X";
        fallback.ctrl = false;
        fallback.shift = false;
        fallback.alt = false;
        RawKeyChord chord = find_raw_chord(json, "save", fallback);
        expect_eq(chord.key_name, "S", "find_raw_chord picks up the prefixed key field");
        expect_true(chord.ctrl, "find_raw_chord picks up the prefixed ctrl field");
        expect_false(chord.shift, "find_raw_chord picks up the prefixed shift field");

        RawKeyChord missing_chord = find_raw_chord(json, "undo", fallback);
        expect_eq(missing_chord.key_name, "X", "find_raw_chord falls back entirely when the prefix is absent");

        // Round-trip: what append_raw_chord_field writes, find_raw_chord must
        // read back identically.
        RawKeyChord original;
        original.key_name = "Z";
        original.ctrl = true;
        original.shift = true;
        original.alt = false;
        std::string out;
        append_raw_chord_field(out, "undo", original, /*trailing_comma=*/false);
        RawKeyChord roundtrip = find_raw_chord(out, "undo", fallback);
        expect_eq(roundtrip.key_name, original.key_name, "round-trip preserves key_name");
        expect_true(roundtrip.ctrl == original.ctrl, "round-trip preserves ctrl");
        expect_true(roundtrip.shift == original.shift, "round-trip preserves shift");
        expect_true(roundtrip.alt == original.alt, "round-trip preserves alt");
    }

    // --- golf_options_convert.cpp ----------------------------------------------

    void test_golf_options_convert()
    {
        {
            GolfPassToggles toggles;
            toggles.aggressive = false;
            UshaderGolfOptions opts = to_golf_options(toggles);
            UshaderGolfOptions zero{};
            expect_true(opts.eliminate_dead_locals == zero.eliminate_dead_locals
                && opts.merge_declarations == zero.merge_declarations
                && opts.hoist_declarations == zero.hoist_declarations,
                "aggressive=false collapses every field to the zero/default UshaderGolfOptions");
        }
        {
            GolfPassToggles toggles;
            toggles.aggressive = true;
            toggles.eliminate_dead_locals = true;
            toggles.merge_declarations = false;
            toggles.swizzle_alphabet = 2;
            toggles.hoist_declarations = true;
            UshaderGolfOptions opts = to_golf_options(toggles);
            expect_true(opts.eliminate_dead_locals, "individual true toggle survives the conversion");
            expect_false(opts.merge_declarations, "individual false toggle survives the conversion");
            expect_eq_int(opts.swizzle_alphabet, 2, "non-bool field (swizzle_alphabet) is copied through");
            expect_true(opts.hoist_declarations, "hoist_declarations toggle survives the conversion");
        }
    }

    // --- glsl_token_rules.cpp / glsl_syntax_colors.cpp --------------------------

    void test_glsl_token_rules()
    {
        expect_true(classify_glsl_token_kind("if") == GlslTokenKind::Keyword, "'if' is a keyword");
        expect_true(classify_glsl_token_kind("vec3") == GlslTokenKind::Keyword, "'vec3' is a keyword");
        expect_true(classify_glsl_token_kind("sin") == GlslTokenKind::BuiltinIdentifier, "'sin' is a builtin function");
        expect_true(classify_glsl_token_kind("iResolution") == GlslTokenKind::BuiltinIdentifier, "'iResolution' is a builtin variable");
        expect_true(classify_glsl_token_kind("myVariable") == GlslTokenKind::Identifier, "an ordinary name is a plain identifier");
        expect_true(classify_glsl_token_kind("42") == GlslTokenKind::Number, "a digit-leading token is a number");
        expect_true(classify_glsl_token_kind("-1.5") == GlslTokenKind::Number, "a signed decimal is a number");
        expect_true(classify_glsl_token_kind("\"hi\"") == GlslTokenKind::String, "a quote-leading token is a string");
        expect_true(classify_glsl_token_kind("'a'") == GlslTokenKind::CharLiteral, "a single-quote-leading token is a char literal");
        expect_true(classify_glsl_token_kind("// note") == GlslTokenKind::Comment, "a line-comment token is a comment");
        expect_true(classify_glsl_token_kind("#version") == GlslTokenKind::Preprocessor, "a #-leading token is a preprocessor directive");
        expect_true(classify_glsl_token_kind("+") == GlslTokenKind::Punctuation, "a bare operator is punctuation");
        expect_true(classify_glsl_token_kind("") == GlslTokenKind::Default, "an empty token is Default");

        {
            bool in_block_comment = false;
            std::vector<GlslTokenSpan> spans = tokenize_glsl_line(L"float a=1.0; // trailing", in_block_comment);
            expect_false(in_block_comment, "a line comment does not open a block comment");
            bool saw_keyword = false;
            bool saw_number = false;
            bool saw_comment = false;
            for (const GlslTokenSpan& span : spans)
            {
                if (span.kind == GlslTokenKind::Keyword) saw_keyword = true;
                if (span.kind == GlslTokenKind::Number) saw_number = true;
                if (span.kind == GlslTokenKind::Comment) saw_comment = true;
            }
            expect_true(saw_keyword, "tokenize_glsl_line finds the 'float' keyword");
            expect_true(saw_number, "tokenize_glsl_line finds the numeric literal");
            expect_true(saw_comment, "tokenize_glsl_line finds the trailing line comment");
        }
        {
            // A block comment opened but not closed on this line must carry
            // the in_block_comment flag forward to the next call.
            bool in_block_comment = false;
            tokenize_glsl_line(L"/* not yet closed", in_block_comment);
            expect_true(in_block_comment, "an unterminated block comment sets in_block_comment");

            std::vector<GlslTokenSpan> continued = tokenize_glsl_line(L"still inside */ float x=1.;", in_block_comment);
            expect_false(in_block_comment, "the closing */ clears in_block_comment on a later line");
            expect_true(!continued.empty() && continued.front().kind == GlslTokenKind::Comment,
                "the continuation line's leading span (up to */) is still a Comment");
        }
        {
            // Every span's [start, start+length) range must stay inside the
            // line and be non-overlapping/monotonic -- a basic sanity check
            // that the tokenizer never produces a garbage span the D2D
            // highlighter would read out of bounds.
            bool in_block_comment = false;
            std::wstring line = L"vec4 col = texture2D(tex, uv.xy) * 0.5;";
            std::vector<GlslTokenSpan> spans = tokenize_glsl_line(line, in_block_comment);
            int previous_end = 0;
            bool monotonic = true;
            bool in_bounds = true;
            for (const GlslTokenSpan& span : spans)
            {
                if (span.start < previous_end) monotonic = false;
                if (span.start < 0 || span.start + span.length > static_cast<int>(line.size())) in_bounds = false;
                previous_end = span.start + span.length;
            }
            expect_true(monotonic, "token spans never overlap or go backwards");
            expect_true(in_bounds, "token spans never exceed the line's bounds");
        }
    }

    void test_glsl_syntax_colors()
    {
        // glsl_syntax_color must return a distinct color per meaningful kind
        // (not silently fall through everything to the same default), and
        // every call must be well-formed (alpha channel present).
        D2D1_COLOR_F keyword_color = glsl_syntax_color(GlslTokenKind::Keyword);
        D2D1_COLOR_F identifier_color = glsl_syntax_color(GlslTokenKind::Identifier);
        D2D1_COLOR_F comment_color = glsl_syntax_color(GlslTokenKind::Comment);
        bool keyword_differs = keyword_color.r != identifier_color.r
            || keyword_color.g != identifier_color.g
            || keyword_color.b != identifier_color.b;
        bool comment_differs = comment_color.r != identifier_color.r
            || comment_color.g != identifier_color.g
            || comment_color.b != identifier_color.b;
        expect_true(keyword_differs, "Keyword color differs from Identifier's default color");
        expect_true(comment_differs, "Comment color differs from Identifier's default color");
        expect_true(keyword_color.a > 0.0f, "returned colors are fully constructed (non-zero alpha)");
    }

    // --- recent_files.cpp --------------------------------------------------
    //
    // load/add/remove/clear_recent_files persist to %APPDATA%\ushader\
    // recent_files.json with no override hook other than the APPDATA
    // environment variable itself, so this redirects APPDATA to a scratch
    // directory for the lifetime of this test process -- never touching the
    // real user's actual recent-files list.

    std::wstring make_scratch_appdata_dir()
    {
        wchar_t temp_dir[MAX_PATH];
        GetTempPathW(MAX_PATH, temp_dir);
        std::wstring dir = temp_dir;
        dir += L"ushader_recent_files_test_appdata";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }

    void test_recent_files()
    {
        std::wstring scratch_dir = make_scratch_appdata_dir();
        SetEnvironmentVariableW(L"APPDATA", scratch_dir.c_str());

        clear_recent_files();
        expect_true(load_recent_files().empty(), "clear_recent_files empties the list");

        add_recent_file("C:/shaders/a.glsl");
        add_recent_file("C:/shaders/b.glsl");
        std::vector<std::string> files = load_recent_files();
        expect_eq_int(static_cast<long long>(files.size()), 2, "two distinct files tracked after two adds");
        expect_eq(files[0], "C:/shaders/b.glsl", "most recently added file is first");
        expect_eq(files[1], "C:/shaders/a.glsl", "earlier file follows it");

        // Re-adding an already-present file must move it to the front, not
        // duplicate it.
        add_recent_file("C:/shaders/a.glsl");
        files = load_recent_files();
        expect_eq_int(static_cast<long long>(files.size()), 2, "re-adding an existing entry does not duplicate it");
        expect_eq(files[0], "C:/shaders/a.glsl", "re-added file moves to the front");

        remove_recent_file("C:/shaders/b.glsl");
        files = load_recent_files();
        expect_eq_int(static_cast<long long>(files.size()), 1, "remove_recent_file drops exactly the named entry");
        expect_eq(files[0], "C:/shaders/a.glsl", "the remaining entry is unaffected");

        // The list must be capped, most-recent-first, when it exceeds the
        // documented maximum (10).
        clear_recent_files();
        for (int i = 0; i < 15; ++i)
        {
            add_recent_file("C:/shaders/file" + std::to_string(i) + ".glsl");
        }
        files = load_recent_files();
        expect_eq_int(static_cast<long long>(files.size()), 10, "the recent-files list is capped at 10 entries");
        expect_eq(files[0], "C:/shaders/file14.glsl", "the cap keeps the most recently added entries");

        clear_recent_files();
        expect_true(load_recent_files().empty(), "clear_recent_files empties the list again");
    }

    // --- glsl_numeric_literals.cpp ------------------------------------------

    void test_glsl_numeric_literals()
    {
        {
            std::string source = "void mainImage(out vec4 fragColor,in vec2 fragCoord)"
                "{fragColor=vec4(1.0,0.5,.25,1e-2);}";
            std::vector<GlslNumericLiteral> literals = find_glsl_float_literals(source);
            expect_eq_int(static_cast<long long>(literals.size()), 4, "finds every float literal in a vec4 constructor");
            expect_eq_double(literals[0].value, 1.0, "first literal parses as 1.0");
            expect_eq_double(literals[1].value, 0.5, "second literal parses as 0.5");
            expect_eq_double(literals[2].value, 0.25, "leading-dot literal .25 parses as 0.25");
            expect_eq_double(literals[3].value, 0.01, "exponent literal 1e-2 parses as 0.01");
            const char* expected_text[] = { "1.0", "0.5", ".25", "1e-2" };
            for (std::size_t i = 0; i < literals.size(); ++i)
            {
                expect_eq(source.substr(literals[i].byte_offset, literals[i].byte_length), expected_text[i],
                    "byte_offset/byte_length slice out exactly the original literal text");
            }
        }
        {
            // Bare integers (loop bounds, indices) are out of scope; digits
            // that are part of a longer identifier (vec3, mat2x2) must not
            // be mistaken for literals; swizzle/member access (.xyz) must
            // not be mistaken for a leading-dot literal.
            std::string source = "vec3 v = arr[2]; mat2x2 m; float x1 = 3.0; v = v.xyz;";
            std::vector<GlslNumericLiteral> literals = find_glsl_float_literals(source);
            expect_eq_int(static_cast<long long>(literals.size()), 1, "only the genuine float literal is found");
            expect_eq_double(literals[0].value, 3.0, "the one real literal parses correctly");
        }
        {
            std::string source = "float a = 1.0; // 2.0 is commented out\n"
                "/* 3.0 is block-commented */\n"
                "float b = 4.0;";
            std::vector<GlslNumericLiteral> literals = find_glsl_float_literals(source);
            expect_eq_int(static_cast<long long>(literals.size()), 2, "literals inside // and /* */ comments are skipped");
            expect_eq_double(literals[0].value, 1.0, "first real literal is 1.0");
            expect_eq_double(literals[1].value, 4.0, "second real literal is 4.0");
        }
        {
            expect_eq(format_glsl_float_literal(1.0), "1.0", "trims trailing zeros but keeps one digit after the point");
            expect_eq(format_glsl_float_literal(0.25), "0.25", "keeps significant trailing digits");
            expect_eq(format_glsl_float_literal(3.0), "3.0", "an integral value still gets a decimal point");
        }
        {
            SliderRange zero = compute_slider_range(0.0);
            expect_eq_double(zero.min_value, -1.0, "zero maps to a symmetric [-1, 1] range (min)");
            expect_eq_double(zero.max_value, 1.0, "zero maps to a symmetric [-1, 1] range (max)");

            SliderRange positive = compute_slider_range(0.5);
            expect_eq_double(positive.min_value, 0.0, "a positive value's range starts at 0");
            expect_eq_double(positive.max_value, 1.0, "a positive value's range tops out at 2x itself");

            SliderRange negative = compute_slider_range(-3.0);
            expect_eq_double(negative.min_value, -6.0, "a negative value's range is symmetric (min)");
            expect_eq_double(negative.max_value, 6.0, "a negative value's range is symmetric, crossing zero (max)");
        }
        {
            std::string spliced = splice_source("vec3(1.0, 2.0)", 5, 3, "1.5");
            expect_eq(spliced, "vec3(1.5, 2.0)", "splice_source replaces exactly the given byte range");
        }
        {
            std::string source = "void mainImage(out vec4 fragColor, in vec2 fragCoord)\n"
                "{\n"
                "    float x = 0.5;\n"
                "    fragColor = vec4(x);\n"
                "}\n";
            std::size_t offset = source.find("0.5");
            std::string snippet = literal_context_snippet(source, offset, 3, 30);
            expect_eq(snippet, "float x = 0.5;", "short lines are returned whole, leading whitespace trimmed");

            std::string long_source = "float veryLongVariableNameHere = 0.5; float another = 1.0;";
            std::size_t long_offset = long_source.find("0.5");
            std::string long_snippet = literal_context_snippet(long_source, long_offset, 3, 20);
            expect_eq_int(static_cast<long long>(long_snippet.size()), 20, "an overlong line is truncated to max_chars");
        }
    }
}

int main()
{
    test_fuzzy_match();
    test_unified_diff();
    test_glsl_format();
    test_keybindings_storage();
    test_golf_options_convert();
    test_glsl_token_rules();
    test_glsl_syntax_colors();
    test_recent_files();
    test_glsl_numeric_literals();

    if (failures == 0)
    {
        std::printf("ui_pure_logic_test: all checks passed\n");
        return 0;
    }
    std::fprintf(stderr, "ui_pure_logic_test: %d failure(s)\n", failures);
    return 1;
}
