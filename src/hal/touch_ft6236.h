/**
 * touch_ft6236.h — optional capacitive touch adapter (FT6236U / FT5206 over I2C).
 * Compile with -DNINO_TOUCH_FT6236. Default build still uses XPT2046.
 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void nino_touch_ft_init(void);
void nino_touch_ft_read_cb(lv_indev_t *indev, lv_indev_data_t *data);

#ifdef __cplusplus
}
#endif