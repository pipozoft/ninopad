/**
 * main.cpp — NinoPad entry point (called from ninopad.ino).
 * Initializes display, touch, LVGL, styles, then loads the boot screen.
 */
#include <Arduino.h>
#include <lvgl.h>

#include "hal/display_driver.h"

#ifdef NINO_TOUCH_XPT2046
#include "hal/touch_xpt2046.h"
#define NINO_TOUCH_INIT       nino_touch_init
#define NINO_TOUCH_READ_CB    nino_touch_read_cb
#elif defined(NINO_TOUCH_FT6236)
#include "hal/touch_ft6236.h"
#define NINO_TOUCH_INIT       nino_touch_ft_init
#define NINO_TOUCH_READ_CB    nino_touch_ft_read_cb
#else
#error "Pick a touch driver: -DNINO_TOUCH_XPT2046 or -DNINO_TOUCH_FT6236"
#endif

#include "ui/nino_styles.h"
#include "ui/screen_manager.h"

static void nino_lv_log_cb(lv_log_level_t level, const char *buf)
{
    (void)level;
    Serial.print(buf);
}

static uint32_t nino_tick_cb(void)
{
    return millis();
}

void setup(void)
{
    Serial.begin(115200);
    delay(100);

    lv_init();
    lv_tick_set_cb(nino_tick_cb);
#if LV_USE_LOG
    lv_log_register_print_cb((lv_log_print_g_cb_t)nino_lv_log_cb);
#endif

    // Hardware
    nino_display_init();
    NINO_TOUCH_INIT();
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, NINO_TOUCH_READ_CB);

    // Styles + first screen
    nino_styles_init();
    nino_screen_show_boot();

    Serial.println("[NinoPad] ready");
}

void loop(void)
{
    lv_timer_handler();
    delay(5);
}