/**
 * app_registry.h — the 9 NinoPad apps and their metadata.
 * Built on top of scr_app_base: each app defines a create(content) callback.
 */
#pragma once
#include <lvgl.h>
#include "ui/icons/nino_icons.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_MY_NAME = 0,
    APP_LUZ_LETTERS,
    APP_WORD_SPY,
    APP_COUNTING_JAR,
    APP_TEN_FRAME,
    APP_SHAPE_PAINT,
    APP_SNIP_SNIP,
    APP_STORY_TIME,
    APP_SETTINGS,
    APP_COUNT
} nino_app_id_t;

typedef void (*nino_app_create_fn)(lv_obj_t *content);

typedef struct {
    const char         *name;     // "MY NAME" etc.
    lv_color_t          color;
    nino_icon_draw_fn   draw_icon;
    nino_app_create_fn  create;
} nino_app_t;

const nino_app_t *nino_app_get(nino_app_id_t id);

#ifdef __cplusplus
}
#endif