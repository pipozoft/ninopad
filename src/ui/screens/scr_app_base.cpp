/**
 * scr_app_base.cpp — app frame: colored header bar, round back button, content area.
 *
 * The back button uses lv_async_call to defer the screen transition until after
 * the event processing cycle completes. This avoids deleting the button that
 * triggered the event (via lv_obj_clean in nino_screen_show_home).
 */
#include "scr_app_base.h"
#include "screen_manager.h"
#include "nino_colors.h"
#include "nino_styles.h"

#define HEADER_H  44

static void on_back(lv_event_t * /*e*/)
{
    lv_async_call([](void*) { nino_screen_show_home(); }, NULL);
}

void scr_app_base_create(lv_obj_t *scr, nino_app_id_t id)
{
    const nino_app_t *app = nino_app_get(id);

    lv_obj_set_style_bg_color(scr, NINO_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ---- Header bar ----
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_add_style(bar, &nino_style_header_bar, 0);
    lv_obj_set_style_bg_color(bar, app->color, 0);
    lv_obj_set_size(bar, 480, HEADER_H);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    // Back circle (left)
    lv_obj_t *back = lv_obj_create(bar);
    lv_obj_add_style(back, &nino_style_back_btn, 0);
    lv_obj_set_size(back, 36, 36);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_clear_flag(back, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *back_glyph = lv_label_create(back);
    lv_label_set_text(back_glyph, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_glyph, NINO_COLOR_BACK_FG, 0);
    lv_obj_set_style_text_font(back_glyph, &lv_font_montserrat_16, 0);
    lv_obj_center(back_glyph);

    lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, NULL);

    // Title (center)
    lv_obj_t *title = lv_label_create(bar);
    lv_label_set_text(title, app->name);
    lv_obj_set_style_text_color(title, NINO_COLOR_DARK_ICON, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // ---- Content area ----
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_add_style(content, &nino_style_content, 0);
    lv_obj_set_size(content, 480, 320 - HEADER_H);
    lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    if (app->create) {
        app->create(content);
    }
}