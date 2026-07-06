#!/usr/bin/env python3
"""
Convert PNG images to LVGL v9 RGB565A8 binary (.bin) format.

RGB565A8 stores 16-bit color pixels followed by 8-bit alpha values,
giving proper transparency without chroma-key hacks.

Usage:
    python3 tools/png_to_lvgl_bin.py assets/images/*.png

Output: for each input.png, writes input.bin in the same directory.
"""
import struct
import sys
import os
from PIL import Image

LV_IMAGE_HEADER_MAGIC = 0x19
LV_COLOR_FORMAT_RGB565A8 = 0x14  # RGB565 pixel data + A8 alpha map

def rgb888_to_rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

def convert_png_to_bin(png_path):
    img = Image.open(png_path).convert("RGBA")
    w, h = img.width, img.height
    pixels = img.load()

    rgb_raw = bytearray()
    alpha_raw = bytearray()
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            rgb_raw += struct.pack("<H", rgb888_to_rgb565(r, g, b))
            alpha_raw += struct.pack("B", a)

    header = struct.pack(
        "<BBHHHHH",
        LV_IMAGE_HEADER_MAGIC,
        LV_COLOR_FORMAT_RGB565A8,
        0,
        w, h,
        0,
        0,
    )

    bin_path = os.path.splitext(png_path)[0] + ".bin"
    with open(bin_path, "wb") as f:
        f.write(header + rgb_raw + alpha_raw)

    size_kb = len(header + rgb_raw + alpha_raw) / 1024
    print(f"  {os.path.basename(png_path):40s} → {os.path.basename(bin_path):40s}  ({w}x{h}, {size_kb:.1f}KB)")
    return bin_path

def main():
    paths = sys.argv[1:]
    if not paths:
        print("Usage: png_to_lvgl_bin.py <png files...>")
        sys.exit(1)

    for p in paths:
        if not p.lower().endswith(".png"):
            print(f"  Skipping {p} (not a PNG)")
            continue
        if not os.path.exists(p):
            print(f"  Skipping {p} (not found)")
            continue
        convert_png_to_bin(p)

    print("\nDone. Copy .bin files to SD card /images/ directory.")

if __name__ == "__main__":
    main()
