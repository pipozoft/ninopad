/**
 * screen_manager.cpp — navigation glue.
 */
#include "screen_manager.h"
#include "screens/scr_boot.h"
#include "screens/scr_home.h"
#include "screens/scr_app_base.h"

void nino_screen_show_boot(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    scr_boot_create(scr);
    lv_screen_load(scr);
}

void nino_screen_show_home(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    scr_home_create(scr);
    lv_screen_load(scr);
}

void nino_screen_show_app(nino_app_id_t id)
{
    if (id < 0 || id >= APP_COUNT) return;
    lv_obj_t *scr = lv_obj_create(NULL);
    scr_app_base_create(scr, id);
    lv_screen_load(scr);
}