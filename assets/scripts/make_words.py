#!/usr/bin/env python3
"""
Download OpenMoji 618x618 PNGs and convert to LVGL v9 .bin files (RGB565A8, 128x96).

Emoji artwork by OpenMoji (CC BY-SA 4.0) — https://openmoji.org/

Usage: python3 make_words.py
Output: assets/words/{word}.bin
"""

import json, os, struct, sys, urllib.request

from PIL import Image
from io import BytesIO

# ---- LVGL constants ----
LV_IMAGE_HEADER_MAGIC = 0x19
LV_COLOR_FORMAT_RGB565A8 = 0x14

W = 128
H = 96
OUTDIR = os.path.join(os.path.dirname(__file__), "..", "words")

# ---- Word list ----
# Each word has its OpenMoji codepoint (hex, uppercase).
# The "wrong" partner is the phonetic decoy.
WORDS = {
    # === 3-letter words (40) ===
    "dog":  {"cp": "1F436", "wrong": "jog", "level": 3},
    "pig":  {"cp": "1F437", "wrong": "dig", "level": 3},
    "cat":  {"cp": "1F431", "wrong": "mat", "level": 3},
    "fox":  {"cp": "1F98A", "wrong": "vex", "level": 3},
    "hen":  {"cp": "1F414", "wrong": "ten", "level": 3},
    "bug":  {"cp": "1F41B", "wrong": "beg", "level": 3},
    "mug":  {"cp": "2615",  "wrong": "rug", "level": 3},
    "log":  {"cp": "1FAB5", "wrong": "fog", "level": 3},
    "map":  {"cp": "1F5FA", "wrong": "rap", "level": 3},
    "hat":  {"cp": "1F3A9", "wrong": "pat", "level": 3},
    "bed":  {"cp": "1F6CF", "wrong": "fed", "level": 3},
    "sun":  {"cp": "2600",  "wrong": "fun", "level": 3},
    "van":  {"cp": "1F690", "wrong": "man", "level": 3},
    "cup":  {"cp": "1F964", "wrong": "pup", "level": 3},
    "pot":  {"cp": "1F372", "wrong": "hot", "level": 3},
    "bus":  {"cp": "1F68C", "wrong": "mud", "level": 3},
    "net":  {"cp": "1F945", "wrong": "pet", "level": 3},
    "cap":  {"cp": "1F9E2", "wrong": "gap", "level": 3},
    "cow":  {"cp": "1F404", "wrong": "how", "level": 3},
    "bee":  {"cp": "1F41D", "wrong": "see", "level": 3},
    # +20 more
    "ant":  {"cp": "1F41C", "wrong": "and", "level": 3},
    "axe":  {"cp": "1FA93", "wrong": "ask", "level": 3},
    "boy":  {"cp": "1F466", "wrong": "joy", "level": 3},
    "car":  {"cp": "1F697", "wrong": "far", "level": 3},
    "egg":  {"cp": "1F95A", "wrong": "leg", "level": 3},
    "fly":  {"cp": "1FAB0", "wrong": "fry", "level": 3},
    "jam":  {"cp": "1FAD0", "wrong": "ham", "level": 3},
    "jar":  {"cp": "1FAD9", "wrong": "bar", "level": 3},
    "lip":  {"cp": "1F444", "wrong": "tip", "level": 3},
    "mop":  {"cp": "1F9F9", "wrong": "hop", "level": 3},
    "nut":  {"cp": "1F95C", "wrong": "but", "level": 3},
    "pan":  {"cp": "1F373", "wrong": "can", "level": 3},
    "pie":  {"cp": "1F967", "wrong": "tie", "level": 3},
    "pin":  {"cp": "1F4CD", "wrong": "win", "level": 3},
    "rat":  {"cp": "1F400", "wrong": "sat", "level": 3},
    "row":  {"cp": "1F6A3", "wrong": "bow", "level": 3},
    "run":  {"cp": "1F3C3", "wrong": "fun", "level": 3},
    "sea":  {"cp": "1F30A", "wrong": "see", "level": 3},
    "tap":  {"cp": "1F6B0", "wrong": "nap", "level": 3},
    "top":  {"cp": "1FA80", "wrong": "mop", "level": 3},
    # === 4-letter words (40) ===
    "fish": {"cp": "1F41F", "wrong": "dish", "level": 4},
    "cake": {"cp": "1F370", "wrong": "bake", "level": 4},
    "book": {"cp": "1F4D6", "wrong": "look", "level": 4},
    "bird": {"cp": "1F426", "wrong": "word", "level": 4},
    "lamp": {"cp": "1F4A1", "wrong": "camp", "level": 4},
    "milk": {"cp": "1F95B", "wrong": "silk", "level": 4},
    "nest": {"cp": "1FAB6", "wrong": "rest", "level": 4},
    "ring": {"cp": "1F48D", "wrong": "sing", "level": 4},
    "sail": {"cp": "26F5",  "wrong": "tail", "level": 4},
    "bell": {"cp": "1F514", "wrong": "sell", "level": 4},
    "door": {"cp": "1F6AA", "wrong": "floor", "level": 4},
    "hand": {"cp": "270B",  "wrong": "sand", "level": 4},
    "kite": {"cp": "1FA81", "wrong": "bite", "level": 4},
    "sock": {"cp": "1F9E6", "wrong": "lock", "level": 4},
    "star": {"cp": "2B50",  "wrong": "scar", "level": 4},
    "moon": {"cp": "1F319", "wrong": "noon", "level": 4},
    "rain": {"cp": "1F327", "wrong": "pain", "level": 4},
    "tree": {"cp": "1F333", "wrong": "free", "level": 4},
    "wolf": {"cp": "1F43A", "wrong": "woof", "level": 4},
    "frog": {"cp": "1F438", "wrong": "fog",  "level": 4},
    # +20 more
    "ball": {"cp": "26BD",  "wrong": "call", "level": 4},
    "barn": {"cp": "1F3E1", "wrong": "warn", "level": 4},
    "bath": {"cp": "1F6C1", "wrong": "math", "level": 4},
    "bear": {"cp": "1F9F8", "wrong": "fear", "level": 4},
    "bike": {"cp": "1F6B2", "wrong": "like", "level": 4},
    "crab": {"cp": "1F980", "wrong": "grab", "level": 4},
    "duck": {"cp": "1F986", "wrong": "luck", "level": 4},
    "fire": {"cp": "1F525", "wrong": "wire", "level": 4},
    "food": {"cp": "1F37D", "wrong": "mood", "level": 4},
    "gift": {"cp": "1F381", "wrong": "lift", "level": 4},
    "goat": {"cp": "1F410", "wrong": "coat", "level": 4},
    "gold": {"cp": "1F947", "wrong": "mold", "level": 4},
    "king": {"cp": "1F451", "wrong": "wing", "level": 4},
    "lion": {"cp": "1F981", "wrong": "line", "level": 4},
    "lock": {"cp": "1F512", "wrong": "dock", "level": 4},
    "nose": {"cp": "1F443", "wrong": "rose", "level": 4},
    "owl":  {"cp": "1F989", "wrong": "foul", "level": 4},
    "pear": {"cp": "1F350", "wrong": "tear", "level": 4},
    "ship": {"cp": "1F6A2", "wrong": "chip", "level": 4},
    "snow": {"cp": "26C4",  "wrong": "blow", "level": 4},
    # === 5-letter words (30) ===
    "house":  {"cp": "1F3E0", "wrong": "mouse",  "level": 5},
    "bread":  {"cp": "1F35E", "wrong": "thread", "level": 5},
    "candy":  {"cp": "1F36C", "wrong": "handy",  "level": 5},
    "clock":  {"cp": "23F0",  "wrong": "block",  "level": 5},
    "crown":  {"cp": "1F451", "wrong": "brown",  "level": 5},
    "grape":  {"cp": "1F347", "wrong": "shape",  "level": 5},
    "heart":  {"cp": "2764",  "wrong": "smart",  "level": 5},
    "lemon":  {"cp": "1F34B", "wrong": "demon",  "level": 5},
    "pizza":  {"cp": "1F355", "wrong": "wizza",  "level": 5},
    "sheep":  {"cp": "1F411", "wrong": "jeep",   "level": 5},
    "tiger":  {"cp": "1F42F", "wrong": "lion",   "level": 5},
    "train":  {"cp": "1F686", "wrong": "plane",  "level": 5},
    "ghost":  {"cp": "1F47B", "wrong": "toast",  "level": 5},
    "mouse":  {"cp": "1F42D", "wrong": "house",  "level": 5},
    "ocean":  {"cp": "1F30A", "wrong": "motion", "level": 5},
    "onion":  {"cp": "1F9C5", "wrong": "union",  "level": 5},
    # +14 more
    "apple":  {"cp": "1F34E", "wrong": "maple",  "level": 5},
    "beach":  {"cp": "1F3D6", "wrong": "reach",  "level": 5},
    "brain":  {"cp": "1F9E0", "wrong": "crane",  "level": 5},
    "chair":  {"cp": "1FA91", "wrong": "chain",  "level": 5},
    "cloud":  {"cp": "2601",  "wrong": "loud",   "level": 5},
    "happy":  {"cp": "1F60A", "wrong": "sappy",  "level": 5},
    "horse":  {"cp": "1F434", "wrong": "worse",  "level": 5},
    "phone":  {"cp": "1F4F1", "wrong": "tone",   "level": 5},
    "shell":  {"cp": "1F41A", "wrong": "bell",   "level": 5},
    "shirt":  {"cp": "1F455", "wrong": "skirt",  "level": 5},
    "skate":  {"cp": "26F8",  "wrong": "gate",   "level": 5},
    "snake":  {"cp": "1F40D", "wrong": "snack",  "level": 5},
    "whale":  {"cp": "1F40B", "wrong": "while",  "level": 5},
    "wheat":  {"cp": "1F33E", "wrong": "cheat",  "level": 5},
}


