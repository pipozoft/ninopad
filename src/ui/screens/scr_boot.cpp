/**
 * scr_boot.cpp — NinoPad boot screen.
 * - Dark bg (#1A1A1A)
 * - 7 individually colored letters forming "NinoPad"
 * - Subtle pulse anim on the row
 * - Auto-advance to home after 2s, or tap anywhere to skip
 */
#include "scr_boot.h"
#include "screen_manager.h"
#include "nino_colors.h"
#include "nino_styles.h"
#include "../utils/anim_utils.h"

static bool advanced = false;

static void advance_home(void)
{
    if (advanced) return;
    advanced = true;
    nino_screen_show_home();
}

static void on_tap_skip(lv_event_t * /*e*/)
{
    advance_home();
}

static void on_boot_timer(lv_timer_t * /*t*/)
{
    advance_home();
}

void scr_boot_create(lv_obj_t *scr)
{
    advanced = false;
    lv_obj_add_style(scr, &nino_style_bg, 0);

    bool clickable_prev = true;
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    (void)clickable_prev;

    // ---- centered letter row ----
    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 0);
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

    // pulse on the row (slight scale up/down, infinite)
    nino_anim_pulse(row, 700, 280, -1);

    // tap-anywhere skip + 2s auto-advance
    lv_obj_add_event_cb(scr, on_tap_skip, LV_EVENT_CLICKED, NULL);
    lv_timer_t *t = lv_timer_create(on_boot_timer, 2000, NULL);
    lv_timer_set_repeat_count(t, 1);
}