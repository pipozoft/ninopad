/**
 * screen_manager.cpp — navigation glue.
 *
 * Cache strategy: icons (67.5KB cached) are freed when entering an app
 * so app screens have enough RAM. Restored on home return (icons re-read
 * from SD on first render, which is faster than initial boot since there's
 * no competition for the SPI bus).
 */
#include "screen_manager.h"
#include "screens/scr_boot.h"
#include "screens/scr_home.h"
#include "screens/scr_app_base.h"
#include "misc/cache/instance/lv_image_cache.h"

void nino_screen_show_boot(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    scr_boot_create(scr);
}

void nino_screen_show_home(void)
{
    nino_home_stop();
    lv_image_cache_resize(72000, false);
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    scr_home_create(scr);
}

void nino_screen_show_app(nino_app_id_t id)
{
    if (id < 0 || id >= APP_COUNT) return;
    nino_home_stop();
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_image_cache_resize(0, true);
    scr_app_base_create(scr, id);
}
