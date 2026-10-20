"""Builds the textures of the workshop menu icon from Logo/Logo.png.

  textures/LostObjects/MenuIcon_d.dds   the logo with its transparency (512x512)
  textures/LostObjects/MenuIcon_n.dds   flat normal map (64x64)
  tools/icon/MenuIcon_preview.png       what the diffuse map looks like

Uncompressed 32-bit DDS (A8R8G8B8) with mipmaps - every Fallout 4 version
reads it, no extra tools needed. Run from the repository root:
  python tools/icon/make_icon_textures.py
"""
import struct
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SIZE = 512
MARGIN = 8            # transparent border around the logo, in pixels


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
    """The logo exactly as in Logo.png (transparent background), centered."""
    logo = Image.open(ROOT / "Logo" / "Logo.png").convert("RGBA")
    box = logo.getbbox()  # crop the empty margins
    if box:
        logo = logo.crop(box)
    inner = SIZE - 2 * MARGIN
    scale = min(inner / logo.width, inner / logo.height)
    logo = logo.resize((max(1, round(logo.width * scale)), max(1, round(logo.height * scale))), Image.LANCZOS)

    # Transparent pixels get the logo's average colour, so the edges of the
    # letters do not turn dark in the smaller mipmaps.
    data = logo.get_flattened_data() if hasattr(logo, "get_flattened_data") else logo.getdata()
    pixels = [p for p in data if p[3] > 200]
    average = tuple(sum(p[i] for p in pixels) // max(1, len(pixels)) for i in range(3)) if pixels else (0, 0, 0)
    image = Image.new("RGBA", (SIZE, SIZE), average + (0,))
    image.alpha_composite(logo, ((SIZE - logo.width) // 2, (SIZE - logo.height) // 2))
    return image


def main() -> int:
    out = ROOT / "data" / "Textures" / "LostObjects"
    out.mkdir(parents=True, exist_ok=True)

    image = diffuse()
    (out / "MenuIcon_d.dds").write_bytes(dds_bytes(image))
    image.save(ROOT / "tools" / "icon" / "MenuIcon_preview.png")

    normal = Image.new("RGBA", (64, 64), (128, 128, 255, 255))
    (out / "MenuIcon_n.dds").write_bytes(dds_bytes(normal))

    print("wrote", out / "MenuIcon_d.dds", "and", out / "MenuIcon_n.dds")
    return 0


if __name__ == "__main__":
    sys.exit(main())
