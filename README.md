# NinoPad — Touchscreen Learning Device for Kindergarteners

LVGL v9 + ESP32 + ST7796 480×320 + XPT2046 resistive touch.

## Quick Start

```bash
# Install deps
python3 -m venv .venv
.venv/bin/pip install platformio

# Build & flash (default: esp32)
.venv/bin/pio run -e esp32 -t upload
.venv/bin/pio device monitor    # 115200 baud
```

## Hardware

| Component | Part | Interface |
|-----------|------|-----------|
| MCU | ESP32 (ESP-WROOM-32, 240MHz, 320KB RAM, 4MB flash) | — |
| Display | 4" ST7796 480×320 RGB565 | SPI3 (VSPI), 3-wire + CS/DC/RST |
| Touch | XPT2046 resistive | GPIO bit-bang (shares SPI3 pins, taken over via GPIO matrix) |
| SD Card | MicroSD | SPI2 (HSPI), dedicated bus |
| Backlight | PWM via LEDC (ch0, 5kHz, 8-bit) | GPIO27 |

**No PSRAM.** RAM budget: ~47KB static + 153KB display buffer + ~120KB LVGL heap.

### Pin Map

| Function | GPIO | Notes |
|----------|------|-------|
| **Display (SPI3)** | | Arduino_ESP32SPI(bus, spi_num=3) |
| TFT CS | 15 | |
| TFT DC | 2 | |
| TFT RST | 4 | |
| TFT MOSI | 13 | SPI3 MOSI |
| TFT MISO | 12 | SPI3 MISO |
| TFT SCK | 14 | SPI3 SCK |
| LCD Backlight | 27 | LEDC ch0, 5kHz, 8-bit |
| **Touch (GPIO bit-bang)** | | Shares SCK/MOSI/MISO with display, taken/released via GPIO matrix |
| Touch CS | 33 | |
| Touch IRQ | 36 | Pulled up, active low |
| **SD Card (SPI2/HSPI)** | | Dedicated bus, 4MHz |
| SD CS | 5 | |
| SD SCK | 18 | |
| SD MOSI | 23 | |
| SD MISO | 19 | |
| **Capacitive touch (alt)** | | FT6236U, I2C, not compiled by default |
| FT SDA | 8 | |
| FT SCL | 9 | |

## Project Structure

```
ninopad/
├── platformio.ini           # esp32 + esp32s3 build config
├── lv_conf.h                # LVGL v9 config (16bpp, CLIB heap, grid/canvas/kb)
├── ninopad.ino              # Arduino IDE stub (ignored by PlatformIO)
├── src/
│   ├── main.cpp             # setup/loop: init display, touch, LVGL, boot→home
│   ├── hal/
│   │   ├── display_driver.* # Arduino_GFX ST7796 init, flush_cb, backlight PWM
│   │   ├── touch_xpt2046.*  # XPT2046 bit-bang read_cb (default, NINO_TOUCH_XPT2046)
│   │   └── touch_ft6236.*   # FT6236 capacitive stub (compile-time swap)
│   ├── ui/
│   │   ├── nino_colors.h    # Brand color hex macros, debug overlay flag
│   │   ├── nino_styles.*    # Shared LVGL styles (btn, header, content)
│   │   ├── screen_manager.* # scr_boot → scr_home → scr_app transitions
│   │   ├── screens/
│   │   │   ├── scr_boot.*   # Colored "NinoPad" logo, status label, auto-advance
│   │   │   ├── scr_home.*   # Status bar (wifi/time), 3×3 app grid, debug sysmon
│   │   │   └── scr_app_base.* # Colored header, back button, content area
│   │   ├── apps/
│   │   │   ├── app_registry.* # 9 app entries (name, color, icon, create-callback)
│   │   │   ├── app_my_name.*  # "My name is" + textarea + canvas placeholder
│   │   │   ├── app_settings.* # Textarea + keyboard → NVS save
│   │   │   └── app_placeholder.* # "Coming soon!" (7 placeholder apps)
│   │   └── icons/
│   │       └── nino_icons.*  # LVGL line-art primitives (smiley, flame, gear, etc.)
│   ├── utils/
│   │   ├── anim_utils.*     # pulse, bounce, fade animations
│   │   ├── sd_utils.*       # SD card mount (SPI2, 3 retries, soft-fail)
│   │   └── wifi_utils.*     # WiFi connect from SD JSON, NTP sync (EST5EDT)
│   └── storage/
│       └── settings.*       # NVS-backed child name storage (SD+JSON deferred)
├── hardware/                # Wiring diagrams (TBD)
├── assets/                  # Font/icon/sound assets (deferred)
├── tools/
│   └── pio_strip_helium.py  # Removes ARM Helium assembly (ESP32 compat hack)
└── PLAN.md                  # Implementation plan
```

