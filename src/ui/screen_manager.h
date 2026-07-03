/**
 * screen_manager.h — top-level navigation: boot → home → app → home.
 * All screens reuse the same screen object (lv_scr_act + lv_obj_clean)
 * to avoid screen-object creation and transition overhead.
 * Timers must be stopped BEFORE cleaning (see nino_home_stop).
 */
#pragma once
#include "apps/app_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

void nino_screen_show_boot(void);
void nino_screen_show_home(void);
void nino_screen_show_app(nino_app_id_t id);

#ifdef __cplusplus
}
#endif