# ---- Conversion ----

def download_png(cp):
    url = f"https://raw.githubusercontent.com/hfg-gmuend/openmoji/master/color/618x618/{cp}.png"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=30) as resp:
            return resp.read()
    except Exception as e:
        print(f"  FAIL download {cp}: {e}")
        return None


def center_fit(png_bytes, tw, th):
    img = Image.open(BytesIO(png_bytes)).convert("RGBA")
    iw, ih = img.size
    scale = min(tw / iw, th / ih)
    nw = int(iw * scale)
    nh = int(ih * scale)
    img = img.resize((nw, nh), Image.LANCZOS)
    canvas = Image.new("RGBA", (tw, th), (0, 0, 0, 0))
    ox = (tw - nw) // 2
    oy = (th - nh) // 2
    canvas.paste(img, (ox, oy), img)
    return canvas


def make_lvgl_bin(img, tw, th):
    stride = tw * 2  # RGB565 = 2 bytes/px
    header = struct.pack(
        "<BBH HH HH",
        LV_IMAGE_HEADER_MAGIC,
        LV_COLOR_FORMAT_RGB565A8,
        0,        # flags
        tw,       # width
        th,       # height
        stride,   # stride
        0,        # reserved
    )

    pixels = list(img.getdata())
    rgb_data = bytearray()
    alpha_data = bytearray()

    for r, g, b, a in pixels:
        rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        rgb_data += struct.pack("<H", rgb565)
        alpha_data.append(a)

    return header + bytes(rgb_data) + bytes(alpha_data)


