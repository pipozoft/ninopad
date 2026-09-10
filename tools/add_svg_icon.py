#!/usr/bin/env python3
"""Add SVG paths as a Unicode glyph in a TrueType icon font."""

import argparse
import xml.etree.ElementTree as ET

from fontTools.pens.transformPen import TransformPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.svgLib.path import parse_path
from fontTools.ttLib import TTFont


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("font")
    parser.add_argument("svg")
    parser.add_argument("codepoint", type=lambda value: int(value, 0))
    parser.add_argument("glyph_name")
    args = parser.parse_args()

    font = TTFont(args.font)
    root = ET.parse(args.svg).getroot()
    view_box = [float(value) for value in root.attrib["viewBox"].split()]
    x, y, width, height = view_box
    scale = 800 / max(width, height)
    x_offset = 100 + (800 - width * scale) / 2 - x * scale
    y_offset = 900 - (800 - height * scale) / 2 + y * scale

    pen = TTGlyphPen(font.getGlyphSet())
    transform_pen = TransformPen(pen, (scale, 0, 0, -scale, x_offset, y_offset))
    paths = root.findall(".//{http://www.w3.org/2000/svg}path")
    if not paths:
        raise ValueError("SVG contains no paths")
    for path in paths:
        parse_path(path.attrib["d"], transform_pen)

    glyph_name = font.getBestCmap().get(args.codepoint, args.glyph_name)
    glyph_order = font.getGlyphOrder()
    if glyph_name not in glyph_order:
        font.setGlyphOrder(glyph_order + [glyph_name])
    glyph = pen.glyph()
    glyph.recalcBounds(font["glyf"])
    font["glyf"][glyph_name] = glyph
    font["hmtx"][glyph_name] = (font["head"].unitsPerEm, glyph.xMin)
    for table in font["cmap"].tables:
        if table.isUnicode():
            table.cmap[args.codepoint] = glyph_name
    font.save(args.font)


if __name__ == "__main__":
    main()
