/**
 * app_registry.cpp — bind each app id to its name, color, icon, create fn.
 */
#include "app_registry.h"
#include "nino_colors.h"
#include "app_my_name.h"
#include "app_word_spy.h"
#include "app_ten_frame.h"
#include "app_story_time.h"
#include "app_luz_letters.h"
#include "app_snip_snip.h"
#include "app_settings.h"
#include "app_placeholder.h"

static const nino_app_t apps[APP_COUNT] = {
    // Row 1
    { "MY NAME",      NINO_COLOR_MYNAME,    nino_icon_smiley,   app_my_name_create     },
    { "LUZ LETTERS",  NINO_COLOR_LUZ,       nino_icon_flame,    app_luz_letters_create },
    { "WORD SPY",     NINO_COLOR_WORDSPY,   nino_icon_eye,      app_word_spy_create   },
    // Row 2
    { "COUNTING JAR", NINO_COLOR_COUNTING,  nino_icon_money,   app_placeholder_create },
    { "10-FRAME SUN", NINO_COLOR_TENFRAME,  nino_icon_sun,      app_ten_frame_create  },
    { "SHAPE PAINT",  NINO_COLOR_SHAPE,     nino_icon_brush,    app_placeholder_create },
    // Row 3
    { "SNIP SNIP",    NINO_COLOR_SNIP,      nino_icon_scissors, app_snip_snip_create   },
    { "STORY TIME",   NINO_COLOR_STORY,     nino_icon_book,     app_story_time_create },
    { "SETTINGS",     NINO_COLOR_SETTINGS,  nino_icon_gear,     app_settings_create    },
};

const nino_app_t *nino_app_get(nino_app_id_t id)
{
    if (id < 0 || id >= APP_COUNT) return nullptr;
    return &apps[id];
}