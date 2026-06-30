# NinoPad MVP — Implementation Plan

## Locked decisions
- **LVGL v9** with **Arduino_GFX ST7796 480×320 landscape** + **XPT2046 resistive touch** (existing HW). `touch/ft6236.cpp` kept as optional drop-in stub per spec.
- **PlatformIO** (`platformio.ini`, board `esp32-s3-devkitc-1`) + thin `.ino` wrapper for Arduino-IDE compat.
- **Text spec only** — icons drawn procedurally with LVGL vector primitives (no PNG assets at MVP).
- **Scope: items 1–6** (display/touch, boot, home, app base, My Name, Settings) + 7 placeholder apps. Tracing engine, audio, haptics, SD/JSON, parent web = stubs only.

## Hardware pin map (from existing `.ino`)
| Function | GPIO |
|---|---|
| TFT CS/DC/RST | 15/2/4 |
| TFT SCK/MOSI/MISO | 14/13/12 |
| TFT BL | 27 |
| XPT2046 CS | 21 |
| (future) I2S audio | TBD, PSRAM assumed |

## Project structure
```
platformio.ini
ninopad.ino                    # thin wrapper → setup()/loop() call into src
lv_conf.h                      # ESP32-S3 tuned
src/
  main.cpp                     # init display/touch/lvgl, screen mgr
  hal/
    display_driver.cpp/h       # Arduino_GFX ST7796 init, flush_cb, BL PWM
    touch_xpt2046.cpp/h        # XPT2046 read_cb
    touch_ft6236.cpp/h         # optional FT6236 stub
  ui/
    nino_colors.h              # 7 brand color macros
    nino_styles.cpp/h          # shared styles
    screen_manager.cpp/h       # screen transitions + anims
    screens/
      scr_boot.cpp/h
      scr_home.cpp/h
      scr_app_base.cpp/h
    apps/
      app_my_name.cpp/h
      app_settings.cpp/h
      app_placeholder.cpp/h
      app_registry.cpp/h
    icons/
      nino_icons.cpp/h          # LVGL vector icon draws
  utils/
    anim_utils.cpp/h
storage/
  settings.cpp/.h               # NVS-backed (SD JSON deferred)
```

## Key technical choices
1. No external asset files at MVP. Use LVGL built-in fonts. Skip `lv_font_conv`/`lv_img_conv`.
2. Touch wrap behind indirection so FT6236 can swap at compile-time via `#define`.
3. Boot "NinoPad": each letter separate lv_label colored individually; pulse anim.
4. Home: `lv_obj` grid 3×3, each cell custom-colored button, bounce on tap.
5. App base: colored header + back round btn → fade to home.
6. My Name: italic-ish label + dashed border (drawn segments) + bordered canvas "DRAW HERE".
7. Settings: textarea + keyboard modal; NVS persistence behind Storage API.
8. Transitions: `lv_anim` fade opa.
9. `lv_conf.h`: 16bpp, custom mem (PSRAM), perf monitor off, log on, grid+canvas+keyboard+textarea on.

## MVP acceptance checklist
- [ ] Boot: 7 colored letters, pulse, auto 2s / tap skip
- [ ] Home: status bar, title, 3×3 grid w/ correct hex
- [ ] Tap button: bounce → open app
- [ ] App base: colored header + back → home
- [ ] My Name: "My name is" + dashed box + canvas placeholder
- [ ] Settings: textarea+keyboard; persists across reboot
- [ ] 7 placeholder apps: header + "Coming soon!"
- [ ] `pio run` builds

## Deferred (stubs)
- Real app logic for 7 apps
- Tracing engine
- I2S audio
- SD/JSON storage (NVS only for now)
- Haptics, parent web config
- Font/image conversion tooling, real fonts
- Full `hardware/` wiring docs