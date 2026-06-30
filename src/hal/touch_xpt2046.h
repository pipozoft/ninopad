/**
 * touch_xpt2046.h — resistive touch read_cb adapter for LVGL v9.
 * Spec lists FT6236U capacitive; this XPT2046 path is what the
 * original ninopad.ino starter uses, and is the default (NINO_TOUCH_XPT2046).
 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void nino_touch_init(void);
void nino_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data);

#ifdef __cplusplus
}
#endif