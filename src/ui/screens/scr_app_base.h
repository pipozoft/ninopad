/**
 * scr_app_base.h — colored header, back button, content container, then app body.
 */
#pragma once
#include <lvgl.h>
#include "../apps/app_registry.h"

#ifdef __cplusplus
extern "C" {
#endif
void scr_app_base_create(lv_obj_t *scr, nino_app_id_t id);
#ifdef __cplusplus
}
#endif