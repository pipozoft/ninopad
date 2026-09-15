#!/usr/bin/env python3
"""Create opaque RGB565 phonics pictures from the shared RGB565A8 word art."""

import struct
from pathlib import Path
from PIL import Image


MAGIC = 0x19
RGB565 = 0x12
RGB565A8 = 0x14
HEADER_SIZE = 12
SOUND_SIZE = (80, 60)
WORDS = """
map mop mug sun sock sail tap top train pan pig pot nest net nut ball bed bus
cap cat cup dog door duck fish fox frog hat hen house rain row run lamp lip log bug
""".split()


def expand_5(value):
    return (value << 3) | (value >> 2)


def expand_6(value):
    return (value << 2) | (value >> 4)


def flatten(source, destination, output_size=None):
    data = source.read_bytes()
    magic, color_format, flags, width, height, stride, reserved = struct.unpack_from(
        "<BBHHHHH", data
    )
    if magic != MAGIC or color_format != RGB565A8:
        raise ValueError(f"{source} is not an LVGL RGB565A8 image")

    color_stride = stride or width * 2
    color_size = color_stride * height
    alpha_offset = HEADER_SIZE + color_size
    alpha_stride = color_stride // 2
    if len(data) < alpha_offset + alpha_stride * height:
        raise ValueError(f"{source} is truncated")

    rgba = bytearray(width * height * 4)
    for y in range(height):
        for x in range(width):
            pixel = struct.unpack_from("<H", data, HEADER_SIZE + y * color_stride + x * 2)[0]
            alpha = data[alpha_offset + y * alpha_stride + x]
            red = expand_5((pixel >> 11) & 0x1F)
            green = expand_6((pixel >> 5) & 0x3F)
            blue = expand_5(pixel & 0x1F)
            offset = (y * width + x) * 4
            rgba[offset:offset + 4] = bytes((red, green, blue, alpha))

    image = Image.frombytes("RGBA", (width, height), bytes(rgba))
    if output_size:
        image = image.resize(output_size, Image.Resampling.LANCZOS)
    background = Image.new("RGB", image.size, "white")
    background.paste(image, mask=image.getchannel("A"))

    width, height = background.size
    output = bytearray(width * height * 2)
    pixels = background.load()
    for y in range(height):
        for x in range(width):
            red, green, blue = pixels[x, y]
            flattened = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
            struct.pack_into("<H", output, (y * width + x) * 2, flattened)

    header = struct.pack("<BBHHHHH", MAGIC, RGB565, flags, width, height, width * 2, reserved)
    destination.write_bytes(header + output)


def main():
    root = Path(__file__).resolve().parents[1]
    source_dir = root / "sd_card_content" / "words"
    output_dir = root / "sd_card_content" / "phonics" / "images"
    sound_dir = root / "sd_card_content" / "phonics" / "sounds"
    output_dir.mkdir(parents=True, exist_ok=True)
    sound_dir.mkdir(parents=True, exist_ok=True)

    for word in WORDS:
        source = source_dir / f"{word}.bin"
        destination = output_dir / source.name
        flatten(source, destination)
        sound_destination = sound_dir / source.name
        flatten(source, sound_destination, SOUND_SIZE)
        print(f"{source.name}: full {destination.stat().st_size}, "
              f"sound {sound_destination.stat().st_size} bytes")

    print(f"Created {len(WORDS)} full and sound-choice phonics images")


if __name__ == "__main__":
    main()