# ---- Main ----

def main():
    os.makedirs(OUTDIR, exist_ok=True)

    results = {"ok": 0, "skip": 0, "fail": 0}
    failed = []

    # Sort by level then word
    words_sorted = sorted(WORDS.items(), key=lambda kv: (kv[1]["level"], kv[0]))

    for word, info in words_sorted:
        path = os.path.join(OUTDIR, f"{word}.bin")
        if os.path.exists(path):
            print(f"  [{info['level']}] {word}.bin — exists, skipping")
            results["skip"] += 1
            continue

        cp = info["cp"]
        print(f"  [{info['level']}] {word}.bin (U+{cp}) ... ", end="", flush=True)

        png = download_png(cp)
        if png is None:
            print("FAIL (download)")
            results["fail"] += 1
            failed.append(word)
            continue

        try:
            canvas = center_fit(png, W, H)
            data = make_lvgl_bin(canvas, W, H)
            with open(path, "wb") as f:
                f.write(data)
            print(f"OK ({len(data)} bytes)")
            results["ok"] += 1
        except Exception as e:
            print(f"FAIL ({e})")
            results["fail"] += 1
            failed.append(word)

    print(f"\nDone: {results['ok']} OK, {results['skip']} skipped, {results['fail']} failed")
    if failed:
        print(f"Failed: {', '.join(failed)}")

    # Write metadata JSON for the C++ code generator
    meta = []
    for word, info in WORDS.items():
        meta.append({
            "correct": word,
            "wrong": info["wrong"],
            "level": info["level"],
            "img": f"S:/words/{word}.bin",
        })
    # Group by level
    by_level = {}
    for m in meta:
        by_level.setdefault(m["level"], []).append(m)
    for lev in sorted(by_level):
        fname = os.path.join(OUTDIR, f"words_{lev}.json")
        with open(fname, "w") as f:
            json.dump(by_level[lev], f, indent=2)
        print(f"Wrote {fname} ({len(by_level[lev])} words)")


if __name__ == "__main__":
    main()
