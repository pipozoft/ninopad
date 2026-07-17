#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *nino_congrats_create(lv_obj_t *parent, const char *title,
                                int star_count, const char *subtitle,
                                lv_color_t btn_color, lv_coord_t btn_width,
                                lv_event_cb_t play_cb);

#ifdef __cplusplus
}
#endif
