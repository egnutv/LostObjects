"""Builds the textures of the workshop menu icon from Logo/Logo.png.

  textures/LostObjects/MenuIcon_d.dds   picture frame with the logo (512x512)
  textures/LostObjects/MenuIcon_n.dds   flat normal map (64x64)
  tools/icon/MenuIcon_preview.png       what the diffuse map looks like

Uncompressed 32-bit DDS (A8R8G8B8) with mipmaps - every Fallout 4 version
reads it, no extra tools needed. Run from the repository root:
  python tools/icon/make_icon_textures.py
"""
import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
SIZE = 512
FRAME = 40            # frame border in pixels (the mesh edges sample this color)
FRAME_COLOR = (74, 52, 34, 255)      # dark wood
FRAME_LIGHT = (112, 82, 54, 255)     # inner bevel
CANVAS = (236, 230, 216, 255)        # off-white canvas


def dds_bytes(image: Image.Image) -> bytes:
    image = image.convert("RGBA")
    width, height = image.size
    levels = []
    level = image
    while True:
        r, g, b, a = level.split()
        levels.append(Image.merge("RGBA", (b, g, r, a)).tobytes())  # BGRA in memory = A8R8G8B8
        if level.size == (1, 1):
            break
        level = level.resize((max(1, level.size[0] // 2), max(1, level.size[1] // 2)), Image.LANCZOS)

    DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH, DDSD_PITCH, DDSD_PIXELFORMAT, DDSD_MIPMAPCOUNT = 0x1, 0x2, 0x4, 0x8, 0x1000, 0x20000
    DDPF_ALPHAPIXELS, DDPF_RGB = 0x1, 0x40
    DDSCAPS_COMPLEX, DDSCAPS_TEXTURE, DDSCAPS_MIPMAP = 0x8, 0x1000, 0x400000

    header = struct.pack(
        "<4s7I44x" + "2I4s5I" + "4I4x",
        b"DDS ", 124,
        DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT,
        height, width, width * 4, 0, len(levels),
        32, DDPF_RGB | DDPF_ALPHAPIXELS, b"\0\0\0\0", 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000,
        DDSCAPS_COMPLEX | DDSCAPS_TEXTURE | DDSCAPS_MIPMAP, 0, 0, 0,
    )
    assert len(header) == 128, len(header)
    return header + b"".join(levels)


def diffuse() -> Image.Image:
    image = Image.new("RGBA", (SIZE, SIZE), FRAME_COLOR)
    draw = ImageDraw.Draw(image)
    draw.rectangle([FRAME - 8, FRAME - 8, SIZE - FRAME + 7, SIZE - FRAME + 7], fill=FRAME_LIGHT)
    draw.rectangle([FRAME, FRAME, SIZE - FRAME - 1, SIZE - FRAME - 1], fill=CANVAS)

    logo = Image.open(ROOT / "Logo" / "Logo.png").convert("RGBA")
    box = logo.getbbox()  # crop transparent margins
    if box:
        logo = logo.crop(box)
    inner = SIZE - 2 * FRAME - 40
    scale = min(inner / logo.width, inner / logo.height)
    logo = logo.resize((max(1, int(logo.width * scale)), max(1, int(logo.height * scale))), Image.LANCZOS)
    image.alpha_composite(logo, ((SIZE - logo.width) // 2, (SIZE - logo.height) // 2))
    image.putalpha(255)  # fully opaque, the mesh uses no alpha
    return image


def main() -> int:
    out = ROOT / "data" / "Textures" / "LostObjects"
    out.mkdir(parents=True, exist_ok=True)

    image = diffuse()
    (out / "MenuIcon_d.dds").write_bytes(dds_bytes(image))
    image.convert("RGB").save(ROOT / "tools" / "icon" / "MenuIcon_preview.png")

    normal = Image.new("RGBA", (64, 64), (128, 128, 255, 255))
    (out / "MenuIcon_n.dds").write_bytes(dds_bytes(normal))

    print("wrote", out / "MenuIcon_d.dds", "and", out / "MenuIcon_n.dds")
    return 0


if __name__ == "__main__":
    sys.exit(main())
