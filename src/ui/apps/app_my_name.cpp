#include "app_my_name.h"
#include "storage/settings.h"
#include "nino_colors.h"
#include "fonts/KG.h"

#define PT_MAX  4096
#define ST_MAX  128
#define MIN_DIST 4
#define PAL_SZ  8

// ---- Drawing state ----

static lv_point_precise_t pts[PT_MAX];
static int st_start[ST_MAX];
static lv_obj_t *st_line[ST_MAX];
static int pt_n = 0;
static int st_n = 0;
static int cur_col = 0;
static bool tr_mode = false;

static lv_obj_t *canvas = NULL;
static lv_obj_t *guide = NULL;
static lv_obj_t *pal_btns[PAL_SZ];
static lv_obj_t *mode_btn[2];
static const lv_color_t palette[PAL_SZ] = {
    lv_color_hex(0xE74C3C),
    lv_color_hex(0xE67E22),
    lv_color_hex(0xF1C40F),
    lv_color_hex(0x2ECC71),
    lv_color_hex(0x1ABC9C),
    lv_color_hex(0x3498DB),
    lv_color_hex(0x9B59B6),
    lv_color_hex(0x222222),
};

// ---- Stroke management ----

static void add_pt(lv_coord_t x, lv_coord_t y)
{
    if (pt_n >= PT_MAX || st_n == 0) return;
    int si = st_n - 1;
    if (pt_n > st_start[si]) {
        lv_coord_t dx = x - pts[pt_n - 1].x;
        lv_coord_t dy = y - pts[pt_n - 1].y;
        if (dx * dx + dy * dy < MIN_DIST * MIN_DIST) return;
    }
    pts[pt_n].x = x;
    pts[pt_n].y = y;
    pt_n++;
    lv_line_set_points(st_line[si], &pts[st_start[si]], pt_n - st_start[si]);
}

static void end_stroke(void)
{
    if (st_n == 0) return;
    int si = st_n - 1;
    if (pt_n - st_start[si] <= 1) {
        lv_obj_del(st_line[si]);
        st_n--;
        pt_n = st_start[si];
    }
}

static void start_stroke(lv_coord_t x, lv_coord_t y)
{
    if (st_n >= ST_MAX) return;
    st_start[st_n] = pt_n;
    lv_obj_t *line = lv_line_create(canvas);
    lv_obj_set_style_line_color(line, palette[cur_col], 0);
    lv_obj_set_style_line_width(line, 4, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    st_line[st_n] = line;
    st_n++;
    add_pt(x, y);
}

// ---- Event handlers ----

static void on_canvas_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t p;
    lv_indev_get_point(lv_indev_active(), &p);
    lv_area_t ca;
    lv_obj_get_coords(canvas, &ca);
    p.x -= ca.x1;
    p.y -= ca.y1;
    if (code == LV_EVENT_PRESSED) {
        start_stroke(p.x, p.y);
    } else if (code == LV_EVENT_PRESSING) {
        add_pt(p.x, p.y);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        end_stroke();
    }
}

static void on_pal_tap(lv_event_t *e)
{
    cur_col = (int)(intptr_t)lv_event_get_user_data(e);
    for (int i = 0; i < PAL_SZ; i++) {
        lv_obj_set_style_outline_width(pal_btns[i], i == cur_col ? 3 : 0, 0);
    }
}

static void on_clear(lv_event_t *e)
{
    (void)e;
    for (int i = 0; i < st_n; i++) lv_obj_del(st_line[i]);
    st_n = 0;
    pt_n = 0;
}

static void on_undo(lv_event_t *e)
{
    (void)e;
    if (st_n == 0) return;
    st_n--;
    lv_obj_del(st_line[st_n]);
    pt_n = st_start[st_n];
}

// ---- Mode switching ----

