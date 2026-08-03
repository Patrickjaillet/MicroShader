"""golf.md Phase 33.4 -- optional, dev-only, offline size comparison.

Compares uShader's golfed output size against a locally installed copy of
Shader Minifier (https://github.com/laurentlb/shader-minifier) on the
fixtures/*.glsl corpus. Informational only: never a merge gate, never run
by CI, never fetches anything over the network, and never built into
ushader.exe -- this script is not shipped and requires the contributor to
have both `cargo` and their own separately-installed `shader_minifier.exe`
(or `mono shader_minifier.exe` on non-Windows) already on PATH. See the
Offline-First corollary and Phase 33.4 in golf.md for the scope this
script deliberately stays inside.

Usage:
    python scripts/benchmark_vs_shader_minifier.py [--shader-minifier PATH]

If shader_minifier is not found on PATH (and --shader-minifier is not
given), the script still prints uShader's own sizes and notes that the
comparison column was skipped, rather than failing.
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
FIXTURES_DIR = REPO_ROOT / "fixtures"
RUST_CORE_DIR = REPO_ROOT / "rust-core"


def find_shader_minifier(explicit_path):
    if explicit_path:
        return explicit_path
    found = shutil.which("shader_minifier") or shutil.which("shader_minifier.exe")
    return found


def run_ushader_golf(fixture_path):
    result = subprocess.run(
        ["cargo", "run", "--release", "--bin", "golf", "--", "-a", str(fixture_path)],
        cwd=RUST_CORE_DIR,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        return None
    return len(result.stdout.encode("utf-8"))


def run_shader_minifier(shader_minifier_exe, fixture_path, tmp_out):
    command = [shader_minifier_exe, str(fixture_path), "-o", str(tmp_out)]
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode != 0 or not tmp_out.exists():
        return None
    size = tmp_out.stat().st_size
    tmp_out.unlink(missing_ok=True)
    return size


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shader-minifier", help="Path to shader_minifier(.exe), if not on PATH")
    args = parser.parse_args()

    shader_minifier_exe = find_shader_minifier(args.shader_minifier)
    if shader_minifier_exe is None:
        print("shader_minifier not found on PATH -- printing uShader sizes only.", file=sys.stderr)
        print("Install it separately and pass --shader-minifier PATH to compare.", file=sys.stderr)

    fixtures = sorted(FIXTURES_DIR.glob("*.glsl"))
    if not fixtures:
        print(f"no .glsl fixtures found under {FIXTURES_DIR}", file=sys.stderr)
        return 1

    rows = []
    for fixture in fixtures:
        ushader_bytes = run_ushader_golf(fixture)
        minifier_bytes = None
        if shader_minifier_exe is not None:
            tmp_out = fixture.with_suffix(".shader_minifier_tmp.glsl")
            minifier_bytes = run_shader_minifier(shader_minifier_exe, fixture, tmp_out)
        rows.append((fixture.name, ushader_bytes, minifier_bytes))

    name_width = max(len(name) for name, _, _ in rows)
    header = f"{'fixture'.ljust(name_width)}  uShader  Shader Minifier  delta"
    print(header)
    print("-" * len(header))
    for name, ushader_bytes, minifier_bytes in rows:
        ushader_col = str(ushader_bytes) if ushader_bytes is not None else "ERROR"
        if minifier_bytes is None:
            minifier_col = "n/a"
            delta_col = "n/a"
        else:
            minifier_col = str(minifier_bytes)
            delta_col = str(ushader_bytes - minifier_bytes) if ushader_bytes is not None else "n/a"
        print(f"{name.ljust(name_width)}  {ushader_col.rjust(7)}  {minifier_col.rjust(15)}  {delta_col.rjust(5)}")

    print()
    print("Informational only -- not a merge gate. See golf.md Phase 33.4.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
