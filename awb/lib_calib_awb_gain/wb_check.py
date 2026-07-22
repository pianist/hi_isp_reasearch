#!/usr/bin/env python3
"""Estimate white balance of a neutral JPEG using ImageMagick.

The photographed object must be white or neutral gray.  By default the script
ignores 10% at the left/right edges and 15% at the top/bottom.  An explicit
ImageMagick crop geometry can be supplied with --crop.

Examples:
    ./wb_check.py white.jpg
    ./wb_check.py --crop 800x500+200+100 white.jpg
    ./wb_check.py --zones white.jpg
"""

import argparse
import math
import shutil
import subprocess
import sys
from dataclasses import dataclass


@dataclass
class Stats:
    r: float
    g: float
    b: float
    lab_l: float
    lab_a: float
    lab_b: float


def run(command: list[str]) -> str:
    try:
        return subprocess.check_output(command, text=True, stderr=subprocess.PIPE).strip()
    except subprocess.CalledProcessError as exc:
        message = exc.stderr.strip() or str(exc)
        raise RuntimeError(message) from exc


def find_imagemagick() -> tuple[list[str], list[str]]:
    if shutil.which("magick"):
        return ["magick", "identify"], ["magick"]
    if shutil.which("identify") and shutil.which("convert"):
        return ["identify"], ["convert"]
    raise RuntimeError("ImageMagick not found (need magick, or identify + convert)")


def image_size(identify: list[str], filename: str) -> tuple[int, int]:
    text = run(identify + ["-format", "%w %h", filename])
    width, height = map(int, text.split())
    return width, height


def measure(convert: list[str], filename: str, crop: str) -> Stats:
    common = convert + [filename, "-crop", crop, "+repage"]
    rgb = run(common + [
        "-format",
        "%[fx:mean.r*255] %[fx:mean.g*255] %[fx:mean.b*255]",
        "info:",
    ])
    lab = run(common + [
        "-colorspace", "Lab",
        "-format",
        "%[fx:mean.r*100] %[fx:mean.g*255-128] %[fx:mean.b*255-128]",
        "info:",
    ])
    r, g, b = map(float, rgb.split())
    lab_l, lab_a, lab_b = map(float, lab.split())
    return Stats(r, g, b, lab_l, lab_a, lab_b)


def cast_description(a: float, b: float) -> str:
    chroma = math.hypot(a, b)
    if chroma < 1.0:
        return "practically neutral"

    components: list[str] = []
    if abs(a) >= 1.0:
        components.append("green" if a < 0 else "magenta")
    if abs(b) >= 1.0:
        components.append("blue" if b < 0 else "yellow")

    if chroma < 3.0:
        strength = "slight"
    elif chroma < 6.0:
        strength = "visible"
    else:
        strength = "strong"
    return f"{strength} {'-'.join(components) if components else 'color'} cast"


def print_stats(name: str, crop: str, s: Stats) -> None:
    rg = s.r / s.g
    bg = s.b / s.g
    gr = s.g / s.r
    gb = s.g / s.b

    # Relative correction to the current gains.  Green is the reference.
    corr_r, corr_g, corr_b = gr, 1.0, gb
    q8_gr = round(gr * 256)
    q8_gb = round(gb * 256)

    print(f"{name}  crop={crop}")
    print(f"  mean sRGB : R={s.r:7.2f}  G={s.g:7.2f}  B={s.b:7.2f}")
    print(f"  ratios    : R/G={rg:.4f}  B/G={bg:.4f}")
    print(f"  AWB-like  : G/R={gr:.4f} (0x{q8_gr:03x}), "
          f"G/B={gb:.4f} (0x{q8_gb:03x})")
    print(f"  Lab       : L*={s.lab_l:.2f}  a*={s.lab_a:+.2f}  b*={s.lab_b:+.2f}")
    print(f"  verdict   : {cast_description(s.lab_a, s.lab_b)}")
    print(f"  correction: GainR*={corr_r:.4f}, GainG*=1.0000, GainB*={corr_b:.4f}")


def default_crop(width: int, height: int) -> str:
    x = round(width * 0.10)
    y = round(height * 0.15)
    w = width - 2 * x
    h = height - 2 * y
    return f"{w}x{h}+{x}+{y}"


def zone_crops(width: int, height: int) -> list[tuple[str, str]]:
    x0 = round(width * 0.10)
    y0 = round(height * 0.15)
    usable_w = width - 2 * x0
    usable_h = height - 2 * y0
    zone_w = usable_w // 3
    return [
        ("left  ", f"{zone_w}x{usable_h}+{x0}+{y0}"),
        ("center", f"{zone_w}x{usable_h}+{x0 + zone_w}+{y0}"),
        ("right ", f"{usable_w - 2 * zone_w}x{usable_h}+{x0 + 2 * zone_w}+{y0}"),
    ]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Estimate WB from a white/neutral-gray JPEG using ImageMagick"
    )
    parser.add_argument("image", help="JPEG or other image understood by ImageMagick")
    parser.add_argument("--crop", metavar="WxH+X+Y", help="explicit measurement ROI")
    parser.add_argument(
        "--zones", action="store_true",
        help="also report left, center and right zones (disabled with --crop)",
    )
    args = parser.parse_args()

    try:
        identify, convert = find_imagemagick()
        width, height = image_size(identify, args.image)
        crop = args.crop or default_crop(width, height)
        print_stats("inner ", crop, measure(convert, args.image, crop))

        if args.zones and not args.crop:
            print()
            for name, zone in zone_crops(width, height):
                print_stats(name, zone, measure(convert, args.image, zone))
                print()
    except (OSError, RuntimeError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

