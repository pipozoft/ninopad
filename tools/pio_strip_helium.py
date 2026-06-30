# PIO build guard: LVGL's CMakeLists excludes the ARM-only Helium
# assembly files on non-ARM targets, but PlatformIO ignores CMakeLists
# and tries to assemble `lv_blend_helium.S` on Xtensa. Remove the file
# before each ESP32 build (idempotent).
import os
import pathlib

Import("env")

proj_dir = env.subst("$PROJECT_DIR")
env_name = env.get("PIOENV", "esp32s3")
libdeps_dir = os.path.join(proj_dir, ".pio", "libdeps", env_name)

removed = []
for root, _, files in os.walk(libdeps_dir):
    for f in files:
        p = pathlib.Path(root) / f
        if "helium" in p.parts and p.suffix.lower() in (".s", ".sx"):
            try:
                p.unlink()
                removed.append(str(p))
            except FileNotFoundError:
                pass

if removed:
    print("[ninopad] stripped non-ARM Helium assembly sources:")
    for r in removed:
        print("  -", r)