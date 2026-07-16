#include "app_luz_letters.h"
#include "nino_colors.h"
#include <Arduino.h>
#include "KG.c"

#define MAX_PTS        400
#define MAX_STROKES    16

static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
#define CHAR_COUNT (sizeof(chars) - 1)

static int current_idx = 0;
static lv_obj_t *trace_area;
static lv_obj_t *guide_label;
static lv_obj_t *letter_label;
static lv_obj_t *next_btn;

// Shared point buffer + per-stroke line objects
static lv_point_precise_t pts[MAX_PTS];
static int pt_count = 0;
static lv_obj_t *strokes[MAX_STROKES];
static int stroke_start[MAX_STROKES];
static int stroke_end[MAX_STROKES];
static int stroke_count = 0;

static void clear_strokes(void)
{
    for (int i = 0; i < stroke_count; i++) {
        if (strokes[i]) lv_obj_delete(strokes[i]);
    }
    stroke_count = 0;
    pt_count = 0;
}

static void add_stroke_point(lv_point_precise_t p)
{
    if (pt_count >= MAX_PTS) {
        // Drop oldest stroke to free points
        if (stroke_count > 0) {
            lv_obj_delete(strokes[0]);
            int drop = stroke_end[0] - stroke_start[0];
            for (int i = 1; i < stroke_count; i++) {
                strokes[i - 1] = strokes[i];
                stroke_start[i - 1] = stroke_start[i] - drop;
                stroke_end[i - 1] = stroke_end[i] - drop;
            }
            stroke_count--;
            pt_count -= drop;
            memmove(pts, pts + drop, pt_count * sizeof(lv_point_precise_t));
        }
        if (pt_count >= MAX_PTS) return;
    }
    pts[pt_count++] = p;
}

// ---- Drawing ----

static void draw_char(int idx)
{
    char c[2] = { chars[idx], '\0' };
    lv_label_set_text(guide_label, c);
    lv_obj_center(guide_label);

    clear_strokes();

    char buf[8];
    snprintf(buf, sizeof(buf), "\"%c\"", chars[idx]);
    lv_label_set_text(letter_label, buf);
}

// ---- Touch tracking ----

static lv_point_precise_t get_trace_point(lv_obj_t *area)
{
    lv_point_t pt;
    lv_indev_get_point(lv_indev_active(), &pt);

    lv_area_t coords;
    lv_obj_get_coords(area, &coords);

    lv_point_precise_t rel;
    rel.x = pt.x - coords.x1;
    rel.y = pt.y - coords.y1;
    return rel;
}

static void on_clear(lv_event_t *e)
{
    (void)e;
    clear_strokes();
}

static void on_trace_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *area = (lv_obj_t *)lv_event_get_target(e);

    if (code == LV_EVENT_PRESSED) {
        // Start a new stroke
        if (stroke_count < MAX_STROKES) {
            strokes[stroke_count] = lv_line_create(trace_area);
            lv_obj_set_style_line_width(strokes[stroke_count], 6, 0);
            lv_obj_set_style_line_color(strokes[stroke_count], lv_color_hex(0xFF8C00), 0);
            lv_obj_set_style_line_rounded(strokes[stroke_count], 1, 0);
            stroke_start[stroke_count] = pt_count;
            stroke_end[stroke_count] = pt_count;
            stroke_count++;
        }
    } else if (code == LV_EVENT_PRESSING) {
        if (stroke_count == 0) return;
        lv_point_precise_t rel = get_trace_point(area);
        add_stroke_point(rel);
        int si = stroke_count - 1;
        stroke_end[si] = pt_count;
        int count = stroke_end[si] - stroke_start[si];
        lv_line_set_points_mutable(strokes[si], &pts[stroke_start[si]], count);
    }
}

// ---- Navigation ----

static void on_prev(lv_event_t *e)
{
    (void)e;
    current_idx = (current_idx - 1 + CHAR_COUNT) % CHAR_COUNT;
    draw_char(current_idx);
}

static void on_next(lv_event_t *e)
{
    (void)e;
    current_idx = (current_idx + 1) % CHAR_COUNT;
    draw_char(current_idx);
}

