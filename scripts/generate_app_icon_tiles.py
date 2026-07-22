from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "assets" / "icons" / "app_source.png"
TILE_DIR = ROOT / "assets" / "icons" / "app"
TILE_SIZES = [16, 32, 48, 256]


def main():
    source = Image.open(SOURCE).convert("RGBA")

    TILE_DIR.mkdir(parents=True, exist_ok=True)
    for size in TILE_SIZES:
        tile = source.resize((size, size), Image.LANCZOS)
        tile.save(TILE_DIR / f"app_{size}.png")

    ico_sizes = [(s, s) for s in TILE_SIZES]
    source.resize((256, 256), Image.LANCZOS).save(
        ROOT / "assets" / "icons" / "app.ico", sizes=ico_sizes)
    source.resize((256, 256), Image.LANCZOS).save(
        ROOT / "assets" / "icons" / "installer.ico", sizes=ico_sizes)

    print("wrote tiles for sizes %s to %s" % (TILE_SIZES, TILE_DIR))
    print("rebuilt app.ico and installer.ico with sizes %s" % (TILE_SIZES,))


if __name__ == "__main__":
    main()