## Technical Learnings (Read Before Hacking)

### SPI Bus Architecture

- **Display** uses **SPI3 (VSPI)** via `Arduino_ESP32SPI(..., 3)`. The GFX library's internal `#define VSPI 1` is wrong — ESP32 Arduino core uses VSPI = 3, not 1. Using `spi_num=1` crashes on boot (white screen, flash controller conflict).
- **SD card** uses **SPI2 (HSPI)** on pins 18/23/19/5 — a dedicated bus, no conflict.
- **Touch** shares the display's SPI3 pins (14/13/12) via **GPIO bit-bang**. The `take_pins()` / `release_pins()` functions detach/reattach SPI3 signals from the GPIO matrix using `pinMatrixOutDetach` / `pinMatrixInDetach` / `pinMatrixOutAttach` / `pinMatrixInAttach`. This avoids SPI bus contention.

### Display Color Inversion

The ST7796 panel may display inverted colors (red→cyan) depending on the panel variant. The `Arduino_ST7796` constructor's `IPS` parameter controls this:

```cpp
Arduino_ST7796(bus, rst, rotation, IPS);
// IPS=true  → sends 0x21 (INVON)  — for panels that need inversion
// IPS=false → sends 0x20 (INVOFF) — for panels that are normally direct
```

If colors are inverted, toggle `IPS`. There is no `setSwapBytes()` in Arduino_GFX 1.3.x.

### Screen Reuse Pattern (No Screen Load)

All screens use `lv_scr_act()` + `lv_obj_clean()` instead of `lv_scr_load()`. This avoids creating multiple screen objects and eliminates screen-load transitions. The single screen object is reused for boot → home → app → home.

**CRITICAL**: Always stop timers **before** cleaning the screen. If a timer callback references labels that are destroyed by `lv_obj_clean()`, the callback becomes a dangling pointer and freezes the device.

```cpp
void nino_screen_show_home(void) {
    nino_home_stop();           // 1. delete timer, null pointers
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);          // 2. safe: no live timers target this screen
    scr_home_create(scr);
}
```

### Label Bending (Partial Buffer Artifact)

In partial buffer mode, LVGL composes into the buffer then flushes to the display in strips. If a label's text changes, only the label's bounding box is marked dirty. The flush area may not align with the buffer's strip boundaries, causing visual tearing ("bending") on that row.

**LVGL v8** had `rounder_cb` to align flush areas to strip boundaries. **LVGL v9 removed it.**

**Workaround**: Invalidate the entire screen after text updates:

```cpp
lv_obj_invalidate(lv_scr_act());
```

This forces a full redraw in aligned strips. At 5-second intervals the performance impact is negligible.

### LVGL Sysmon Without Bending

`lv_sysmon_show_performance()` creates a label that updates every 300ms on its own timer, causing independent partial redraws and bending. **Solution**: Pause the sysmon's auto-refresh and manually dump from your 5-second status timer:

```cpp
lv_sysmon_show_performance(disp);
lv_sysmon_performance_pause(disp);       // stop auto-refresh

// In timer callback:
lv_sysmon_performance_dump(disp);        // manual update
lv_obj_invalidate(lv_scr_act());         // full-screen redraw
```

Both `LV_USE_SYSMON` and `LV_USE_PERF_MONITOR` must be 1 in `lv_conf.h`.

### Event Handler Safety

Deleting LVGL objects from within an event handler that references those objects can corrupt LVGL's internal state. If a button's `LV_EVENT_CLICKED` handler calls `lv_obj_clean(scr)` (which deletes that button), the event system may crash or produce undefined behavior.

**Fix**: Use `lv_async_call()` to defer screen transitions:

