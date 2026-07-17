#include "scr_congrats.h"
#include <Arduino.h>

lv_obj_t *nino_congrats_create(lv_obj_t *parent, const char *title,
                                int star_count, const char *subtitle,
                                lv_color_t btn_color, lv_coord_t btn_width,
                                lv_event_cb_t play_cb)
{
    lv_obj_t *overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, 480, 276);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0xF0FFF0), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    int y = 24;

    if (star_count > 0) {
        char stars[16];
        snprintf(stars, sizeof(stars), "%.*s", star_count, "***");
        lv_obj_t *sl = lv_label_create(overlay);
        lv_label_set_text(sl, stars);
        lv_obj_set_style_text_font(sl, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(sl, lv_color_hex(0xF1C40F), 0);
        lv_obj_align(sl, LV_ALIGN_TOP_MID, 0, y);
        y += 46;
    }

    lv_obj_t *ml = lv_label_create(overlay);
    lv_label_set_text(ml, title);
    lv_obj_set_style_text_font(ml, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(ml, lv_color_hex(0x27AE60), 0);
    lv_obj_set_style_text_align(ml, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ml, LV_ALIGN_TOP_MID, 0, y);
    lv_label_set_long_mode(ml, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ml, 400);
    y += 36;

    if (subtitle) {
        lv_obj_t *ssl = lv_label_create(overlay);
        lv_label_set_text(ssl, subtitle);
        lv_obj_set_style_text_font(ssl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(ssl, lv_color_hex(0x666666), 0);
        lv_obj_align(ssl, LV_ALIGN_TOP_MID, 0, y);
        y += 28;
    }

    lv_coord_t btn_height = 52;
    y += 20;

    lv_obj_t *pb = lv_btn_create(overlay);
    lv_obj_set_style_bg_color(pb, btn_color, 0);
    lv_obj_set_size(pb, btn_width, btn_height);
    lv_obj_set_style_radius(pb, 26, 0);
    lv_obj_set_style_shadow_width(pb, 0, 0);
    lv_obj_align(pb, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_add_event_cb(pb, play_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *pl = lv_label_create(pb);
    lv_label_set_text(pl, "Play Again");
    lv_obj_set_style_text_color(pl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(pl, &lv_font_montserrat_20, 0);
    lv_obj_center(pl);

    return overlay;
}
