/**
 * app_settings.cpp — parent config screen: child's name entry with on-screen keyboard.
 * Saves to NVS via the settings API on each commit.
 */
#include "app_settings.h"
#include "storage/settings.h"
#include "nino_colors.h"

static lv_obj_t *name_ta;
static lv_obj_t *kb;

static void on_kb_ready(lv_event_t * /*e*/)
{
    const char *txt = lv_textarea_get_text(name_ta);
    if (txt && txt[0]) {
        nino_settings_set_name(txt);
    }
    // In LVGL v9, keyboard auto-hides via textarea. No separate hide() API.
    if (!lv_obj_has_flag(kb, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

void app_settings_create(lv_obj_t *content)
{
    lv_obj_set_style_bg_color(content, NINO_COLOR_CONTENT_TINT, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Instructions
    lv_obj_t *intro = lv_label_create(content);
    lv_label_set_text(intro, "Child's name:");
    lv_obj_set_style_text_color(intro, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(intro, &lv_font_montserrat_16, 0);
    lv_obj_align(intro, LV_ALIGN_TOP_LEFT, 8, 8);

    // Name textarea
    name_ta = lv_textarea_create(content);
    lv_obj_set_size(name_ta, 360, 44);
    lv_obj_align(name_ta, LV_ALIGN_TOP_LEFT, 8, 36);
    lv_textarea_set_max_length(name_ta, 32);
    lv_textarea_set_one_line(name_ta, true);
    lv_textarea_set_placeholder_text(name_ta, "tap to type name");

    char buf[64];
    if (nino_settings_get_name(buf, sizeof(buf))) {
        lv_textarea_set_text(name_ta, buf);
    }

    // Keyboard below — pop on first touch via textarea event, simpler to keep always-on
    kb = lv_keyboard_create(content);
    lv_obj_set_size(kb, 480, 170);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, name_ta);
    lv_obj_add_event_cb(kb, on_kb_ready, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, on_kb_ready, LV_EVENT_CANCEL, NULL);
}