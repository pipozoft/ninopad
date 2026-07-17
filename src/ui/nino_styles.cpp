/**
 * nino_styles.cpp — NinoPad shared LVGL v9 styles.
 */
#include "nino_styles.h"
#include "nino_colors.h"

lv_style_t nino_style_bg;
lv_style_t nino_style_app_btn;
lv_style_t nino_style_app_btn_pressed;
lv_style_t nino_style_header_bar;
lv_style_t nino_style_back_btn;
lv_style_t nino_style_back_btn_pressed;
lv_style_t nino_style_status_text;
lv_style_t nino_style_app_title_dark;
lv_style_t nino_style_app_title_light;
lv_style_t nino_style_content;
lv_style_t nino_style_btn_primary;
lv_style_t nino_style_btn_success;
lv_style_t nino_style_btn_danger;

void nino_styles_init(void)
{
    // ---- Dark background ----
    lv_style_init(&nino_style_bg);
    lv_style_set_bg_color(&nino_style_bg, NINO_COLOR_BG);
    lv_style_set_bg_opa(&nino_style_bg, LV_OPA_COVER);
    lv_style_set_pad_all(&nino_style_bg, 0);
    lv_style_set_border_width(&nino_style_bg, 0);

    // ---- App button base ----
    lv_style_init(&nino_style_app_btn);
    lv_style_set_radius(&nino_style_app_btn, 20);
    lv_style_set_bg_opa(&nino_style_app_btn, LV_OPA_COVER);
    lv_style_set_border_width(&nino_style_app_btn, 0);
    lv_style_set_shadow_width(&nino_style_app_btn, 0);
    lv_style_set_pad_all(&nino_style_app_btn, 4);
    lv_style_set_align(&nino_style_app_btn, LV_ALIGN_CENTER);
    lv_style_set_layout(&nino_style_app_btn, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&nino_style_app_btn, LV_FLEX_FLOW_COLUMN);
    lv_style_set_flex_main_place(&nino_style_app_btn, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_cross_place(&nino_style_app_btn, LV_FLEX_ALIGN_CENTER);

    lv_style_init(&nino_style_app_btn_pressed);
    lv_style_set_shadow_width(&nino_style_app_btn_pressed, 0);

    // ---- Header bar (colored by app, set per object) ----
    lv_style_init(&nino_style_header_bar);
    lv_style_set_radius(&nino_style_header_bar, 0);
    lv_style_set_bg_opa(&nino_style_header_bar, LV_OPA_COVER);
    lv_style_set_pad_all(&nino_style_header_bar, 0);
    lv_style_set_border_width(&nino_style_header_bar, 0);
    lv_style_set_layout(&nino_style_header_bar, LV_LAYOUT_NONE);

    // ---- Back button (round, white) ----
    lv_style_init(&nino_style_back_btn);
    lv_style_set_radius(&nino_style_back_btn, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&nino_style_back_btn, NINO_COLOR_WHITE);
    lv_style_set_bg_opa(&nino_style_back_btn, LV_OPA_COVER);
    lv_style_set_border_width(&nino_style_back_btn, 0);
    lv_style_set_pad_all(&nino_style_back_btn, 0);

    lv_style_init(&nino_style_back_btn_pressed);
    lv_style_set_bg_color(&nino_style_back_btn_pressed, lv_color_hex(0xCCCCCC));
    lv_style_set_bg_opa(&nino_style_back_btn_pressed, LV_OPA_COVER);

    // ---- Status bar text ----
    lv_style_init(&nino_style_status_text);
    lv_style_set_text_color(&nino_style_status_text, NINO_COLOR_BAR_FG);
    lv_style_set_text_font(&nino_style_status_text, &lv_font_montserrat_12);
    lv_style_set_text_align(&nino_style_status_text, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_ver(&nino_style_status_text, 0);

    // ---- App titles ----
    lv_style_init(&nino_style_app_title_dark);
    lv_style_set_text_color(&nino_style_app_title_dark, NINO_COLOR_DARK_ICON);
    lv_style_set_text_font(&nino_style_app_title_dark, &lv_font_montserrat_24);
    lv_style_set_text_align(&nino_style_app_title_dark, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&nino_style_app_title_light);
    lv_style_set_text_color(&nino_style_app_title_light, NINO_COLOR_WHITE);
    lv_style_set_text_font(&nino_style_app_title_light, &lv_font_montserrat_24);
    lv_style_set_text_align(&nino_style_app_title_light, LV_TEXT_ALIGN_CENTER);

    // ---- Light tinted content ----
    lv_style_init(&nino_style_content);
    lv_style_set_bg_color(&nino_style_content, NINO_COLOR_CONTENT_TINT);
    lv_style_set_bg_opa(&nino_style_content, LV_OPA_COVER);
    lv_style_set_border_width(&nino_style_content, 0);
    lv_style_set_pad_all(&nino_style_content, 8);
    lv_style_set_radius(&nino_style_content, 0);

    // ---- Shared button styles ----
    lv_style_init(&nino_style_btn_primary);
    lv_style_set_bg_color(&nino_style_btn_primary, NINO_COLOR_PRIMARY);
    lv_style_set_radius(&nino_style_btn_primary, 22);
    lv_style_set_shadow_width(&nino_style_btn_primary, 0);
    lv_style_set_border_width(&nino_style_btn_primary, 0);
    lv_style_set_bg_opa(&nino_style_btn_primary, LV_OPA_COVER);

    lv_style_init(&nino_style_btn_success);
    lv_style_set_bg_color(&nino_style_btn_success, NINO_COLOR_SUCCESS);
    lv_style_set_radius(&nino_style_btn_success, 22);
    lv_style_set_shadow_width(&nino_style_btn_success, 0);
    lv_style_set_border_width(&nino_style_btn_success, 0);
    lv_style_set_bg_opa(&nino_style_btn_success, LV_OPA_COVER);

    lv_style_init(&nino_style_btn_danger);
    lv_style_set_bg_color(&nino_style_btn_danger, NINO_COLOR_DANGER);
    lv_style_set_radius(&nino_style_btn_danger, 22);
    lv_style_set_shadow_width(&nino_style_btn_danger, 0);
    lv_style_set_border_width(&nino_style_btn_danger, 0);
    lv_style_set_bg_opa(&nino_style_btn_danger, LV_OPA_COVER);
}