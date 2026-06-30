/**
 * touch_ft6236.cpp — optional FT6236U capacitive touch -> LVGL indev.
 * Default build does NOT compile this; XPT2046 is used. If you wire a
 * real FT6236 module (SDA=8, SCL=9, INT=3 by convention for this board),
 * switch platformio.ini build flag from NINO_TOUCH_XPT2046
 * to NINO_TOUCH_FT6236. The driver registration in main.cpp picks this up.
 */
#include "touch_ft6236.h"

#ifdef NINO_TOUCH_FT6236

#include <Wire.h>
#include "FT6236.h"

extern "C" uint32_t nino_display_hor_res(void);
extern "C" uint32_t nino_display_ver_res(void);

#define FT6236_ADDR   0x38
#define FT_SDA        8
#define FT_SCL        9

static FT6236 ft;

extern "C" void nino_touch_ft_init(void)
{
    Wire.begin(FT_SDA, FT_SCL, 400000);
    ft.begin(FT6236_ADDR);
}

extern "C" void nino_touch_ft_read_cb(lv_indev_t * /*indev*/, lv_indev_data_t *data)
{
    if (ft.touched()) {
        TS_Point p = ft.getPoint();
        data->state  = LV_INDEV_STATE_PRESSED;
        // FT chips report already-scaled coords; assume panel == native res.
        data->point.x = p.x;
        data->point.y = p.y;
        if (data->point.x >= (int32_t)nino_display_hor_res())
            data->point.x = nino_display_hor_res() - 1;
        if (data->point.y >= (int32_t)nino_display_ver_res())
            data->point.y = nino_display_ver_res() - 1;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

#endif /* NINO_TOUCH_FT6236 */