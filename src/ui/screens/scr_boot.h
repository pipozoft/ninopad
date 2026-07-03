#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif
void scr_boot_create(lv_obj_t *scr);
void scr_boot_set_status(const char *text);
void scr_boot_advance_home(void);
#ifdef __cplusplus
}
#endif
