/**
 * lv_conf.h — NinoPad (ESP32 / ESP32-S3 + ST7796 480x320 + XPT2046)
 * LVGL v9 config. 16bpp, CLIB heap (system malloc), moderate feature surface for MVP.
 * Sysmon + perf monitor enabled. Widgets disabled: meter, table, tabview, tileview, win.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*================
 * COLOR SETTINGS
 *================*/
#define LV_COLOR_DEPTH          16
#define LV_COLOR_CHROMA_KEY     lv_color_hex(0xff00ff)
#define LV_COLOR_SCREEN_TRANSP  0

/*================
 * MEMORY — use libc malloc (LV_STDLIB_CLIB). On ESP32-S3 with PSRAM, libc malloc
 * routes large allocations to PSRAM transparently. On standard ESP32 (no PSRAM),
 * everything stays in DRAM. If you need to override lv_malloc for custom pools,
 * set LV_USE_STDLIB_MALLOC to LV_STDLIB_CUSTOM.
 *================*/
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
/* LV_MEM_SIZE not needed when using CLIB (uses system heap) */

/* LVGL's stdlib providers (use libc defaults). */
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB
#define LV_USE_STDLIB_TIME      LV_STDLIB_CLIB

/*================
 * Feature flags
 *================*/
#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF           1
#define LV_USE_ASSERT_STYLE     0
#define LV_USE_ASSERT_OBJ       0

#define LV_USE_PERF_MONITOR      1
#define LV_USE_PERF_MONITOR_POS  LV_ALIGN_BOTTOM_RIGHT
#define LV_USE_MEM_MONITOR       0
#define LV_USE_REFR_DEBUG         0

#define LV_DRAW_BUF_STRIDE_ALIGN 2
#define LV_DRAW_BUF_ALIGN        4

/*================
 * FONT
 *================*/
#define LV_FONT_MONTSERRAT_8     0
#define LV_FONT_MONTSERRAT_10    0
#define LV_FONT_MONTSERRAT_12    1
#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_16    1
#define LV_FONT_MONTSERRAT_18    0
#define LV_FONT_MONTSERRAT_20    1
#define LV_FONT_MONTSERRAT_22    0
#define LV_FONT_MONTSERRAT_24    1
#define LV_FONT_MONTSERRAT_26    0
#define LV_FONT_MONTSERRAT_28    1
#define LV_FONT_MONTSERRAT_32    0
#define LV_FONT_MONTSERRAT_36    0
#define LV_FONT_MONTSERRAT_48    0

#define LV_FONT_MONTSERRAT_44    0

#define LV_FONT_DEFAULT          &lv_font_montserrat_16
#define LV_FONT_FMT_TXT_LARGE    0
#define LV_USE_FONT_COMPRESSED    1
#define LV_USE_FONT_PLACEHOLDER   1
#define LV_FONT_SUBPX_BPP          LV_FONT_SUBPX_NONE

/*================
 * WIDGETS
 *================*/
#define LV_USE_ANIMIMG    0
#define LV_USE_ARC        0
#define LV_USE_BAR        0
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  0
#define LV_USE_CANVAS     0
#define LV_USE_CHECKBOX   0
#define LV_USE_DROPDOWN   0
#define LV_USE_IMG        1
#define LV_USE_IMGBUTTON  0
#define LV_USE_KEYBOARD   1
#define LV_USE_LABEL      1
#define LV_USE_LED        0
#define LV_USE_LINE       1
#define LV_USE_LIST       0
#define LV_USE_MENU       0
#define LV_USE_METER      0
#define LV_USE_MSGBOX     0
#define LV_USE_ROLLER     0
#define LV_USE_SCALE      0
#define LV_USE_SLIDER     0
#define LV_USE_SPAN       0
#define LV_USE_SPINBOX    0
#define LV_USE_SPINNER    0
#define LV_USE_SWITCH     0
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      0
#define LV_USE_TABVIEW    0
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0
#define LV_USE_CHART      0
#define LV_USE_CALENDAR   0
#define LV_USE_QRCODE     0
#define LV_USE_BARCODE    0

/*================
 * LAYOUTS
 *================*/
#define LV_USE_FLEX  1
#define LV_USE_GRID  1

/*================
 * STYLES & THEMES
 *================*/
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_SIMPLE  0
#define LV_USE_THEME_MONO    0
#define LV_THEME_DEFAULT_DARK 1

/*================
 * DRAW
 *================*/
#define LV_BIN_DECODER_RAM_LOAD 1

/* CACHE: image cache keeps decoded image in RAM so decoders don't re-read SD. */
/* 72KB holds 9 icons (50×50 RGB565A8 @ 7.5KB each = 67.5KB) but NOT the logo
   (299×55 @ 49KB), so logo is evicted during preload and its 49KB returned to heap. */
#define LV_CACHE_DEF_SIZE       0
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0

#define LV_DRAW_SW_SUPPORT_L8 0
#define LV_DRAW_SW_SUPPORT_I1 0
#define LV_DRAW_SW_SUPPORT_AL88 0
#define LV_DRAW_SW_SUPPORT_ARGB8888 0
#define LV_DRAW_SW_SUPPORT_XRGB8888 0
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_SUPPORT_RGB888 0

#define LV_DRAW_SW_CLOBBER_BITS_DITHER 4

/* LVGL's own dither/color config. */
#define LV_DITHER_GRADIENT 0
#define LV_USE_DRAW_MASKS 1

/*================
 * OS / OPS
 *================*/
#define LV_USE_OS           LV_OS_NONE
#define LV_USE_PTHREAD       0
#define LV_DEF_REFR_PERIOD  20
#define LV_INDEV_DEF_READ_PERIOD 30
#define LV_USE_PROFILER     0
#define LV_PROFILER_INCLUDE  "lvgl_profiler.h"
#define LV_USE_SYSMON       1
#define LV_USE_OBSERVER     1
#define LV_USE_MSG          0
#define LV_USE_IME_TYPES 0
#define LV_USE_IME_PINYIN 0
#define LV_USE_FILE_NATIVE 0

#endif /* LV_CONF_H */
