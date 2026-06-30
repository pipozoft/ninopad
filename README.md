# NinoPad — Touchscreen Learning Device for Kindergarteners

LVGL v9 + ESP32-S3 + ST7796 480×320 + XPT2046 resistive touch.

## Quick Start

```bash
# Install deps
python3 -m venv .venv
.venv/bin/pip install platformio

# Build & flash
.venv/bin/pio run -t upload    # USB connected ESP32-S3
.venv/bin/pio device monitor    # 115200 baud
```

## Hardware Pin Map

| Function | GPIO | Notes |
|----------|------|-------|
| **Display** | | ST7796 480×320 SPI |
| TFT CS | 15 | |
| TFT DC | 2 | |
| TFT RST | 4 | |
| TFT MOSI | 13 | SPI MOSI |
| TFT MISO | 12 | SPI MISO |
| TFT SCK | 14 | SPI SCK |
| LCD Backlight | 27 | PWM (LEDC ch0, 5kHz, 8-bit) |
| **Touch** | | XPT2046 resistive |
| Touch CS | 21 | Shared SPI (with display) |
| (alt) FT6236 SDA | 8 | Capacitive option (I2C) |
| (alt) FT6236 SCL | 9 | |
| **Future** | | |
| I2S BCK | TBD | MAX98357A |
| I2S LRC | TBD | |
| I2S DIN | TBD | |
| SD CS | TBD | MicroSD SPI |
| SD MOSI | TBD | |
| SD MISO | TBD | |
| SD SCK | TBD | |
| Vibration | TBD | Haptic motor PWM |

## Project Structure

```
ninopad/
├── platformio.ini           # ESP32-S3 build config
├── lv_conf.h                # LVGL v9 config (16bpp, BUILTIN pool, grid/canvas/kb)
├── ninopad.ino              # Arduino IDE stub (PlatformIO uses src/main.cpp)
├── src/
│   ├── main.cpp             # init display, touch, LVGL, screen_manager
│   ├── hal/
│   │   ├── display_driver.* # Arduino_GFX ST7796 flush_cb, backlight PWM
│   │   ├── touch_xpt2046.*  # XPT2046 read_cb (default)
│   │   └── touch_ft6236.*   # FT6236 stub (compile-time swap)
│   ├── ui/
│   │   ├── nino_colors.h    # Brand color hex macros
│   │   ├── nino_styles.*    # Shared LVGL styles (btn, header, content)
│   │   ├── screen_manager.* # scr_boot → scr_home → scr_app transitions
│   │   ├── screens/
│   │   │   ├── scr_boot.*   # Colored "NinoPad" logo, pulse anim, 2s auto-advance
│   │   │   ├── scr_home.*   # Status bar, title, 3×3 app grid w/ bounce-on-tap
│   │   │   └── scr_app_base.* # Colored header, back button, content area
│   │   └── apps/
│   │       ├── app_registry.* # 9 app entries (name, color, icon, create-callback)
│   │       ├── app_my_name.*  # "My name is" + textarea + canvas placeholder
│   │       ├── app_settings.* # Textarea + keyboard → NVS save
│   │       └── app_placeholder.* # "Coming soon!" (used by 7 apps)
│   ├── utils/
│   │   └── anim_utils.*     # pulse, bounce, fade animations
│   └── storage/
│       ├── settings.*       # NVS-backed child name (SD+JSON deferred)
├── hardware/                # Wiring diagrams (TBD)
├── assets/                  # Font/icon/sound assets (deferred)
├── tools/
│   └── pio_strip_helium.py  # Removes ARM Helium assembly (ESP32 compat hack)
└── PLAN.md                  # Implementation plan
```

## Customizing the Child's Name

On-device:

1. Tap **SETTINGS** on home screen
2. Type name in the textarea
3. Tap ✓ (keyboard Ready) — persists across reboots via NVS

Programmatically — set initial name in `src/storage/settings.cpp` or edit the `prefs.getString` default.

## Adding a New App

1. Create `src/ui/apps/app_mything.cpp` defining a `void app_mything_create(lv_obj_t *content)` function
2. Create matching `.h` including `app_registry.h`
3. Add the entry in `src/ui/apps/app_registry.cpp`:
   ```c
   { "MY THING", NINO_COLOR_MYNAME, nino_icon_sun, app_mything_create }
   ```
4. (Optional) Add a new `nino_icon_*` function in `src/ui/icons/nino_icons.cpp`

The app base template handles the colored header bar and back button; `content` is the container below it.

## Deferred Features (Stubs Exist)

| Feature | Status | Notes |
|---------|--------|-------|
| Tracing engine | Stub | `app_my_name` canvas + dashed border. Guide-path validation, green/red dot, star burst deferred |
| Audio (I2S DMA WAV) | Not started | `audio/` not in tree. Queue system for tap/success/letter sounds |
| SD+JSON storage | Not started | `src/storage` has NVS-only. ArduinoJson dep added for future |
| Vibration/haptics | Not started | |
| Parent web config | Not started | |
| Font/image asset pipeline | Not started | `assets/` directories empty. `lv_font_conv`/`lv_img_conv` Python scripts deferred |
| FT6236 capacitive touch | Stub | File present, hidden behind `-DNINO_TOUCH_FT6236`. Not compiled by default |
| 7 placeholder apps | Stub | Show "Coming soon!". Real content deferred |

## Switching to FT6236 Capacitive Touch

In `platformio.ini`, change the touch driver flag:

```ini
build_flags =
  -DNINO_TOUCH_FT6236
  # -DNINO_TOUCH_XPT2046    # comment out
```

Pin configuration: see `src/hal/touch_ft6236.cpp` (SDA=8, SCL=9).

## Memory & Performance Tuning

- **LVGL buffer**: `src/hal/display_driver.cpp` uses 480×40×2 = 38KB per buffer, two buffers via `malloc()`. Default DRAM enough for 480×320 w/ 16bpp.
- **LVGL pool**: `lv_conf.h` sets `LV_MEM_SIZE` to 64KB (BUILTIN allocator). Sufficient for ~500 objects.
- **µtask interval**: `lv_timer_handler()` called every 5ms in `loop()`. Target ~60fps.
- **PSRAM**: If your DevKitC-1 variant has PSRAM (N16R8), `malloc()` auto-routes to PSRAM. For guaranteed PSRAM-only allocation, set `LV_USE_STDLIB_MALLOC = LV_STDLIB_CUSTOM` and provide `heap_caps_malloc` wrappers.
- To reduce flash, disable unused LVGL widgets (`LV_USE_* 0`) in `lv_conf.h`.

## Dependencies

- LVGL v9.5 (built-in pool, 16bpp)
- Arduino_GFX v1.3.9 (ST7796 driver via Arduino_ESP32SPI)
- XPT2046_Touchscreen (resistive)
- ArduinoJson 7 (deferred, for SD+JSON)
- Arduino Preferences (NVS)

## License

MIT (per original ninopad.ino starter).
