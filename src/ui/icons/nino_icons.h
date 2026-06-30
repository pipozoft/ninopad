/**
 * nino_icons.h — line-art icons drawn with lv_line primitives into a 50x50 parent.
 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Each function adds lv_line_t children inside `parent` (a 50x50 transparent obj).
// `ink` is the line color (typically NINO_COLOR_DARK_ICON on colored buttons).
typedef void (*nino_icon_draw_fn)(lv_obj_t *parent, lv_color_t ink);

void nino_icon_smiley (lv_obj_t *p, lv_color_t ink);
void nino_icon_flame  (lv_obj_t *p, lv_color_t ink);
void nino_icon_eye    (lv_obj_t *p, lv_color_t ink);
void nino_icon_money  (lv_obj_t *p, lv_color_t ink);
void nino_icon_sun    (lv_obj_t *p, lv_color_t ink);
void nino_icon_brush  (lv_obj_t *p, lv_color_t ink);
void nino_icon_scissors(lv_obj_t *p, lv_color_t ink);
void nino_icon_book   (lv_obj_t *p, lv_color_t ink);
void nino_icon_gear   (lv_obj_t *p, lv_color_t ink);

// Status bar icons (small). Drawn in white onto parent.
void nino_icon_wifi   (lv_obj_t *p, lv_color_t ink);
void nino_icon_battery(lv_obj_t *p, lv_color_t ink);

#ifdef __cplusplus
}
#endif