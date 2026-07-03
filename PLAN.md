# NinoPad — Implementation Plan & Technical Journal

## Board

| Target | MCU | RAM | Flash | PSRAM |
|--------|-----|-----|-------|-------|
| `esp32` (default) | ESP32 Xtensa LX6 dual 240MHz | 320KB | 4MB | No |
| `esp32s3` | ESP32-S3 Xtensa LX7 dual 240MHz | 512KB | 16MB | 8MB |

Current development target: **`esp32`** (ESP-WROOM-32, no PSRAM). All code works on both; the S3 build adds PSRAM support for full-frame buffers.

## Hardware Pin Map

| Function | GPIO | Bus | Notes |
|----------|------|-----|-------|
| TFT CS | 15 | SPI3 | |
| TFT DC | 2 | SPI3 | |
| TFT RST | 4 | SPI3 | |
| TFT SCK | 14 | SPI3 | |
| TFT MOSI | 13 | SPI3 | |
| TFT MISO | 12 | SPI3 | |
| LCD BL | 27 | — | LEDC PWM ch0, 5kHz, 8-bit |
| Touch CS | 33 | GPIO | XPT2046, bit-bang over SPI3 pins 14/13/12 |
| Touch IRQ | 36 | GPIO | Active low, pull-up |
| SD CS | 5 | SPI2 | Dedicated HSPI bus |
| SD SCK | 18 | SPI2 | |
| SD MOSI | 23 | SPI2 | |
| SD MISO | 19 | SPI2 | |
| FT SDA | 8 | I2C | Capacitive touch (alt), not compiled by default |
| FT SCL | 9 | I2C | |

## Locked Decisions

- **LVGL v9** (9.5) with **Arduino_GFX ST7796 480×320 landscape** + **XPT2046 resistive touch** (existing HW). FT6236 capacitive kept as optional drop-in stub.
- **PlatformIO** (`platformio.ini`, default `esp32` board). Thin `.ino` wrapper for Arduino IDE compat.
- **Text spec only** — icons drawn procedurally with LVGL line primitives (no PNG assets at MVP).
- **Screen reuse**: `lv_scr_act()` + `lv_obj_clean()` — no `lv_scr_load()` to avoid screen object creation overhead and flash transitions.
- **Display buffer**: single partial buffer (160 rows × 480px × 2B = 153KB). No PSRAM. Fallback to 60 rows if malloc fails.
- **Scope**: items 1–6 (display/touch, boot, home, app base, My Name, Settings) + 7 placeholder apps. Tracing engine, audio, haptics, SD/JSON, parent web = stubs only.

## Project Structure

```
platformio.ini
ninopad.ino                    # thin wrapper → setup()/loop()
lv_conf.h                      # ESP32 tuned: 16bpp, CLIB heap, sysmon on
src/
  main.cpp                     # init display/touch/LVGL, boot→home sequence
  hal/
    display_driver.cpp/h       # Arduino_GFX ST7796 init, flush_cb, PWM backlight
    touch_xpt2046.cpp/h        # XPT2046 bit-bang read_cb (default)
    touch_ft6236.cpp/h         # optional FT6236 capacitive stub
  ui/
    nino_colors.h              # brand colors (MicroPython palette), debug overlay flag
    nino_styles.cpp/h          # shared LVGL styles
    screen_manager.cpp/h       # boot→home→app→home transitions
    screens/
      scr_boot.cpp/h           # 7 colored letters, status label, auto-advance
      scr_home.cpp/h           # status bar, 3x3 grid, 5s update timer, LVGL sysmon
      scr_app_base.cpp/h       # colored header + back button + content area
    apps/
      app_my_name.cpp/h        # "My name is" + textarea + canvas placeholder
      app_settings.cpp/h       # textarea + keyboard → NVS
      app_placeholder.cpp/h    # "Coming soon!" for 7 apps
      app_registry.cpp/h       # 9 entries: name, color, icon, create-fn
    icons/
      nino_icons.cpp/h         # 11 line-art icons via lv_line primitives
  utils/
    anim_utils.cpp/h           # pulse, bounce, fade animations
    sd_utils.cpp/h             # SD mount (SPI2, 3 retries, soft-fail)
    wifi_utils.cpp/h           # WiFi from SD JSON, NTP sync with DST
  storage/
    settings.cpp/h             # NVS-backed child name (SD+JSON deferred)
hardware/                      # Wiring diagrams (TBD)
assets/                        # Font/icon/sound assets (deferred)
tools/
  pio_strip_helium.py          # Removes ARM Helium assembly (ESP32 compat)
PLAN.md                        # This file
```

## Technical Learnings (Technical Journal)

### SPI3 (VSPI) vs SPI1
`Arduino_ESP32SPI(..., 3)` must be used for the display. The GFX library's internal `#define VSPI 1` is wrong — ESP32 Arduino core uses `VSPI = 3`, `HSPI = 2`. Using `spi_num=1` maps to the flash controller, causing white-screen-on-boot. See `display_driver.cpp:67`.