void app_luz_letters_create(lv_obj_t *content)
{
    current_idx = 0;
    stroke_count = 0;
    pt_count = 0;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Title + letter name row
    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, "Trace the letter!");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x555555), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 6);

    letter_label = lv_label_create(content);
    lv_label_set_text(letter_label, "\"A\"");
    lv_obj_set_style_text_font(letter_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(letter_label, lv_color_hex(0x333333), 0);
    lv_obj_align(letter_label, LV_ALIGN_TOP_RIGHT, -10, 4);

    // Tracing area — left side
    trace_area = lv_obj_create(content);
    lv_obj_remove_style_all(trace_area);
    lv_obj_set_size(trace_area, 360, 220);
    lv_obj_align(trace_area, LV_ALIGN_TOP_LEFT, 0, 40);
    lv_obj_set_style_bg_color(trace_area, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(trace_area, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(trace_area, 1, 0);
    lv_obj_set_style_border_color(trace_area, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_radius(trace_area, 4, 0);
    lv_obj_clear_flag(trace_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(trace_area, LV_OBJ_FLAG_CLICKABLE);

    // Guide label (KG dots font, scaled up) — behind trace lines
    guide_label = lv_label_create(trace_area);
    lv_label_set_text(guide_label, "A");
    lv_obj_set_style_text_font(guide_label, &KG, 0);
    lv_obj_set_style_text_color(guide_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_transform_pivot_x(guide_label, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(guide_label, LV_PCT(50), 0);
    lv_obj_set_style_transform_scale(guide_label, 1200, 0);
    lv_obj_center(guide_label);

    // Touch events on trace area
    lv_obj_add_event_cb(trace_area, on_trace_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(trace_area, on_trace_event, LV_EVENT_PRESSING, NULL);

    // Next button — right side, top
    next_btn = lv_btn_create(content);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x3498DB), 0);
    lv_obj_set_size(next_btn, 80, 44);
    lv_obj_align(next_btn, LV_ALIGN_CENTER, 185, -10);
    lv_obj_set_style_radius(next_btn, 22, 0);
    lv_obj_set_style_shadow_width(next_btn, 0, 0);
    lv_obj_add_event_cb(next_btn, on_next, LV_EVENT_CLICKED, NULL);

    lv_obj_t *next_lab = lv_label_create(next_btn);
    lv_label_set_text(next_lab, "Next");
    lv_obj_set_style_text_color(next_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(next_lab, &lv_font_montserrat_16, 0);
    lv_obj_center(next_lab);

    // Prev button — below next
    lv_obj_t *prev_btn = lv_btn_create(content);
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0x3498DB), 0);
    lv_obj_set_size(prev_btn, 80, 44);
    lv_obj_align(prev_btn, LV_ALIGN_CENTER, 185, 44);
    lv_obj_set_style_radius(prev_btn, 22, 0);
    lv_obj_set_style_shadow_width(prev_btn, 0, 0);
    lv_obj_add_event_cb(prev_btn, on_prev, LV_EVENT_CLICKED, NULL);

    lv_obj_t *prev_lab = lv_label_create(prev_btn);
    lv_label_set_text(prev_lab, "Prev");
    lv_obj_set_style_text_color(prev_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(prev_lab, &lv_font_montserrat_16, 0);
    lv_obj_center(prev_lab);

    // Clear button — below prev
    lv_obj_t *clear_btn = lv_btn_create(content);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0xE74C3C), 0);
    lv_obj_set_size(clear_btn, 80, 44);
    lv_obj_align(clear_btn, LV_ALIGN_CENTER, 185, 98);
    lv_obj_set_style_radius(clear_btn, 22, 0);
    lv_obj_set_style_shadow_width(clear_btn, 0, 0);
    lv_obj_add_event_cb(clear_btn, on_clear, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clear_lab = lv_label_create(clear_btn);
    lv_label_set_text(clear_lab, "Clear");
    lv_obj_set_style_text_color(clear_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(clear_lab, &lv_font_montserrat_16, 0);
    lv_obj_center(clear_lab);

    draw_char(0);
}
