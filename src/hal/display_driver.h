/**
 * display_driver.h — NinoPad display init + LVGL flush_cb.
 * Hardware: ST7796 4" 480x320 SPI, driven by Arduino_GFX.
 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void nino_display_init(void);
uint32_t nino_display_hor_res(void);
uint32_t nino_display_ver_res(void);
void nino_backlight_set_pct(uint8_t pct);
void nino_disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void *nino_display_gfx(void);   // returns Arduino_GFX* for direct painting

#ifdef __cplusplus
}
#endif