static void set_mode(bool trace)
{
    tr_mode = trace;
    if (trace) {
        char buf[64];
        const char *txt = "Name";
        if (nino_settings_get_name(buf, sizeof(buf)) && buf[0]) txt = buf;
        guide = lv_label_create(canvas);
        lv_label_set_text(guide, txt);
        lv_obj_set_style_text_font(guide, &KG_font, 0);
        lv_obj_set_style_text_color(guide, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_align(guide, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_opa(guide, LV_OPA_50, 0);
        lv_obj_set_width(guide, 460);
        lv_obj_center(guide);
        lv_obj_clear_flag(guide, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(guide, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_move_foreground(guide);
    } else {
        if (guide) {
            lv_obj_del(guide);
            guide = NULL;
        }
    }
    for (int i = 0; i < 2; i++) {
        lv_obj_set_style_bg_opa(mode_btn[i], i == (trace ? 1 : 0) ? LV_OPA_COVER : LV_OPA_30, 0);
    }
}

static void on_mode_draw(lv_event_t *e)
{
    (void)e;
    if (tr_mode) set_mode(false);
}

static void on_mode_trace(lv_event_t *e)
{
    (void)e;
    if (!tr_mode) set_mode(true);
}

// ---- App entry point ----

void app_my_name_create(lv_obj_t *content)
{
    pt_n = 0;
    st_n = 0;
    cur_col = 0;
    tr_mode = true;
    guide = NULL;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_pad_row(content, 0, 0);

    // ---- Name row ----
    lv_obj_t *nr = lv_obj_create(content);
    lv_obj_remove_style_all(nr);
    lv_obj_set_size(nr, 480, 34);
    lv_obj_set_flex_flow(nr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(nr, 8, 0);
    lv_obj_clear_flag(nr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *intro = lv_label_create(nr);
    lv_label_set_text(intro, "My name is ");
    lv_obj_set_style_text_color(intro, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(intro, &lv_font_montserrat_16, 0);

    lv_obj_t *name_lab = lv_label_create(nr);
    char buf[64];
    if (nino_settings_get_name(buf, sizeof(buf)) && buf[0]) {
        lv_label_set_text(name_lab, buf);
    } else {
        lv_label_set_text(name_lab, "____");
    }
    lv_obj_set_style_text_color(name_lab, lv_color_hex(0x222222), 0);
    lv_obj_set_style_text_font(name_lab, &lv_font_montserrat_16, 0);

    // Spacer to push mode buttons right
    lv_obj_t *nr_spacer = lv_obj_create(nr);
    lv_obj_remove_style_all(nr_spacer);
    lv_obj_set_flex_grow(nr_spacer, 1);
    lv_obj_clear_flag(nr_spacer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(nr_spacer, LV_OBJ_FLAG_CLICKABLE);

    // Mode toggle pills (in name row, right side)
    const char *mode_labels[2] = {"Draw", "Trace"};
    for (int i = 0; i < 2; i++) {
        mode_btn[i] = lv_btn_create(nr);
        lv_obj_set_style_bg_color(mode_btn[i], lv_color_hex(0x3498DB), 0);
        lv_obj_set_style_bg_opa(mode_btn[i], i == 0 ? LV_OPA_COVER : LV_OPA_30, 0);
        lv_obj_set_size(mode_btn[i], 58, 28);
        lv_obj_set_style_radius(mode_btn[i], 14, 0);
        lv_obj_set_style_shadow_width(mode_btn[i], 0, 0);
        lv_obj_set_style_margin_right(mode_btn[i], 4, 0);
        lv_obj_add_event_cb(mode_btn[i], i == 0 ? on_mode_draw : on_mode_trace, LV_EVENT_CLICKED, NULL);
        lv_obj_t *ml = lv_label_create(mode_btn[i]);
        lv_label_set_text(ml, mode_labels[i]);
        lv_obj_set_style_text_color(ml, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(ml, &lv_font_montserrat_12, 0);
        lv_obj_center(ml);
    }

    // ---- Canvas area ----
    canvas = lv_obj_create(content);
    lv_obj_remove_style_all(canvas);
    lv_obj_set_style_bg_color(canvas, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(canvas, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_border_width(canvas, 1, 0);
    lv_obj_set_style_radius(canvas, 0, 0);
    lv_obj_set_width(canvas, 478);
    lv_obj_set_flex_grow(canvas, 1);
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(canvas, on_canvas_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(canvas, on_canvas_event, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(canvas, on_canvas_event, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(canvas, on_canvas_event, LV_EVENT_PRESS_LOST, NULL);

    // ---- Toolbar ----
    lv_obj_t *tb = lv_obj_create(content);
    lv_obj_remove_style_all(tb);
    lv_obj_set_size(tb, 480, 42);
    lv_obj_set_flex_flow(tb, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tb, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(tb, 4, 0);
    lv_obj_set_style_pad_right(tb, 4, 0);
    lv_obj_clear_flag(tb, LV_OBJ_FLAG_SCROLLABLE);

    // Palette circles
    int pal_sz = 26;
    for (int i = 0; i < PAL_SZ; i++) {
        lv_obj_t *c = lv_obj_create(tb);
        lv_obj_remove_style_all(c);
        lv_obj_set_style_bg_color(c, palette[i], 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(c, 0, 0);
        lv_obj_set_style_outline_color(c, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_outline_width(c, i == 0 ? 3 : 0, 0);
        lv_obj_set_style_outline_opa(c, LV_OPA_COVER, 0);
        lv_obj_set_size(c, pal_sz, pal_sz);
        lv_obj_set_style_margin_right(c, 4, 0);
        lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(c, on_pal_tap, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        pal_btns[i] = c;
    }

    lv_obj_t *spacer = lv_obj_create(tb);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_CLICKABLE);

    // Action buttons
    lv_obj_t *clear_btn = lv_btn_create(tb);
    lv_obj_set_style_bg_color(clear_btn, lv_color_hex(0xCC4444), 0);
    lv_obj_set_size(clear_btn, 52, 30);
    lv_obj_set_style_radius(clear_btn, 6, 0);
    lv_obj_set_style_shadow_width(clear_btn, 0, 0);
    lv_obj_set_style_margin_right(clear_btn, 4, 0);
    lv_obj_add_event_cb(clear_btn, on_clear, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cl = lv_label_create(clear_btn);
    lv_label_set_text(cl, "Clear");
    lv_obj_set_style_text_color(cl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(cl, &lv_font_montserrat_12, 0);
    lv_obj_center(cl);

    lv_obj_t *undo_btn = lv_btn_create(tb);
    lv_obj_set_style_bg_color(undo_btn, lv_color_hex(0x888888), 0);
    lv_obj_set_size(undo_btn, 52, 30);
    lv_obj_set_style_radius(undo_btn, 6, 0);
    lv_obj_set_style_shadow_width(undo_btn, 0, 0);
    lv_obj_add_event_cb(undo_btn, on_undo, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ul = lv_label_create(undo_btn);
    lv_label_set_text(ul, "Undo");
    lv_obj_set_style_text_color(ul, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ul, &lv_font_montserrat_12, 0);
    lv_obj_center(ul);

    set_mode(true);
}
