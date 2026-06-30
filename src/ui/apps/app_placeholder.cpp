/**
 * app_placeholder.cpp — "Coming soon!" centered, reused by 7 placeholder apps.
 */
#include "app_placeholder.h"

void app_placeholder_create(lv_obj_t *content)
{
    lv_obj_t *lab = lv_label_create(content);
    lv_label_set_text(lab, "Coming soon!");
    lv_obj_center(lab);
    lv_obj_set_style_text_color(lab, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_24, 0);
}