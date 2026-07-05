#include "app_snip_snip.h"
#include "nino_colors.h"
#include <Arduino.h>
#include <math.h>

#define MAX_TRACE_PTS 300
#define MAX_STROKES   16
#define MAX_PATH_PTS  60
#define PATH_TYPES    4

typedef enum { PATH_STRAIGHT, PATH_ZIGZAG, PATH_WAVE, PATH_SPIRAL } path_type_t;

static const char *path_names[] = {"Straight", "Zigzag", "Wave", "Spiral"};

static int current_path = 0;

static lv_obj_t *trace_area;
static lv_obj_t *guide_line;
static lv_obj_t *path_label;
static lv_obj_t *next_btn;

static lv_point_precise_t path_pts[MAX_PATH_PTS];
static int path_count = 0;

// Shared point buffer + per-stroke line objects
static lv_point_precise_t pts[MAX_TRACE_PTS];
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
    if (pt_count >= MAX_TRACE_PTS) {
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
        if (pt_count >= MAX_TRACE_PTS) return;
    }
    pts[pt_count++] = p;
}

static void gen_straight(void)
{
    path_count = 2;
    path_pts[0].x = 40;  path_pts[0].y = 100;
    path_pts[1].x = 360; path_pts[1].y = 100;
}

static void gen_zigzag(void)
{
    path_count = 0;
    for (int i = 0; i < 6 && path_count < MAX_PATH_PTS; i++) {
        path_pts[path_count].x = 40 + i * 55;
        path_pts[path_count].y = (i % 2 == 0) ? 160 : 40;
        path_count++;
    }
}

static void gen_wave(void)
{
    path_count = 0;
    for (int i = 0; i <= 20 && path_count < MAX_PATH_PTS; i++) {
        float t = (float)i / 20.0f;
        path_pts[path_count].x = (int)(40 + t * 320);
        path_pts[path_count].y = (int)(100 + 50 * sinf(t * 4 * 3.14159f));
        path_count++;
    }
}

static void gen_spiral(void)
{
    path_count = 0;
    for (int i = 0; i <= 30 && path_count < MAX_PATH_PTS; i++) {
        float t = (float)i / 30.0f * 4 * 3.14159f;
        float r = 10 + t * 5;
        path_pts[path_count].x = (int)(200 + r * cosf(t));
        path_pts[path_count].y = (int)(100 + r * sinf(t));
        path_count++;
    }
}

static void (* const gen_funcs[])(void) = {gen_straight, gen_zigzag, gen_wave, gen_spiral};

static void draw_path(int idx)
{
    gen_funcs[idx]();

    // Guide line (dashed)
    lv_line_set_points(guide_line, path_pts, path_count);

    clear_strokes();

    char buf[24];
    snprintf(buf, sizeof(buf), "%s", path_names[idx]);
    lv_label_set_text(path_label, buf);
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
        if (stroke_count < MAX_STROKES) {
            strokes[stroke_count] = lv_line_create(trace_area);
            lv_obj_set_style_line_width(strokes[stroke_count], 6, 0);
            lv_obj_set_style_line_color(strokes[stroke_count], lv_color_hex(0x27AE60), 0);
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

static void on_next(lv_event_t *e)
{
    (void)e;
    current_path = (current_path + 1) % PATH_TYPES;
    draw_path(current_path);
}

void app_snip_snip_create(lv_obj_t *content)
{
    current_path = 0;
    stroke_count = 0;
    pt_count = 0;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Title + path name row
    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, "Trace along the dotted line!");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x555555), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 6);

    path_label = lv_label_create(content);
    lv_label_set_text(path_label, "Straight");
    lv_obj_set_style_text_font(path_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(path_label, lv_color_hex(0x333333), 0);
    lv_obj_align(path_label, LV_ALIGN_TOP_RIGHT, -10, 4);

    // Tracing area — left side, fully visible
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

    // Guide line (solid gray, matching letters guide)
    guide_line = lv_line_create(trace_area);
    lv_obj_set_style_line_width(guide_line, 4, 0);
    lv_obj_set_style_line_color(guide_line, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_line_rounded(guide_line, 1, 0);

    // Touch events
    lv_obj_add_event_cb(trace_area, on_trace_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(trace_area, on_trace_event, LV_EVENT_PRESSING, NULL);

    // Next button — right side
    next_btn = lv_btn_create(content);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x8E44AD), 0);
    lv_obj_set_size(next_btn, 80, 44);
    lv_obj_align(next_btn, LV_ALIGN_CENTER, 185, 6);
    lv_obj_set_style_radius(next_btn, 22, 0);
    lv_obj_set_style_shadow_width(next_btn, 0, 0);
    lv_obj_add_event_cb(next_btn, on_next, LV_EVENT_CLICKED, NULL);

    lv_obj_t *next_lab = lv_label_create(next_btn);
    lv_label_set_text(next_lab, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(next_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(next_lab, &lv_font_montserrat_16, 0);
    lv_obj_center(next_lab);

    // Clear button — below next
    lv_obj_t *clear_btn = lv_btn_create(content);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0xE74C3C), 0);
    lv_obj_set_size(clear_btn, 80, 44);
    lv_obj_align(clear_btn, LV_ALIGN_CENTER, 185, 64);
    lv_obj_set_style_radius(clear_btn, 22, 0);
    lv_obj_set_style_shadow_width(clear_btn, 0, 0);
    lv_obj_add_event_cb(clear_btn, on_clear, LV_EVENT_CLICKED, NULL);

    lv_obj_t *clear_lab = lv_label_create(clear_btn);
    lv_label_set_text(clear_lab, LV_SYMBOL_TRASH);
    lv_obj_set_style_text_color(clear_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(clear_lab, &lv_font_montserrat_16, 0);
    lv_obj_center(clear_lab);

    draw_path(0);
}
