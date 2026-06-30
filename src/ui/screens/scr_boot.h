/**
 * scr_boot.h — boot screen build (colored "NinoPad" letters, pulse anim, 2s/tap skip).
 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif
void scr_boot_create(lv_obj_t *scr);
#ifdef __cplusplus
}
#endif