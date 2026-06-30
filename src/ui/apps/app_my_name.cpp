/**
 * app_my_name.cpp — NinoPad "My Name" app.
 * Layout:
 *   - "My name is" (italic-ish) label left-aligned
 *   - Dashed-border name textarea (style border_dash_gap/width)
 *   - Bordered canvas placeholder with "DRAW HERE" centered text
 * Real drawing/tracing engine is deferred (see PLAN.md).
 */
#include "app_my_name.h"
#include "storage/settings.h"
#include "nino_colors.h"
#include "nino_styles.h"

void app_my_name_create(lv_obj_t *content)
{
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // "My name is" label (italic-like: bold serif would need custom font; use default large)
    lv_obj_t *intro = lv_label_create(content);
    lv_label_set_text(intro, "My name is");
    lv_obj_set_style_text_color(intro, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(intro, &lv_font_montserrat_24, 0);
    lv_obj_align(intro, LV_ALIGN_TOP_LEFT, 8, 6);

    // name textarea with border (v9 lacks border-dash style; dashed border is
    // deferred — see app_my_name TODO for tracing engine. Using plain border.)
    lv_obj_t *name_ta = lv_textarea_create(content);
    lv_obj_set_size(name_ta, 350, 44);
    lv_obj_align(name_ta, LV_ALIGN_TOP_LEFT, 8, 42);
    lv_obj_set_style_border_color(name_ta, NINO_COLOR_DASH, 0);
    lv_obj_set_style_border_width(name_ta, 2, 0);
    lv_obj_set_style_border_side(name_ta, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_bg_opa(name_ta, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(name_ta, 6, 0);

    // load saved name if any
    char namebuf[64];
    if (nino_settings_get_name(namebuf, sizeof(namebuf)) && namebuf[0]) {
        lv_textarea_set_text(name_ta, namebuf);
    } else {
        lv_textarea_set_placeholder_text(name_ta, "type name...");
    }

    // ---- DRAW HERE canvas placeholder ----
    lv_obj_t *canvas = lv_obj_create(content);
    lv_obj_set_size(canvas, 462, 188);
    lv_obj_align(canvas, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_border_color(canvas, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_border_width(canvas, 1, 0);
    lv_obj_set_style_border_side(canvas, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_bg_opa(canvas, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *ph = lv_label_create(canvas);
    lv_label_set_text(ph, "DRAW HERE");
    lv_obj_set_style_text_color(ph, lv_color_hex(0xBBBBBB), 0);
    lv_obj_set_style_text_font(ph, &lv_font_montserrat_24, 0);
    lv_obj_center(ph);

    // TODO(tracing engine): capture LV_EVENT_PRESSING, validate stroke against guide path,
    //   green/red feedback dot, success star burst + audio ding. See PLAN.md "Tracing engine".
    (void)canvas;
}