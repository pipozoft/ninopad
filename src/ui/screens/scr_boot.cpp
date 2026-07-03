#include "scr_boot.h"
#include "screen_manager.h"
#include "nino_colors.h"
#include "nino_styles.h"
#include <Arduino.h>

static lv_obj_t *status_label = NULL;

void scr_boot_create(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, NINO_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 2, 0);

    static const lv_color_t letter_colors[7] = {
        NINO_COLOR_N, NINO_COLOR_i, NINO_COLOR_n, NINO_COLOR_o,
        NINO_COLOR_P, NINO_COLOR_a, NINO_COLOR_d
    };
    static const char *letters[7] = {"N", "i", "n", "o", "P", "a", "d"};
    for (int i = 0; i < 7; ++i) {
        lv_obj_t *lab = lv_label_create(row);
        lv_label_set_text(lab, letters[i]);
        lv_obj_set_style_text_color(lab, letter_colors[i], 0);
        lv_obj_set_style_text_font(lab, &lv_font_montserrat_28, 0);
    }

    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "");
    lv_obj_set_width(status_label, 400);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 30);

    lv_obj_invalidate(scr);
}

void scr_boot_set_status(const char *text)
{
    if (status_label)
    {
        lv_label_set_text(status_label, text);
        lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 30);
    }
}

void scr_boot_advance_home(void)
{
    nino_screen_show_home();
}
