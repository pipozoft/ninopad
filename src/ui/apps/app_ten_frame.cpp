#include "app_ten_frame.h"
#include "nino_colors.h"
#include <Arduino.h>

#define CELLS      10
#define COLS        5

static lv_obj_t *cells[CELLS];
static lv_obj_t *count_label;
static lv_obj_t *decomp_label;
static lv_obj_t *content_ref;
static int cell_values[CELLS];
static bool make10_mode = false;
static int make10_target;

static void update_display(void);

static void on_cell_tap(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= CELLS) return;

    if (make10_mode) {
        if (cell_values[idx] == 0) {
            cell_values[idx] = 1;
        } else {
            return;
        }
    } else {
        cell_values[idx] = (cell_values[idx] + 1) % 3;
    }
    update_display();
}

static void refresh_cell_colors(void)
{
    for (int i = 0; i < CELLS; i++) {
        if (cell_values[i] == 0) {
            lv_obj_set_style_bg_color(cells[i], lv_color_hex(0x333333), 0);
            lv_obj_set_style_border_color(cells[i], lv_color_hex(0x555555), 0);
        } else if (cell_values[i] == 1) {
            lv_obj_set_style_bg_color(cells[i], lv_color_hex(0xFF8C00), 0);
            lv_obj_set_style_border_color(cells[i], lv_color_hex(0xFFA500), 0);
        } else {
            lv_obj_set_style_bg_color(cells[i], lv_color_hex(0xCC3333), 0);
            lv_obj_set_style_border_color(cells[i], lv_color_hex(0xEE4444), 0);
        }
    }
}

static bool any_cell_empty(void)
{
    for (int i = 0; i < CELLS; i++)
        if (cell_values[i] == 0) return true;
    return false;
}

static int total_count(void)
{
    int t = 0;
    for (int i = 0; i < CELLS; i++) t += cell_values[i];
    return t;
}

static void update_display(void)
{
    refresh_cell_colors();
    int total = total_count();
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", total);
    lv_label_set_text(count_label, buf);

    if (make10_mode && total == 10) {
        lv_label_set_text(decomp_label, "\xE2\x98\x85 You made 10!");
    } else if (total > 0) {
        int ones = 0, twos = 0;
        for (int i = 0; i < CELLS; i++) {
            if (cell_values[i] == 1) ones++;
            else if (cell_values[i] == 2) twos++;
        }
        snprintf(buf, sizeof(buf), "%d + %d = %d", ones, twos * 2, total);
        lv_label_set_text(decomp_label, buf);
    } else {
        lv_label_set_text(decomp_label, "Tap a cell");
    }

    if (make10_mode && total >= make10_target) {
        lv_obj_clear_flag(count_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void reset_cells(int value)
{
    for (int i = 0; i < CELLS; i++) cell_values[i] = value;
}

static void reset_game(void)
{
    reset_cells(0);
    if (make10_mode) {
        make10_target = 10;
        int seed = random(4, 9);
        int placed = 0;
        while (placed < seed) {
            int r = random(CELLS);
            if (cell_values[r] == 0) {
                cell_values[r] = 1;
                placed++;
            }
        }
    }
    update_display();
}

static void on_free_mode(lv_event_t *e)
{
    (void)e;
    make10_mode = false;
    reset_game();
}

static void on_make10_mode(lv_event_t *e)
{
    (void)e;
    make10_mode = true;
    reset_game();
}

void app_ten_frame_create(lv_obj_t *content)
{
    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    content_ref = content;

    // Grid area (left side)
    lv_obj_t *grid_area = lv_obj_create(content);
    lv_obj_remove_style_all(grid_area);
    lv_obj_set_size(grid_area, 310, 140);
    lv_obj_align(grid_area, LV_ALIGN_LEFT_MID, 10, -10);

    int cell_w = 52;
    int cell_h = 52;
    int gap = 6;

    for (int i = 0; i < CELLS; i++) {
        cell_values[i] = 0;
        int col = i % COLS;
        int row = i / COLS;
        int x = col * (cell_w + gap);
        int y = row * (cell_h + gap);

        cells[i] = lv_obj_create(grid_area);
        lv_obj_remove_style_all(cells[i]);
        lv_obj_set_size(cells[i], cell_w, cell_h);
        lv_obj_set_pos(cells[i], x, y);
        lv_obj_set_style_border_width(cells[i], 2, 0);
        lv_obj_set_style_border_color(cells[i], lv_color_hex(0x555555), 0);
        lv_obj_set_style_bg_color(cells[i], lv_color_hex(0x333333), 0);
        lv_obj_set_style_bg_opa(cells[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(cells[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_shadow_width(cells[i], 0, 0);
        lv_obj_clear_flag(cells[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(cells[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(cells[i], on_cell_tap, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    // Right side: count + decomposition
    lv_obj_t *info_area = lv_obj_create(content);
    lv_obj_remove_style_all(info_area);
    lv_obj_set_size(info_area, 150, 160);
    lv_obj_align(info_area, LV_ALIGN_RIGHT_MID, -10, -10);
    lv_obj_set_flex_flow(info_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_area, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    count_label = lv_label_create(info_area);
    lv_label_set_text(count_label, "0");
    lv_obj_set_style_text_font(count_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(count_label, lv_color_hex(0x333333), 0);

    decomp_label = lv_label_create(info_area);
    lv_label_set_text(decomp_label, "Tap a cell");
    lv_obj_set_style_text_font(decomp_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(decomp_label, lv_color_hex(0x666666), 0);
    lv_obj_set_style_pad_top(decomp_label, 8, 0);

    // Bottom mode buttons
    lv_obj_t *mode_bar = lv_obj_create(content);
    lv_obj_remove_style_all(mode_bar);
    lv_obj_set_size(mode_bar, 460, 40);
    lv_obj_align(mode_bar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_flex_flow(mode_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *free_btn = lv_btn_create(mode_bar);
    lv_obj_set_style_bg_color(free_btn, lv_color_hex(0x3498DB), 0);
    lv_obj_set_size(free_btn, 140, 34);
    lv_obj_set_style_radius(free_btn, 17, 0);
    lv_obj_set_style_shadow_width(free_btn, 0, 0);
    lv_obj_add_event_cb(free_btn, on_free_mode, LV_EVENT_CLICKED, NULL);
    lv_obj_t *free_lab = lv_label_create(free_btn);
    lv_label_set_text(free_lab, "Free Play");
    lv_obj_set_style_text_color(free_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(free_lab);

    lv_obj_t *m10_btn = lv_btn_create(mode_bar);
    lv_obj_set_style_bg_color(m10_btn, lv_color_hex(0x27AE60), 0);
    lv_obj_set_size(m10_btn, 140, 34);
    lv_obj_set_style_radius(m10_btn, 17, 0);
    lv_obj_set_style_shadow_width(m10_btn, 0, 0);
    lv_obj_add_event_cb(m10_btn, on_make10_mode, LV_EVENT_CLICKED, NULL);
    lv_obj_t *m10_lab = lv_label_create(m10_btn);
    lv_label_set_text(m10_lab, "Make 10");
    lv_obj_set_style_text_color(m10_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(m10_lab);

    reset_game();
}
