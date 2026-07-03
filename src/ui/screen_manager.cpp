/**
 * screen_manager.cpp — navigation glue.
 */
#include "screen_manager.h"
#include "screens/scr_boot.h"
#include "screens/scr_home.h"
#include "screens/scr_app_base.h"

void nino_screen_show_boot(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    scr_boot_create(scr);
}

void nino_screen_show_home(void)
{
    nino_home_stop();
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
    scr_app_base_create(scr, id);
}
