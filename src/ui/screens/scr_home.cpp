/**
 * scr_home.cpp — home screen: status bar (logo, time, wifi) + 3×3 app grid.
 *
 * Timer lifecycle: a 5-second LVGL timer (status_timer) updates time and wifi.
 *
 * On screen transition away, nino_home_stop() destroys the timer and nulls
 * static pointers BEFORE lv_obj_clean() — essential to prevent dangling
 * pointer crashes.
 *
 * Debug overlay: tap the time in the status bar to toggle LVGL sysmon.
 */
#include "scr_home.h"
#include "screen_manager.h"
#include "nino_colors.h"
#include "nino_styles.h"
#include "apps/app_registry.h"
#include "utils/wifi_utils.h"
#include <Arduino.h>
#include <time.h>

#include "fonts/icons_font.h"
#include "fonts/icon_codepoints.h"
#include "debugging/sysmon/lv_sysmon.h"

#define STATUS_H    26

static const char *icon_chars[APP_COUNT] = {
    "\xEE\xA8\x85",   // ICON_FACE_SMILE  → APP_MY_NAME
    "\xEE\xA8\x86",   // ICON_FIRE        → APP_LUZ_LETTERS
    "\xEE\xA8\x84",   // ICON_EYE         → APP_WORD_SPY
    "\xEE\xA8\x83",   // ICON_DOLLAR      → APP_COUNTING_JAR
    "\xEE\xA8\x8A",   // ICON_SUN         → APP_TEN_FRAME
    "\xEE\xA8\x88",   // ICON_PAINT_BRUSH → APP_SHAPE_PAINT
    "\xEE\xA8\x89",   // ICON_SCISSORS    → APP_SNIP_SNIP
    "\xEE\xA8\x81",   // ICON_BOOK_OPEN   → APP_STORY_TIME
    "\xEE\xA8\x82",   // ICON_COG         → APP_SETTINGS
};

static lv_obj_t *time_label = NULL;
static lv_obj_t *wifi_label = NULL;
static lv_obj_t *status_bar_ref = NULL;
static lv_timer_t *status_timer = NULL;
static bool sysmon_showing = false;

static void on_time_tap(lv_event_t *e)
{
    (void)e;
    lv_display_t *d = lv_display_get_default();
    if (sysmon_showing) {
        lv_sysmon_hide_performance(d);
        sysmon_showing = false;
    } else {
        lv_sysmon_show_performance(d);
        lv_sysmon_performance_pause(d);
        sysmon_showing = true;
    }
}

static void on_app_tap(lv_event_t *e)
{
    nino_app_id_t id = (nino_app_id_t)(intptr_t)lv_event_get_user_data(e);
    nino_screen_show_app(id);
}

static void update_status_cb(lv_timer_t *t)
{
    (void)t;

    if (nino_ntp_has_time())
    {
        time_t now = time(nullptr);
        struct tm *ti = localtime(&now);
        char buf[12];
        int h = ti->tm_hour;
        int m = ti->tm_min;
        if (h >= 12) {
            h = (h == 12) ? 12 : h - 12;
            snprintf(buf, sizeof(buf), "%d:%02d pm", h, m);
        } else {
            h = (h == 0) ? 12 : h;
            snprintf(buf, sizeof(buf), "%d:%02d am", h, m);
        }
        if (time_label) lv_label_set_text(time_label, buf);
    }

    if (wifi_label)
    {
        lv_label_set_text(wifi_label, nino_wifi_is_connected() ? "Connected" : "Offline");
    }

    if (sysmon_showing) {
        lv_display_t *d = lv_display_get_default();
        lv_sysmon_performance_dump(d);
    }

    if (status_bar_ref) lv_obj_invalidate(lv_scr_act());
}

static lv_obj_t *create_app_button(lv_obj_t *parent, const nino_app_t *app, nino_app_id_t id,
                                   int btn_w, int btn_h)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_add_style(btn, &nino_style_app_btn, 0);
    lv_obj_set_style_bg_color(btn, app->color, 0);
    lv_obj_set_size(btn, btn_w, btn_h);

    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(btn, 2, 0);
    lv_obj_set_style_pad_row(btn, 2, 0);
    lv_obj_add_event_cb(btn, on_app_tap, LV_EVENT_CLICKED, (void *)(intptr_t)id);

    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text(icon, icon_chars[id]);
    lv_obj_set_style_text_font(icon, &icons_font, 0);
    lv_obj_set_style_text_color(icon, NINO_COLOR_BG, 0);

    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, app->name);
    lv_obj_set_style_text_color(lab, NINO_COLOR_BG, 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_14, 0);

    return btn;
}

void nino_home_stop(void)
{
    if (status_timer)
    {
        lv_timer_del(status_timer);
        status_timer = NULL;
    }
    time_label = NULL;
    wifi_label = NULL;
    status_bar_ref = NULL;
}

void scr_home_create(lv_obj_t *scr)
{
    nino_home_stop();

    lv_obj_set_style_bg_color(scr, NINO_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    int w = 480;
    int pad_sides = 10;
    int gap = 8;
    int btn_w = (w - 2 * pad_sides - 2 * gap) / 3;
    int btn_h = 84;
    int grid_top = STATUS_H + 8;

    // ---- Status bar ----
    lv_obj_t *status_bar = lv_obj_create(scr);
    lv_obj_set_size(status_bar, w, STATUS_H);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_set_style_bg_color(status_bar, NINO_COLOR_BAR_BG, 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_pad_all(status_bar, 0, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_CLICKABLE);
    status_bar_ref = status_bar;

    lv_obj_t *logo_lab = lv_label_create(status_bar);
    lv_label_set_text(logo_lab, "NinoPad");
    lv_obj_set_width(logo_lab, 75);
    lv_obj_set_style_text_color(logo_lab, NINO_COLOR_BAR_FG, 0);
    lv_obj_set_style_text_font(logo_lab, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_opa(logo_lab, LV_OPA_50, 0);
    lv_obj_align(logo_lab, LV_ALIGN_LEFT_MID, pad_sides, 0);

    time_label = lv_label_create(status_bar);
    lv_label_set_text(time_label, "--:-- --");
    lv_obj_set_width(time_label, 90);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(time_label, NINO_COLOR_BAR_FG, 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(time_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(time_label, on_time_tap, LV_EVENT_CLICKED, NULL);

    wifi_label = lv_label_create(status_bar);
    lv_label_set_text(wifi_label, nino_wifi_is_connected() ? "Connected" : "Offline");
    lv_obj_set_width(wifi_label, 75);
    lv_obj_set_style_text_color(wifi_label, NINO_COLOR_BAR_FG, 0);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_12, 0);
    lv_obj_align(wifi_label, LV_ALIGN_RIGHT_MID, -pad_sides, 0);

    // ---- 3x3 App grid ----
    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, w - 2 * pad_sides, 3 * btn_h + 2 * gap);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, grid_top);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, gap, 0);
    lv_obj_set_style_pad_column(grid, gap, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);

    static lv_coord_t col_dsc[] = {btn_w, btn_w, btn_w, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {btn_h, btn_h, btn_h, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

    for (int i = 0; i < APP_COUNT; i++)
    {
        const nino_app_t *app = nino_app_get((nino_app_id_t)i);
        lv_obj_t *btn = create_app_button(grid, app, (nino_app_id_t)i, btn_w, btn_h);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, i % 3, 1,
                                  LV_GRID_ALIGN_CENTER, i / 3, 1);
    }

    status_timer = lv_timer_create(update_status_cb, 5000, NULL);
    update_status_cb(status_timer);

    lv_obj_invalidate(scr);
}
