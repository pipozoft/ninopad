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

#ifdef __cplusplus
}
#endif