```cpp
static void on_back(lv_event_t *) {
    lv_async_call([](void*) { nino_screen_show_home(); }, NULL);
}
```

The async callback runs after the event processing cycle completes.

### XPT2046 Touch Calibration

Raw ADC values are inverted on both axes (X decreases right, Y decreases down). Edge values (from a 4" 480×320 panel):

| Edge | Raw value |
|------|-----------|
| X right | 315 |
| X left | 3910 |
| Y top | 3746 |
| Y bottom | 278 |

Mapped via `map(raw, edge_min, edge_max, display_max-1, 0)` then clamped to 0..max-1.

### Memory Layout

| Region | Size | Notes |
|--------|------|-------|
| Static RAM | ~47KB | Code, globals, BSS |
| Display buffer | 153KB | Single 160-row × 480px × 2B |
| LVGL heap + misc | ~120KB | Remaining of 320KB total |
| **Total RAM** | **320KB** | No PSRAM |

The display buffer is a single chunk (not dual). With 160 rows and 320 total visible rows, each frame requires 2 flushes (fallback to 60 rows if malloc fails = 6 flushes).

### OneFrame for ESP32 (Optional)

If you switch from `Arduino_GFX` to the `OneFrame` family of LVGL display drivers (e.g., `OneFrame_ST7796`), set `LV_DISPLAY_RENDER_MODE_FULL` for a true single-buffer mode where LVGL renders directly into the flush buffer. This eliminates the double-buffer copy but requires enough contiguous RAM for one full frame (480×320×2 = 307KB — which the ESP32 doesn't have). Partial mode is the only option without PSRAM.

## Build Targets

| Target | Board | PSRAM | Upload |
|--------|-------|-------|--------|
| `esp32` (default) | ESP32 Dev Module | No | 115200 |
| `esp32s3` | ESP32-S3 DevKitC-1 (N16R8) | Yes | 921600 |

## Customizing the Child's Name

On-device:
1. Tap **SETTINGS** on home screen
2. Type name in the textarea
3. Tap ✓ (keyboard Ready) — persists across reboots via NVS

Programmatically — edit the NVS default in `src/storage/settings.cpp`.

## Adding a New App

1. Create `src/ui/apps/app_mything.cpp` defining `void app_mything_create(lv_obj_t *content)`
2. Create matching `.h` including `app_registry.h`
3. Add the entry in `src/ui/apps/app_registry.cpp`:
   ```c
   { "MY THING", NINO_COLOR_MYNAME, nino_icon_sun, app_mything_create }
   ```
4. (Optional) Add a new `nino_icon_*` function in `src/ui/icons/nino_icons.cpp`

The app base template handles the colored header bar and back button; `content` is the container below it.

## Switching to FT6236 Capacitive Touch

In `platformio.ini`, change the touch driver flag:
```ini
build_flags =
  -DNINO_TOUCH_FT6236
  # -DNINO_TOUCH_XPT2046    # comment out
```

Pin configuration: see `src/hal/touch_ft6236.cpp` (SDA=8, SCL=9).

## Deferred Features

| Feature | Status | Notes |
|---------|--------|-------|
| Tracing engine | Stub | `app_my_name` canvas + dashed border deferred. Guide-path validation, green/red dot, star burst |
| Audio (I2S DMA WAV) | Not started | MAX98357A. Tap/success/letter sounds |
| SD+JSON storage | Not started | NVS-only currently. ArduinoJson dep added |
| Vibration/haptics | Not started | |
| Parent web config | Not started | |
| Font/image asset pipeline | Not started | `assets/` empty. `lv_font_conv`/`lv_img_conv` deferred |
| FT6236 capacitive touch | Stub | Not compiled by default |
| 7 placeholder apps | Stub | Show "Coming soon!". Real content deferred |

## Dependencies

- LVGL v9.5 (CLIB heap, 16bpp, sysmon/perf monitor enabled)
- Arduino_GFX v1.3.9 (ST7796 via Arduino_ESP32SPI, spi_num=3)
- XPT2046_Touchscreen (unused — custom bit-bang driver used instead)
- ArduinoJson 7 (deferred, for SD+JSON)
- Arduino Preferences (NVS)

## License

MIT (per original ninopad.ino starter).
