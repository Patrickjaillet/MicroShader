import re
import sys
from pathlib import Path

AA_THRESHOLD = 4.5

TOKENS_PATH = Path(__file__).resolve().parent.parent / "src" / "ui" / "theme_tokens.h"

PAIRS = [
    ("text_primary", "bg_panel"),
    ("text_primary", "bg_app"),
    ("text_secondary", "bg_panel"),
    ("text_secondary", "bg_app"),
]


def load_tokens(path):
    text = path.read_text(encoding="utf-8")
    pattern = re.compile(r"constexpr Color4 (\w+) = from_hex\(0x([0-9A-Fa-f]{6})")
    return {name: int(hex_value, 16) for name, hex_value in pattern.findall(text)}


def srgb_channel_to_linear(channel_8bit):
    channel = channel_8bit / 255.0
    if channel <= 0.03928:
        return channel / 12.92
    return ((channel + 0.055) / 1.055) ** 2.4


def relative_luminance(hex_value):
    r = (hex_value >> 16) & 0xFF
    g = (hex_value >> 8) & 0xFF
    b = hex_value & 0xFF
    r_lin = srgb_channel_to_linear(r)
    g_lin = srgb_channel_to_linear(g)
    b_lin = srgb_channel_to_linear(b)
    return 0.2126 * r_lin + 0.7152 * g_lin + 0.0722 * b_lin


def contrast_ratio(hex_a, hex_b):
    l_a = relative_luminance(hex_a) + 0.05
    l_b = relative_luminance(hex_b) + 0.05
    return max(l_a, l_b) / min(l_a, l_b)


def run(label, tokens):
    print("== %s token set ==" % label)
    all_pass = True
    for fg_name, bg_name in PAIRS:
        ratio = contrast_ratio(tokens[fg_name], tokens[bg_name])
        status = "PASS" if ratio >= AA_THRESHOLD else "FAIL"
        if status == "FAIL":
            all_pass = False
        print("%-14s vs %-9s : %.2f:1  [%s]" % (fg_name, bg_name, ratio, status))
    return all_pass


def main():
    tokens = load_tokens(TOKENS_PATH)
    default_pass = run("default", tokens)
    print()
    print(
        "Colorblind-safe mode (Phase 20.3) reuses the same status.ok/"
        "warning/error color tokens and the same text/background tokens "
        "verified above -- it only swaps the drawn shape (circle/"
        "triangle/square), so there is no separate color token set to "
        "check; re-running the identical pairs below is a repeat of the "
        "same numbers, confirming that fact rather than finding new colors."
    )
    print()
    colorblind_pass = run("colorblind-safe", tokens)

    if default_pass and colorblind_pass:
        print("\nAll pairs pass WCAG AA (%.1f:1 threshold)." % AA_THRESHOLD)
        return 0
    print("\nOne or more pairs fail WCAG AA (%.1f:1 threshold)." % AA_THRESHOLD)
    return 1


if __name__ == "__main__":
    sys.exit(main())
