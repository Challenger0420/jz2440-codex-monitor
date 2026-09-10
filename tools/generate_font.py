#!/usr/bin/env python3
"""Rasterize the approved build-time font into C bitmap tables.

The target never parses a font file.  Pillow is a host build dependency only.
Glyph bitmaps are cropped to visible pixels; ``advance`` retains the font's
horizontal metrics so the target renderer can measure and center text.
"""

from pathlib import Path
import argparse
import re

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_GLYPHS = " 0123456789%/:.-ABCDEFGHIJKLMNOPQRSTUVWXYZ"
ONE_BIT_THRESHOLD = 128


def c_ident(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", value)


def rasterize(font_path: Path, size: int, glyphs: str, bitmap_format: str):
    font = ImageFont.truetype(str(font_path), size=size)
    out = []
    for ch in glyphs:
        bbox = font.getbbox(ch)
        if bbox is None:
            left = top = right = bottom = 0
        else:
            left, top, right, bottom = bbox
        width = max(0, right - left)
        height = max(0, bottom - top)
        rows = []
        if width and height:
            # Always rasterize through grayscale.  Alpha roles retain every
            # 8-bit sample; the working 1-bit role applies the explicit 128
            # threshold below.  This avoids treating mode-1 values (0/1) as
            # 8-bit values and accidentally erasing the glyphs.
            mask = Image.new("L", (width, height), 0)
            ImageDraw.Draw(mask).text((-left, -top), ch, font=font, fill=255)
            for row in range(height):
                if bitmap_format == "alpha":
                    rows.append([mask.getpixel((col, row)) for col in range(width)])
                else:
                    value = 0
                    for col in range(width):
                        if mask.getpixel((col, row)) >= ONE_BIT_THRESHOLD:
                            value |= 1 << (width - 1 - col)
                    rows.append(value)
        advance = max(1, int(round(font.getlength(ch))))
        out.append({"ch": ch, "width": width, "height": height, "advance": advance,
                    "rows": rows, "format": bitmap_format})
    return out


def emit_font(name: str, font_path: Path, size: int, glyphs: str, bitmap_format: str):
    ident = c_ident(name)
    glyph_data = rasterize(font_path, size, glyphs, bitmap_format)
    lines = [f"/* {name}: {font_path.name}, {size}px, {bitmap_format} */"]
    for index, glyph in enumerate(glyph_data):
        width = glyph["width"]
        if glyph["format"] == "alpha":
            rows = [value for row in glyph["rows"] for value in row]
            format_value = "FONT_BITMAP_ALPHA"
        else:
            stride = (width + 7) // 8
            rows = []
            for value in glyph["rows"]:
                row_bytes = [(value >> (8 * (stride - 1 - i))) & 0xff for i in range(stride)]
                rows.extend(row_bytes)
            format_value = "FONT_BITMAP_1BIT"
        if not rows:
            rows = [0]
        values = ", ".join(f"0x{value:02x}" for value in rows)
        lines.append(f"static const uint8_t font_{ident}_g{index}_data[] = {{{values}}};")
    lines.append(f"static const struct font_glyph font_{ident}_glyphs[] = {{")
    for index, glyph in enumerate(glyph_data):
        ch = glyph["ch"]
        code = ord(ch)
        lines.append(
            f"    {{{code}, {glyph['width']}, {glyph['height']}, {glyph['advance']}, "
            f"{format_value}, font_{ident}_g{index}_data}},"
        )
    lines.append("};")
    lines.append(
        f"static const struct font_face font_{ident} = "
        f"{{{size}, font_{ident}_glyphs, {len(glyph_data)}}};"
    )
    return lines


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=ROOT / "src" / "font_data.h")
    parser.add_argument("--bold", type=Path, default=ROOT / "tools" / "fonts" / "Oxanium-Bold.ttf")
    parser.add_argument("--semibold", type=Path, default=ROOT / "tools" / "fonts" / "Oxanium-SemiBold.ttf")
    args = parser.parse_args()

    roles = [
        ("header", args.bold, 28, DEFAULT_GLYPHS, "alpha"),
        ("label", args.semibold, 15, DEFAULT_GLYPHS, "alpha"),
        # Keep the large-number map explicit: '%' is a real glyph in this
        # role, never a fallback to '/' or a legacy 5x7 implementation.
        ("large_number", args.bold, 42, "0123456789%-", "1bit"),
        ("reset_time", args.semibold, 27, DEFAULT_GLYPHS, "alpha"),
    ]
    lines = [
        "#ifndef JZ2440_FONT_DATA_H",
        "#define JZ2440_FONT_DATA_H",
        "",
        "#include <stdint.h>",
        "",
        "#define FONT_BITMAP_1BIT 0",
        "#define FONT_BITMAP_ALPHA 1",
        "",
        "struct font_glyph {",
        "    uint8_t code;",
        "    uint8_t width;",
        "    uint8_t height;",
        "    uint8_t advance;",
        "    uint8_t format;",
        "    const uint8_t *data;",
        "};",
        "",
        "struct font_face {",
        "    uint8_t nominal_size;",
        "    const struct font_glyph *glyphs;",
        "    uint8_t count;",
        "};",
        "",
    ]
    for name, path, size, glyphs, bitmap_format in roles:
        if not path.exists():
            raise SystemExit(f"font not found: {path}")
        lines.extend(emit_font(name, path, size, glyphs, bitmap_format))
        lines.append("")
    lines.append("#endif")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines) + "\n", encoding="ascii")
    print(args.output)


if __name__ == "__main__":
    main()
