#!/usr/bin/env python3
"""Convert BMP sprite frames to a C header for SSD1306 display.

Usage:
    python sprites/convert.py sprites/<character_name>

Expects BMP files named: SMALL_1.bmp, SMALL_2.bmp, MEDIUM_1.bmp, LARGE_1.bmp, etc.
Sizes are ordered: SMALL, MEDIUM, LARGE (matching sprite_size_t enum).
Frames within a size are ordered by number suffix.

Output: sprites/<character_name>/<character_name>.h

Requires: pip install Pillow
"""

import sys
import re
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required.\n  pip install Pillow")
    sys.exit(1)

SIZES = ["SMALL", "MEDIUM", "LARGE"]


def bmp_to_bitmap(path):
    """Convert a BMP to a 1bpp row-major byte array."""
    img = Image.open(path).convert("L")
    img = img.point(lambda p: 255 if p > 127 else 0, mode="1")
    w, h = img.size
    row_bytes = (w + 7) // 8
    data = []
    for y in range(h):
        for bx in range(row_bytes):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w and img.getpixel((x, y)):
                    byte |= 0x80 >> bit
            data.append(byte)
    return w, h, data


def find_frames(directory):
    """Find and group BMP files by size and frame number."""
    pattern = re.compile(r"^(SMALL|MEDIUM|LARGE)_(\d+)\.bmp$", re.IGNORECASE)
    groups = {s: [] for s in SIZES}

    for f in directory.iterdir():
        m = pattern.match(f.name)
        if m:
            size = m.group(1).upper()
            frame_num = int(m.group(2))
            groups[size].append((frame_num, f))

    # Sort each group by frame number
    for s in SIZES:
        groups[s].sort(key=lambda x: x[0])

    return groups


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} sprites/<character_name>")
        sys.exit(1)

    char_dir = Path(sys.argv[1])
    if not char_dir.is_dir():
        print(f"Not a directory: {char_dir}")
        sys.exit(1)

    name = char_dir.name
    guard = f"SPRITE_{name.upper()}_H"
    groups = find_frames(char_dir)

    total = sum(len(v) for v in groups.values())
    if total == 0:
        print(f"No BMP files found in {char_dir}")
        print(f"Expected files like: SMALL_1.bmp, MEDIUM_1.bmp, LARGE_1.bmp")
        sys.exit(1)

    lines = [
        f"// Auto-generated from sprites/{name}/ -- do not edit by hand",
        f"// Re-generate with: python sprites/convert.py sprites/{name}",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        '#include "sprite.h"',
        "",
    ]

    # Emit frame data for each size
    for size in SIZES:
        frames = groups[size]
        if not frames:
            continue

        for frame_num, path in frames:
            w, h, data = bmp_to_bitmap(path)
            var = f"sprite_{name}_{size.lower()}_{frame_num}"

            lines.append(f"// {path.name}: {w}x{h}")
            lines.append(f"static const uint8_t {var}_data[] = {{")
            for i in range(0, len(data), 16):
                chunk = data[i : i + 16]
                lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
            lines.append("};")
            lines.append("")

        # Emit frames array for this size
        arr_name = f"sprite_{name}_{size.lower()}_frames"
        lines.append(f"static const sprite_frame_t {arr_name}[] = {{")
        for frame_num, path in frames:
            w, h, _ = bmp_to_bitmap(path)
            var = f"sprite_{name}_{size.lower()}_{frame_num}"
            lines.append(f"    {{ {w}, {h}, {var}_data }},")
        lines.append("};")
        lines.append("")

    # Emit the sprite_t struct
    lines.append(f"static const sprite_t sprite_{name} = {{")
    lines.append(f'    .name = "{name}",')
    lines.append(f"    .sizes = {{")
    for size in SIZES:
        frames = groups[size]
        arr_name = f"sprite_{name}_{size.lower()}_frames"
        if frames:
            lines.append(f"        [{size_enum(size)}] = {{ {len(frames)}, {arr_name} }},")
        else:
            lines.append(f"        [{size_enum(size)}] = {{ 0, NULL }},")
    lines.append("    },")
    lines.append("};")
    lines.append("")
    lines.append(f"#endif // {guard}")
    lines.append("")

    output = char_dir / f"{name}.h"
    output.write_text("\n".join(lines))

    print(f"Generated {output}")
    for size in SIZES:
        n = len(groups[size])
        if n:
            print(f"  {size}: {n} frame(s)")


def size_enum(size):
    return f"SPRITE_{size.upper()}"


if __name__ == "__main__":
    main()
