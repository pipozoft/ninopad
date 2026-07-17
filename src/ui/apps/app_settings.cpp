#include "app_settings.h"
#include "storage/settings.h"
#include "nino_colors.h"
#include "utils/anim_utils.h"
#include <Arduino.h>

static lv_obj_t *parent_content;
static lv_obj_t *name_ta;
static lv_obj_t *kb;
static lv_obj_t *verify_ta;
static lv_obj_t *verify_overlay;

// ---- Settings UI (shown after age verification) ----

static void on_name_ready(lv_event_t *e)
{
    (void)e;
    const char *txt = lv_textarea_get_text(name_ta);
    if (txt && txt[0]) nino_settings_set_name(txt);
}

static void create_settings_ui(void)
{
    lv_obj_t *intro = lv_label_create(parent_content);
    lv_label_set_text(intro, "Child's name:");
    lv_obj_set_style_text_color(intro, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(intro, &lv_font_montserrat_14, 0);
    lv_obj_align(intro, LV_ALIGN_TOP_LEFT, 12, 12);

    name_ta = lv_textarea_create(parent_content);
    lv_obj_set_width(name_ta, 440);
    lv_textarea_set_max_length(name_ta, 32);
    lv_textarea_set_one_line(name_ta, true);
    lv_textarea_set_placeholder_text(name_ta, "tap to type name");
    lv_obj_align(name_ta, LV_ALIGN_TOP_LEFT, 12, 34);

    char buf[64];
    if (nino_settings_get_name(buf, sizeof(buf)))
        lv_textarea_set_text(name_ta, buf);

    lv_obj_t *note = lv_label_create(parent_content);
    lv_label_set_text(note, "Tap " LV_SYMBOL_OK " on keyboard when done");
    lv_obj_set_style_text_color(note, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(note, &lv_font_montserrat_12, 0);
    lv_obj_align(note, LV_ALIGN_TOP_LEFT, 12, 82);

    kb = lv_keyboard_create(parent_content);
    lv_keyboard_set_textarea(kb, name_ta);
    lv_obj_add_event_cb(kb, on_name_ready, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, on_name_ready, LV_EVENT_CANCEL, NULL);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// ---- Age verification ----

static void on_verify(lv_event_t *e)
{
    (void)e;
    const char *txt = lv_textarea_get_text(verify_ta);
    int year = atoi(txt);
    int age = 2026 - year;

    if (age >= 18 && year > 1900) {
        lv_obj_del(verify_overlay);
        verify_overlay = NULL;
        create_settings_ui();
    } else {
        lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
        nino_anim_shake(btn);
    }
}

void app_settings_create(lv_obj_t *content)
{
    parent_content = content;

    lv_obj_set_style_bg_color(content, NINO_COLOR_BG, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    verify_overlay = lv_obj_create(content);
    lv_obj_remove_style_all(verify_overlay);
    lv_obj_set_size(verify_overlay, 480, 276);
    lv_obj_align(verify_overlay, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(verify_overlay, NINO_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(verify_overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(verify_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(verify_overlay);
    lv_label_set_text(title, "Parent Verification");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *prompt = lv_label_create(verify_overlay);
    lv_label_set_text(prompt, "Enter your birth year:");
    lv_obj_set_style_text_font(prompt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(prompt, lv_color_hex(0xBBBBBB), 0);
    lv_obj_align(prompt, LV_ALIGN_TOP_MID, 0, 48);

    verify_ta = lv_textarea_create(verify_overlay);
    lv_obj_set_width(verify_ta, 160);
    lv_textarea_set_max_length(verify_ta, 4);
    lv_textarea_set_one_line(verify_ta, true);
    lv_textarea_set_placeholder_text(verify_ta, "e.g. 1990");
    lv_textarea_set_accepted_chars(verify_ta, "0123456789");
    lv_obj_align(verify_ta, LV_ALIGN_TOP_MID, 0, 72);

    // Numeric keyboard for the year input
    lv_obj_t *verify_kb = lv_keyboard_create(verify_overlay);
    lv_keyboard_set_textarea(verify_kb, verify_ta);
    lv_obj_align(verify_kb, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_keyboard_set_mode(verify_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_event_cb(verify_kb, on_verify, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(verify_kb, on_verify, LV_EVENT_CANCEL, NULL);
}
