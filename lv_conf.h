/**
 * lv_conf.h — NinoPad (ESP32-S3 + ST7796 480x320 + XPT2046)
 * LVGL v9 config. Tuned for PSRAM, 16bpp, moderate feature surface for MVP.
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
 * MEMORY — use libc malloc (LV_STDLIB_CLIB). ESP32-S3 libc malloc routes
 * allocations to PSRAM when present, transparently. If you need guaranteed
 * PSRAM-only pools and want to override lv_malloc, set LV_USE_STDLIB_MALLOC
 * to LV_STDLIB_CUSTOM and provide lv_malloc/lv_realloc/lv_free externally.
 *================*/
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_MEM_SIZE             (64U * 1024U)  /* used only when MALLOC = BUILTIN */

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
#define LV_USE_ASSERT_STYLE     1
#define LV_USE_ASSERT_OBJ       0

#define LV_USE_PERF_MONITOR      0
#define LV_USE_MEM_MONITOR       0
#define LV_USE_REFR_DEBUG         0

#define LV_DRAW_BUF_STRIDE_ALIGN 4
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
#define LV_FONT_MONTSERRAT_20    0
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
#define LV_USE_FONT_COMPRESSED    0
#define LV_USE_FONT_PLACEHOLDER   1
#define LV_FONT_SUBPX_BPP          LV_FONT_SUBPX_NONE

/*================
 * WIDGETS
 *================*/
#define LV_USE_ANIMIMG    1
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_IMGBUTTON  1
#define LV_USE_KEYBOARD   1
#define LV_USE_LABEL      1
#define LV_USE_LED        1
#define LV_USE_LINE       1
#define LV_USE_LIST       1
#define LV_USE_MENU       1
#define LV_USE_METER      0
#define LV_USE_MSGBOX     1
#define LV_USE_ROLLER     1
#define LV_USE_SCALE      0
#define LV_USE_SLIDER     1
#define LV_USE_SPAN       1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      0
#define LV_USE_TABVIEW    0
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0

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
#define LV_DRAW_SW_SUPPORT_L8 1
#define LV_DRAW_SW_SUPPORT_I1 1
#define LV_DRAW_SW_SUPPORT_AL88 1
#define LV_DRAW_SW_SUPPORT_ARGB8888 1
#define LV_DRAW_SW_SUPPORT_XRGB8888 1
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_SUPPORT_RGB888 1

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
#define LV_USE_SYSMON       0
#define LV_USE_OBSERVER     1
#define LV_USE_MSG          0
#define LV_USE_IME_TYPES 0
#define LV_USE_IME_PINYIN 0
#define LV_USE_FILE_NATIVE 0

#endif /* LV_CONF_H */