### Display Inversion
The `Arduino_ST7796` constructor's 4th arg `IPS`:
- `true` → sends `0x21` (INVON) — correct for panels that natively display inverted
- `false` → sends `0x20` (INVOFF) — correct for direct panels

Toggle if colors are inverted (red↔cyan). No `setSwapBytes()` in Arduino_GFX 1.3.x.

### Screen Reuse + Timer Lifecycle
CRITICAL LESSION: All screens use `lv_scr_act()` + `lv_obj_clean()`. Before cleaning, all timers referencing objects on that screen must be stopped. The `nino_home_stop()` function deletes the status timer and nulls static pointers. If a timer fires after its target labels are destroyed, LVGL crashes (dangling pointer).

Sequence: `nino_home_stop()` → `lv_obj_clean(scr)` → `scr_*_create(scr)`.

### Label Bending (No rounder_cb)
In partial buffer mode, LVGL flushes dirty areas in strips. If a label update creates a small dirty rectangle that doesn't align to buffer boundaries, the SPI flush has unaligned start/end, causing visual tearing on that row.

LVGL v8 had `rounder_cb` to round flush areas to buffer boundaries. LVGL v9 removed it.

Workaround: After text updates, call `lv_obj_invalidate(lv_scr_act())` to force a full-screen redraw in aligned strips.

### LVGL Sysmon Bending Fix
`lv_sysmon_show_performance()` creates a timer (default 300ms period) that updates the sysmon label independently, causing bending between full-screen invalidations.

Fix: Call `lv_sysmon_performance_pause(disp)` after showing the sysmon, then manually call `lv_sysmon_performance_dump(disp)` from your own timer callback just before `lv_obj_invalidate(lv_scr_act())`. This ensures the sysmon updates at the same time as the full-redraw cycle.

Both `LV_USE_SYSMON` and `LV_USE_PERF_MONITOR` must be 1 in `lv_conf.h`.

### Event Handler Safety
Deleting the screen's children (`lv_obj_clean(scr)`) from within an event handler that references an object on that screen causes undefined behavior. Use `lv_async_call()` to defer screen transitions:

```cpp
lv_async_call([](void*) { nino_screen_show_home(); }, NULL);
```

### XPT2046 Calibration
Raw ADC values: X 315 (right) to 3910 (left), Y 278 (bottom) to 3746 (top). Both axes inverted. Mapped via `map()`, then clamped.

### Memory Budget (no PSRAM)
- Static: ~47KB
- Display buffer: 153KB (160-row single, 480×320×2)
- LVGL heap + misc: ~120KB
- Total: 320KB

### Buffer Fallback
If `malloc(153KB)` fails (fragmentation), falls back to 60 rows × 480 × 2 = 57.6KB. This increases strip count from 2 to 6, reducing frame rate.

## MVP Acceptance Checklist

- [x] Boot: 7 colored letters + status label, auto-advance after loading sequence
- [x] Home: status bar (logo, time, wifi), 3×3 grid with correct colors
- [x] Tap button: bounce → open app
- [x] App base: colored header + back button → home (via lv_async_call deferral)
- [x] My Name: "My name is" + textarea + canvas placeholder
- [x] Settings: textarea + keyboard → NVS persist
- [x] 7 placeholder apps: header + "Coming soon!"
- [x] `pio run` builds (RAM 14%, Flash 95%)
- [x] WiFi + NTP from SD JSON (multiple networks, DST-aware EST5EDT)
- [x] Debug overlay: LVGL sysmon (FPS, CPU, mem) updated every 5s
- [x] No label bending (full-screen invalidation + sysmon pause/manual-dump)
- [x] No freezes on home↔app navigation (timer lifecycle + async_call)

## Key Technical Choices

1. No external asset files at MVP. LVGL built-in fonts (Montserrat 12/14/16/24/28).
2. Touch behind `#define` indirection — swap XPT2046↔FT6236 at compile time.
3. Boot: 7 separate `lv_label` objects, each colored individually per `nino_colors.h`.
4. Home: `lv_layout_grid` 3×3, each cell with per-app color and line-art icon.
5. App base: colored header + round back button → `lv_async_call` → home.
6. Settings: textarea + always-visible keyboard; NVS preferences.
7. Debug overlay: LVGL sysmon (paused + manual dump), enabled via `ENABLE_DEBUG_OVERLAY 1`.
8. `lv_conf.h`: 16bpp, CLIB heap, sysmon+perf monitor enabled, grid+canvas+keyboard+textarea on.

## Deferred (Stubs)

- Real app logic for 7 placeholder apps
- Tracing engine (guide-path validation, feedback dots, star burst)
- I2S audio (MAX98357A, WAV files from SD)
- SD/JSON storage (replace NVS for settings)
- Haptics (PWM vibration motor)
- Parent web config
- Font/image conversion tooling
- Full `hardware/` wiring docs
