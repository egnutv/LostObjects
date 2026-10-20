"""Builds nexus/header.png (1920x1080): the logo on a dark background with a
one-line subtitle, for the Nexus Mods gallery. Optional - the logo itself stays
exactly as Logo/Logo.png. Run from the repository root:
  python tools/icon/make_nexus_header.py
"""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
W, H = 1920, 1080
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONT_BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def font(path: str, size: int) -> ImageFont.ImageFont:
    try:
        return ImageFont.truetype(path, size)
    except OSError:
        return ImageFont.load_default()


def main() -> None:
    image = Image.new("RGBA", (W, H))
    draw = ImageDraw.Draw(image)
    for y in range(H):  # dark vertical gradient
        t = y / (H - 1)
        c = (int(22 + 10 * t), int(20 + 6 * t), int(30 + 12 * t), 255)
        draw.line([(0, y), (W, y)], fill=c)

    logo = Image.open(ROOT / "Logo" / "Logo.png").convert("RGBA")
    box = logo.getbbox()
    if box:
        logo = logo.crop(box)
    scale = min(1300 / logo.width, 560 / logo.height)
    logo = logo.resize((round(logo.width * scale), round(logo.height * scale)), Image.LANCZOS)
    image.alpha_composite(logo, ((W - logo.width) // 2, 170))

    lines = [
        (font(FONT_BOLD, 54), "Every lost workshop item - back where it belongs", (235, 235, 240, 255), 800),
        (font(FONT, 34), "Fallout 4 1.11.240  |  F4SE plugin  |  settings holotape  |  safe to remove", (175, 170, 190, 255), 885),
    ]
    for f, text, color, y in lines:
        width = draw.textlength(text, font=f)
        draw.text(((W - width) / 2, y), text, font=f, fill=color)

    out = ROOT / "nexus" / "header.png"
    image.convert("RGB").save(out)
    print("wrote", out)


if __name__ == "__main__":
    main()
