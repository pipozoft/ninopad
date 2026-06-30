/**
 * screen_manager.h — top-level navigation: boot → home → app → home.
 * Each function builds and loads a fresh LVGL screen, applies a fade-in.
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