/**
 * scr_home.cpp — NinoPad home screen.
 * Layout (480x320 landscape):
 *   row0: 32px height status bar (WiFi | 09:41 | battery)
 *   row1: 40px height "NinoPad" title in white
 *   row2: 248px 3x3 grid of large rounded-square app buttons
 */
#include "scr_home.h"
#include "nino_colors.h"
#include "nino_styles.h"
#include "../utils/anim_utils.h"
#include "../icons/nino_icons.h"
#include "../apps/app_registry.h"
#include "screen_manager.h"

static void on_btn_clicked(lv_anim_t *a)
{
    nino_app_id_t id = (nino_app_id_t)(intptr_t)a->user_data;
    nino_screen_show_app(id);
}

static void on_btn_press(lv_event_t *e)
{
    nino_app_id_t id = (nino_app_id_t)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_current_target(e);
    nino_anim_bounce(btn, 200, 290, on_btn_clicked, (void *)(intptr_t)id);
}

static void build_button(lv_obj_t *grid, const nino_app_t *app, nino_app_id_t id,
                         int32_t col, int32_t row)
{
    lv_obj_t *btn = lv_obj_create(grid);
    lv_obj_add_style(btn, &nino_style_app_btn, 0);
    lv_obj_set_style_bg_color(btn, app->color, 0);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1,
                                LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    // icon container (50x50 transparent box; icon draws inside)
    lv_obj_t *icon_box = lv_obj_create(btn);
    lv_obj_remove_style_all(icon_box);
    lv_obj_set_size(icon_box, 50, 50);
    if (app->draw_icon) app->draw_icon(icon_box, NINO_COLOR_DARK_ICON);

    // label below icon
    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, app->name);
    lv_obj_set_style_text_color(lab, NINO_COLOR_WHITE, 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(lab, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lab, 110);

    lv_obj_add_event_cb(btn, on_btn_press, LV_EVENT_SHORT_CLICKED, (void *)(intptr_t)id);
}

void scr_home_create(lv_obj_t *scr)
{
    lv_obj_add_style(scr, &nino_style_bg, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ---- Status bar ----
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 480, 28);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, NINO_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    // WiFi (left)
    lv_obj_t *wifi = lv_obj_create(bar);
    lv_obj_remove_style_all(wifi);
    lv_obj_set_size(wifi, 28, 22);
    lv_obj_align(wifi, LV_ALIGN_LEFT_MID, 4, 0);
    nino_icon_wifi(wifi, NINO_COLOR_WHITE);

    // time (center)
    lv_obj_t *time = lv_label_create(bar);
    lv_label_set_text(time, "09:41");
    lv_obj_align(time, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(time, &nino_style_status_text, 0);
    lv_obj_set_style_text_font(time, &lv_font_montserrat_16, 0);

    // battery (right)
    lv_obj_t *batt = lv_obj_create(bar);
    lv_obj_remove_style_all(batt);
    lv_obj_set_size(batt, 28, 22);
    lv_obj_align(batt, LV_ALIGN_RIGHT_MID, -4, 0);
    nino_icon_battery(batt, NINO_COLOR_WHITE);

    // ---- NinoPad title ----
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "NinoPad");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_text_color(title, NINO_COLOR_WHITE, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    // ---- 3x3 grid ----
    static int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, 470, 240);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(grid, 8, 0);
    lv_obj_set_style_pad_row(grid, 8, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < APP_COUNT; ++i) {
        const nino_app_t *app = nino_app_get((nino_app_id_t)i);
        int col = i % 3;
        int row = i / 3;
        build_button(grid, app, (nino_app_id_t)i, col, row);
    }
}