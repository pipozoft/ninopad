/**
 * nino_colors.h — exact brand colors per NinoPad spec.
 */
#pragma once
#include <lvgl.h>

#define NINO_HEX(r,g,b)   (((r) << 16) | ((g) << 8) | (b))

// Boot logo letters
#define NINO_COLOR_N      lv_color_hex(0xFF6B8A) // pink
#define NINO_COLOR_i      lv_color_hex(0xFFB366) // orange
#define NINO_COLOR_n      lv_color_hex(0xD4F76A) // light green
#define NINO_COLOR_o      lv_color_hex(0x6BFF7A) // green
#define NINO_COLOR_P      lv_color_hex(0x66B3FF) // blue
#define NINO_COLOR_a      lv_color_hex(0xA366FF) // purple
#define NINO_COLOR_d      lv_color_hex(0xFF66D4) // magenta

// App button colors (per spec, row-major)
#define NINO_COLOR_MYNAME     lv_color_hex(0xFF6B8A) // MY NAME
#define NINO_COLOR_LUZ        lv_color_hex(0xFFB366) // LUZ LETTERS
#define NINO_COLOR_WORDSPY    lv_color_hex(0xD4F76A) // WORD SPY
#define NINO_COLOR_COUNTING   lv_color_hex(0x6BFF7A) // COUNTING JAR
#define NINO_COLOR_TENFRAME   lv_color_hex(0x6BFFF0) // 10-FRAME SUN
#define NINO_COLOR_SHAPE      lv_color_hex(0x66B3FF) // SHAPE PAINT
#define NINO_COLOR_SNIP       lv_color_hex(0xA366FF) // SNIP SNIP
#define NINO_COLOR_STORY      lv_color_hex(0xFF66D4) // STORY TIME
#define NINO_COLOR_SETTINGS   lv_color_hex(0xFFF066) // SETTINGS

// Status bar / chrome
#define NINO_COLOR_BG         lv_color_hex(0x1A1A1A)
#define NINO_COLOR_BAR_FG     lv_color_hex(0xFFFFFF)
#define NINO_COLOR_WHITE      lv_color_hex(0xFFFFFF)
#define NINO_COLOR_BLACK      lv_color_hex(0x000000)
#define NINO_COLOR_DARK_ICON   lv_color_hex(0x222222) // dark line art on buttons
#define NINO_COLOR_BACK_FG     lv_color_hex(0x222222) // circle back button glyph
#define NINO_COLOR_CONTENT_TINT lv_color_hex(0xF5F5F5) // light tinted app bg
#define NINO_COLOR_DASH        lv_color_hex(0x9A9A9A) // dashed box border