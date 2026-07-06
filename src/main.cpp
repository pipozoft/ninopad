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
#include "ui/screens/scr_boot.h"
#include "ui/screens/scr_home.h"
#include "utils/sd_utils.h"
#include "utils/wifi_utils.h"
#include "storage/sd_fs_drv.h"

#ifndef NINO_TZ_OFFSET
#define NINO_TZ_OFFSET -5
#endif

static void pump_lvgl(void)
{
    lv_timer_handler();
    lv_display_t *d = lv_display_get_next(NULL);
    if (d) lv_refr_now(d);
}

static void run_loading_sequence(void)
{
    if (nino_sd_is_mounted())
    {
        scr_boot_set_status("SD card ready");
        pump_lvgl();
        delay(300);

        scr_boot_set_status("Loading icons...");
        pump_lvgl();
        nino_home_preload_icons();
        pump_lvgl();

        scr_boot_set_status("Connecting to WiFi...");
        pump_lvgl();
        bool ok = nino_wifi_connect_from_sd();

        if (ok)
        {
            scr_boot_set_status("WiFi connected");
            pump_lvgl();
            delay(300);

            scr_boot_set_status("Syncing time...");
            pump_lvgl();
            nino_ntp_sync(NINO_TZ_OFFSET);

            scr_boot_set_status("Ready");
            pump_lvgl();
            delay(200);
        }
    }

    scr_boot_advance_home();
}

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

    nino_display_init();
    NINO_TOUCH_INIT();
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, NINO_TOUCH_READ_CB);

    sd_fs_drv_register();
    nino_sd_mount();
    nino_styles_init();
    nino_screen_show_boot();
    pump_lvgl();

    run_loading_sequence();

    Serial.println("[NinoPad] ready");
}

void loop(void)
{
    static uint32_t loop_cnt = 0;
    loop_cnt++;
    if ((loop_cnt % 100) == 0) Serial.print(".");
    lv_timer_handler();
    lv_display_t *d = lv_display_get_next(NULL);
    if (d) lv_refr_now(d);
    delay(2);
}
