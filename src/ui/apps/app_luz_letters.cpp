#include "app_luz_letters.h"
#include "nino_colors.h"
#include <Arduino.h>

#define MAX_TRACE_PTS  200
#define MAX_WAYPOINTS  20

typedef struct {
    const char *letter;
    int  waypoint_count;
    lv_point_precise_t waypoints[MAX_WAYPOINTS];
} letter_def_t;

// ---- Letter definitions (outline waypoints in a 200x200 area) ----

static const letter_def_t letters[] = {
    { "L", 3, {{40,20},{40,180},{180,180}} },
    { "T", 3, {{20,20},{180,20},{100,20},{100,180}} },
    { "I", 4, {{60,20},{140,20},{100,20},{100,180},{60,180},{140,180}} },
    { "H", 5, {{40,20},{40,180},{40,100},{160,100},{160,20},{160,180}} },
    { "E", 6, {{160,20},{40,20},{40,180},{160,180},{40,100},{120,100}} },
    { "F", 5, {{160,20},{40,20},{40,180},{40,100},{120,100}} },
};
#define LETTER_COUNT (sizeof(letters) / sizeof(letters[0]))

static int current_letter = 0;
static lv_obj_t *trace_area;
static lv_obj_t *guide_line;
static lv_obj_t *trace_line;
static lv_obj_t *letter_label;
static lv_obj_t *next_btn;
static lv_point_precise_t trace_pts[MAX_TRACE_PTS];
static lv_point_precise_t guide_pts[MAX_WAYPOINTS];
static int trace_count = 0;
static lv_obj_t *content_ref;

// ---- Drawing ----

static void draw_letter(int idx)
{
    const letter_def_t *ld = &letters[idx];
    const int ox = 140;
    const int oy = 20;

    // Guide line (thick gray) — use static array; lv_line stores pointer
    for (int i = 0; i < ld->waypoint_count && i < MAX_WAYPOINTS; i++) {
        guide_pts[i].x = ld->waypoints[i].x + ox;
        guide_pts[i].y = ld->waypoints[i].y + oy;
    }
    lv_line_set_points(guide_line, guide_pts, ld->waypoint_count);

    // Reset trace
    trace_count = 0;
    lv_line_set_points_mutable(trace_line, trace_pts, 0);

    // Letter name
    char buf[8];
    snprintf(buf, sizeof(buf), "\"%s\"", ld->letter);
    lv_label_set_text(letter_label, buf);
}

// ---- Touch tracking ----

static void on_trace_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        trace_count = 0;
        lv_line_set_points_mutable(trace_line, trace_pts, 0);
        lv_point_t pt;
        lv_indev_get_point(lv_indev_active(), &pt);
        lv_obj_t *area = (lv_obj_t *)lv_event_get_target(e);
        lv_point_precise_t rel;
        rel.x = pt.x - lv_obj_get_x(area);
        rel.y = pt.y - lv_obj_get_y(area);
        if (trace_count < MAX_TRACE_PTS) {
            trace_pts[trace_count++] = rel;
        }
    }

    if (code == LV_EVENT_PRESSING) {
        lv_point_t pt;
        lv_indev_get_point(lv_indev_active(), &pt);
        lv_obj_t *area = (lv_obj_t *)lv_event_get_target(e);
        lv_point_precise_t rel;
        rel.x = pt.x - lv_obj_get_x(area);
        rel.y = pt.y - lv_obj_get_y(area);
        if (trace_count < MAX_TRACE_PTS) {
            trace_pts[trace_count++] = rel;
            lv_line_set_points_mutable(trace_line, trace_pts, trace_count);
        }
    }
}

// ---- Navigation ----

static void on_next(lv_event_t *e)
{
    (void)e;
    current_letter = (current_letter + 1) % LETTER_COUNT;
    draw_letter(current_letter);
}

void app_luz_letters_create(lv_obj_t *content)
{
    content_ref = content;
    current_letter = 0;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, "Trace the letter with your finger!");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x555555), 0);
    lv_obj_set_width(title, 460);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    // Letter name
    letter_label = lv_label_create(content);
    lv_label_set_text(letter_label, "\"L\"");
    lv_obj_set_style_text_font(letter_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(letter_label, lv_color_hex(0x333333), 0);
    lv_obj_align(letter_label, LV_ALIGN_TOP_MID, 0, 34);

    // Tracing area
    trace_area = lv_obj_create(content);
    lv_obj_remove_style_all(trace_area);
    lv_obj_set_size(trace_area, 400, 200);
    lv_obj_align(trace_area, LV_ALIGN_TOP_MID, 0, 68);
    lv_obj_set_style_bg_color(trace_area, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(trace_area, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(trace_area, 1, 0);
    lv_obj_set_style_border_color(trace_area, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_radius(trace_area, 4, 0);
    lv_obj_clear_flag(trace_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(trace_area, LV_OBJ_FLAG_CLICKABLE);

    // Guide line
    guide_line = lv_line_create(trace_area);
    lv_obj_set_style_line_width(guide_line, 4, 0);
    lv_obj_set_style_line_color(guide_line, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_line_rounded(guide_line, 1, 0);

    // Trace line
    trace_line = lv_line_create(trace_area);
    lv_obj_set_style_line_width(trace_line, 6, 0);
    lv_obj_set_style_line_color(trace_line, lv_color_hex(0xFF8C00), 0);
    lv_obj_set_style_line_rounded(trace_line, 1, 0);

    // Touch events on trace area
    lv_obj_add_event_cb(trace_area, on_trace_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(trace_area, on_trace_event, LV_EVENT_PRESSING, NULL);

    // Next button
    next_btn = lv_btn_create(content);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x3498DB), 0);
    lv_obj_set_size(next_btn, 140, 44);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_radius(next_btn, 22, 0);
    lv_obj_set_style_shadow_width(next_btn, 0, 0);
    lv_obj_add_event_cb(next_btn, on_next, LV_EVENT_CLICKED, NULL);

    lv_obj_t *next_lab = lv_label_create(next_btn);
    lv_label_set_text(next_lab, "Next \xE2\x86\x92");
    lv_obj_set_style_text_color(next_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(next_lab, &lv_font_montserrat_16, 0);
    lv_obj_center(next_lab);

    draw_letter(current_letter);
}
