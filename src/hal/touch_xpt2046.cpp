/**
 * touch_xpt2046.cpp — XPT2046 resistive touch -> LVGL indev.
 * Touch CS on GPIO21. SPI shared with display bus on pins 14/13/12.
 */
#include "touch_xpt2046.h"

#ifdef NINO_TOUCH_XPT2046

#include <XPT2046_Touchscreen.h>

#define TOUCH_CS   21
extern "C" uint32_t nino_display_hor_res(void);
extern "C" uint32_t nino_display_ver_res(void);

static XPT2046_Touchscreen ts(TOUCH_CS);

extern "C" void nino_touch_init(void)
{
    ts.begin();
    ts.setRotation(1); // landscape to match ST7796 rotation 1
}

extern "C" void nino_touch_read_cb(lv_indev_t * /*indev*/, lv_indev_data_t *data)
{
    if (ts.touched()) {
        TS_Point p = ts.getPoint();

        // XPT2046 raw range ~200..3800 across the panel.
        // Values near 0 or 4095 indicate false trigger (no real touch).
        if (p.x < 100 || p.x > 4000 || p.y < 100 || p.y > 4000) {
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }

        data->state = LV_INDEV_STATE_PRESSED;

        int32_t h = (int)nino_display_hor_res();
        int32_t v = (int)nino_display_ver_res();
        int32_t mx = map(p.x, 200, 3800, 0, h);
        int32_t my = map(p.y, 200, 3800, 0, v);

        // Clamp to screen bounds (map can extrapolate outside the range).
        data->point.x = (mx < 0) ? 0 : (mx >= h) ? (h - 1) : mx;
        data->point.y = (my < 0) ? 0 : (my >= v) ? (v - 1) : my;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

#endif /* NINO_TOUCH_XPT2046 */