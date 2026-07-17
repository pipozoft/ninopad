/**
 * nino_styles.h — shared LVGL v9 styles for NinoPad (bg, app buttons, header, back button).
 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Black/dark background used on boot and home screens.
extern lv_style_t nino_style_bg;

// App button base: rounded 12px, shadow, pressed-state shrink. Color is per-instance
// (we set bg_color per object), but radius/shadow/transition are shared.
extern lv_style_t nino_style_app_btn;
extern lv_style_t nino_style_app_btn_pressed;

// Colored app header bar.
extern lv_style_t nino_style_header_bar;

// Round back button (white circle, dark glyph).
extern lv_style_t nino_style_back_btn;
extern lv_style_t nino_style_back_btn_pressed;

// Status bar text (small white on dark).
extern lv_style_t nino_style_status_text;
extern lv_style_t nino_style_app_title_dark;
extern lv_style_t nino_style_app_title_light;

// Light tinted app content area.
extern lv_style_t nino_style_content;

// Shared button styles (color + 22px pill + no border/shadow).
extern lv_style_t nino_style_btn_primary;
extern lv_style_t nino_style_btn_success;
extern lv_style_t nino_style_btn_danger;

void nino_styles_init(void);

#ifdef __cplusplus
}
#endif