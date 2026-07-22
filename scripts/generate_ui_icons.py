import math
from pathlib import Path

from PIL import Image, ImageDraw

OUT_DIR = Path(__file__).resolve().parent.parent / "assets" / "icons" / "ui"
SIZES = [16, 20, 24, 32]
SUPERSAMPLE = 8
CANVAS = 32
STROKE = 2.6


def canvas():
    size = CANVAS * SUPERSAMPLE
    return Image.new("RGBA", (size, size), (0, 0, 0, 0)), size


def s(value):
    return value * SUPERSAMPLE


def line(draw, size, p0, p1, width=STROKE):
    draw.line([s(p0[0]), s(p0[1]), s(p1[0]), s(p1[1])], fill=(255, 255, 255, 255), width=round(s(width)), joint="curve")


def polyline(draw, size, points, width=STROKE):
    for a, b in zip(points, points[1:]):
        line(draw, size, a, b, width)


def circle(draw, size, center, radius, width=STROKE, fill=False):
    x, y = center
    bbox = [s(x - radius), s(y - radius), s(x + radius), s(y + radius)]
    if fill:
        draw.ellipse(bbox, fill=(255, 255, 255, 255))
    else:
        draw.ellipse(bbox, outline=(255, 255, 255, 255), width=round(s(width)))


def rounded_rect(draw, size, box, radius, width=STROKE, fill=False):
    bbox = [s(box[0]), s(box[1]), s(box[2]), s(box[3])]
    if fill:
        draw.rounded_rectangle(bbox, radius=s(radius), fill=(255, 255, 255, 255))
    else:
        draw.rounded_rectangle(bbox, radius=s(radius), outline=(255, 255, 255, 255), width=round(s(width)))


def dot(draw, size, center, radius=1.3):
    circle(draw, size, center, radius, fill=True)


def draw_play(draw, size):
    draw.polygon([(s(11), s(7)), (s(11), s(25)), (s(25), s(16))], fill=(255, 255, 255, 255))


def draw_circle_alert(draw, size):
    circle(draw, size, (16, 16), 11)
    line(draw, size, (16, 9), (16, 18))
    dot(draw, size, (16, 22))


def draw_code(draw, size):
    polyline(draw, size, [(12, 9), (6, 16), (12, 23)])
    polyline(draw, size, [(20, 9), (26, 16), (20, 23)])


def draw_image(draw, size):
    rounded_rect(draw, size, (5, 6, 27, 26), 3)
    circle(draw, size, (12, 13), 2.4)
    polyline(draw, size, [(7, 23), (13, 16), (18, 21), (22, 15), (26, 20)])


def draw_folder_open(draw, size):
    draw.polygon([
        (s(5), s(11)), (s(13), s(11)), (s(15), s(14)), (s(27), s(14)),
        (s(24), s(25)), (s(4), s(25)),
    ], outline=(255, 255, 255, 255), width=round(s(STROKE)))
    polyline(draw, size, [(5, 11), (5, 8), (13, 8), (15, 11)])


def draw_save(draw, size):
    rounded_rect(draw, size, (6, 5, 26, 27), 2)
    rounded_rect(draw, size, (10, 5, 22, 12), 1)
    rounded_rect(draw, size, (10, 18, 22, 27), 1)


def draw_copy(draw, size):
    rounded_rect(draw, size, (11, 11, 27, 27), 2.5)
    draw.line([s(9), s(21), s(5), s(21), s(5), s(5), s(21), s(5), s(21), s(9)],
              fill=(255, 255, 255, 255), width=round(s(STROKE)), joint="curve")


def draw_download(draw, size):
    line(draw, size, (16, 5), (16, 19))
    polyline(draw, size, [(10, 14), (16, 20), (22, 14)])
    polyline(draw, size, [(6, 24), (6, 27), (26, 27), (26, 24)])


def draw_camera(draw, size):
    rounded_rect(draw, size, (4, 10, 28, 25), 3)
    draw.polygon([(s(11), s(10)), (s(13), s(7)), (s(19), s(7)), (s(21), s(10))], fill=(255, 255, 255, 255))
    circle(draw, size, (16, 17), 4.5)


def draw_info(draw, size):
    circle(draw, size, (16, 16), 11)
    dot(draw, size, (16, 11))
    line(draw, size, (16, 15), (16, 22))


def draw_circle(draw, size):
    circle(draw, size, (16, 16), 11)


def draw_circle_stop(draw, size):
    circle(draw, size, (16, 16), 11)
    rounded_rect(draw, size, (12, 12, 20, 20), 1.5, fill=True)


def draw_minus(draw, size):
    line(draw, size, (7, 16), (25, 16))


def draw_square(draw, size):
    rounded_rect(draw, size, (7, 7, 25, 25), 2)


def draw_x(draw, size):
    line(draw, size, (8, 8), (24, 24))
    line(draw, size, (24, 8), (8, 24))


ICONS = {
    "play": draw_play,
    "circle_alert": draw_circle_alert,
    "code": draw_code,
    "image": draw_image,
    "folder_open": draw_folder_open,
    "save": draw_save,
    "copy": draw_copy,
    "download": draw_download,
    "camera": draw_camera,
    "info": draw_info,
    "circle": draw_circle,
    "circle_stop": draw_circle_stop,
    "minus": draw_minus,
    "square": draw_square,
    "x": draw_x,
}


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for name, fn in ICONS.items():
        img, size = canvas()
        draw = ImageDraw.Draw(img)
        fn(draw, size)
        for target in SIZES:
            resized = img.resize((target, target), Image.LANCZOS)
            resized.save(OUT_DIR / f"{name}_{target}.png")
    print("generated %d icons x %d scales in %s" % (len(ICONS), len(SIZES), OUT_DIR))


if __name__ == "__main__":
    main()
