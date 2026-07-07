#include "app_shape_paint.h"
#include "nino_colors.h"
#include <Arduino.h>

#define MAX_SHAPES   15
#define TYPES         5
#define COLORS        5
#define T_CIRCLE      0
#define T_SQUARE      1
#define T_TRIANGLE    2
#define T_OVAL        3
#define T_RECT        4
#define SHAPE_AREA_X  10
#define SHAPE_AREA_Y  58
#define SHAPE_AREA_W  444
#define SHAPE_AREA_H  172

static const int shape_w[] = {42, 42, 48, 50, 34};
static const int shape_h[] = {42, 42, 48, 28, 46};
static const char *type_names[] = {"CIRCLES", "SQUARES", "TRIANGLES", "OVALS", "RECTANGLES"};
static const char *type_singular[] = {"CIRCLE", "SQUARE", "TRIANGLE", "OVAL", "RECTANGLE"};

static const lv_color_t palette[] = {
    lv_color_hex(0xE74C3C), lv_color_hex(0x3498DB),
    lv_color_hex(0xF1C40F), lv_color_hex(0x2ECC71),
    lv_color_hex(0xE67E22),
};
static const char *color_names[] = {"RED", "BLUE", "YELLOW", "GREEN", "ORANGE"};

static const lv_point_precise_t tri_pts[] = {{24, 4}, {4, 45}, {44, 45}, {24, 4}};

// ---- State ----
static lv_obj_t *parent_content;
static int qn;
static int shape_count;
static int type_count;
static int active_types[TYPES];
static int shape_types[MAX_SHAPES];
static int shape_x[MAX_SHAPES];
static int shape_y[MAX_SHAPES];
static int round_order[TYPES];
static lv_color_t round_color[TYPES];
static int painted_cnt;
static int painted_total[TYPES];

struct Shape {
    lv_obj_t *obj;
    lv_obj_t *outline;
    lv_obj_t *fill_line;
    int type;
    int orig_x;
    bool painted;
};
static Shape shapes[MAX_SHAPES];

static lv_obj_t *prompt_lab;
static lv_obj_t *color_lab;
static lv_obj_t *prog_lab;
static lv_obj_t *fb_lab;
static lv_obj_t *overlay;
static lv_timer_t *adv_tmr;
static lv_timer_t *rst_tmr;
static int rst_idx;
static bool locked;

// ---- Helpers ----

static void shuf(int arr[], int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = random(i + 1);
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}

static bool overlaps(int x, int y, int w, int h, int cnt)
{
    for (int i = 0; i < cnt; i++) {
        int t = shape_types[i];
        int ow = shape_w[t], oh = shape_h[t];
        if (abs(x - shape_x[i]) < (w + ow) / 2 + 6 &&
            abs(y - shape_y[i]) < (h + oh) / 2 + 6)
            return true;
    }
    return false;
}

static lv_color_t darker(lv_color_t c)
{
    return lv_color_darken(c, LV_OPA_30);
}

// ---- Level config ----

static void level_active_types(int lvl)
{
    switch (lvl) {
    case 1: active_types[0] = T_CIRCLE; active_types[1] = T_SQUARE; type_count = 2; break;
    case 2: active_types[0] = T_CIRCLE; active_types[1] = T_SQUARE;
            active_types[2] = T_TRIANGLE; type_count = 3; break;
    default:
            active_types[0] = T_CIRCLE; active_types[1] = T_SQUARE;
            active_types[2] = T_TRIANGLE; active_types[3] = T_OVAL;
            active_types[4] = T_RECT; type_count = 5; break;
    }
}

// ---- Shape generation ----

static void generate_shapes(void)
{
    int types[MAX_SHAPES], idx = 0;
    int per = MAX_SHAPES / type_count;
    int rem = MAX_SHAPES % type_count;
    for (int i = 0; i < type_count; i++) {
        int n = per + (i < rem ? 1 : 0);
        for (int j = 0; j < n; j++)
            types[idx++] = active_types[i];
    }
    shape_count = idx;
    shuf(types, shape_count);

    for (int i = 0; i < shape_count; i++) {
        int t = types[i];
        int sw = shape_w[t], sh = shape_h[t];
        int x, y, tries = 0;
        do {
            x = SHAPE_AREA_X + random(SHAPE_AREA_W - sw);
            y = SHAPE_AREA_Y + random(SHAPE_AREA_H - sh);
            tries++;
        } while (tries < 300 && overlaps(x, y, sw, sh, i));
        shape_types[i] = t;
        shape_x[i] = x;
        shape_y[i] = y;
    }
}

// ---- Animations ----

static void paint_anim_shape(lv_obj_t *obj, lv_color_t color)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_border_color(obj, darker(color), 0);

    lv_obj_set_style_transform_scale(obj, 250, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        lv_obj_set_style_transform_scale((lv_obj_t *)var, v, 0);
    });
    lv_anim_set_values(&a, 250, 256);
    lv_anim_set_time(&a, 200);
    lv_anim_set_completed_cb(&a, [](lv_anim_t *a) {
        lv_obj_set_style_transform_scale((lv_obj_t *)a->var, 256, 0);
    });
    lv_anim_start(&a);
}

static void paint_triangle(Shape *s, lv_color_t color)
{
    lv_obj_set_style_line_color(s->outline, darker(color), 0);
    lv_obj_set_style_line_color(s->fill_line, color, 0);
    lv_obj_set_style_line_width(s->fill_line, 0, 0);
    lv_obj_clear_flag(s->fill_line, LV_OBJ_FLAG_HIDDEN);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s->fill_line);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        lv_obj_set_style_line_width((lv_obj_t *)var, v, 0);
    });
    lv_anim_set_values(&a, 0, 30);
    lv_anim_set_time(&a, 200);
    lv_anim_start(&a);
}

static void wiggle_obj(lv_obj_t *obj, int orig_x)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        lv_obj_set_x((lv_obj_t *)var, v);
    });
    lv_anim_set_values(&a, orig_x - 6, orig_x + 6);
    lv_anim_set_time(&a, 40);
    lv_anim_set_reverse_duration(&a, 40);
    lv_anim_set_repeat_count(&a, 2);
    lv_anim_set_user_data(&a, (void *)(intptr_t)orig_x);
    lv_anim_set_completed_cb(&a, [](lv_anim_t *a) {
        lv_obj_set_x((lv_obj_t *)a->var, (intptr_t)lv_anim_get_user_data(a));
    });
    lv_anim_start(&a);
}

// ---- Shape actions ----

static void flash_wrong(Shape *s)
{
    if (s->type == T_TRIANGLE)
        lv_obj_set_style_line_color(s->outline, lv_color_hex(0xE74C3C), 0);
    else
        lv_obj_set_style_border_color(s->obj, lv_color_hex(0xE74C3C), 0);
    wiggle_obj(s->obj, s->orig_x);
}

static void restore_shape(Shape *s)
{
    if (s->painted) return;
    if (s->type == T_TRIANGLE)
        lv_obj_set_style_line_color(s->outline, lv_color_hex(0x999999), 0);
    else
        lv_obj_set_style_border_color(s->obj, lv_color_hex(0x999999), 0);
}

static void paint_shape(Shape *s, lv_color_t color)
{
    s->painted = true;
    if (s->type == T_TRIANGLE)
        paint_triangle(s, color);
    else if (s->type == T_OVAL) {
        lv_obj_set_style_bg_opa(s->obj, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(s->obj, color, 0);
        lv_obj_set_style_border_color(s->obj, darker(color), 0);
    } else
        paint_anim_shape(s->obj, color);
}

// ---- Game flow forward decl ----
static void next_q(void);

// ---- Shape creation ----

static void create_shape(int i, lv_obj_t *parent)
{
    int t = shape_types[i];
    int sw = shape_w[t], sh = shape_h[t];
    Shape *s = &shapes[i];
    s->type = t;
    s->painted = false;
    s->outline = NULL;
    s->fill_line = NULL;

    if (t == T_OVAL) {
        sw = 28;
        sh = 28;
    }

    s->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(s->obj);
    lv_obj_set_size(s->obj, sw, sh);
    s->orig_x = shape_x[i];
    lv_obj_clear_flag(s->obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s->obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(s->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s->obj, 0, 0);

    if (t == T_OVAL) {
        int ox = shape_x[i] + (shape_w[T_OVAL] - sw) / 2;
        lv_obj_set_pos(s->obj, ox, shape_y[i]);
        s->orig_x = ox;
        lv_obj_set_style_radius(s->obj, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s->obj, 3, 0);
        lv_obj_set_style_border_color(s->obj, lv_color_hex(0x999999), 0);
        lv_obj_set_style_transform_scale_x(s->obj, 456, 0);
    } else if (t == T_TRIANGLE) {
        lv_obj_set_pos(s->obj, shape_x[i], shape_y[i]);
        lv_obj_set_style_border_width(s->obj, 0, 0);

        s->fill_line = lv_line_create(s->obj);
        lv_obj_set_style_line_width(s->fill_line, 30, 0);
        lv_obj_set_style_line_color(s->fill_line, lv_color_hex(0xFFD700), 0);
        lv_obj_set_style_line_rounded(s->fill_line, false, 0);
        lv_obj_add_flag(s->fill_line, LV_OBJ_FLAG_HIDDEN);
        lv_line_set_points(s->fill_line, tri_pts, 4);

        s->outline = lv_line_create(s->obj);
        lv_obj_set_style_line_width(s->outline, 4, 0);
        lv_obj_set_style_line_color(s->outline, lv_color_hex(0x999999), 0);
        lv_obj_set_style_line_rounded(s->outline, true, 0);
        lv_line_set_points(s->outline, tri_pts, 4);
    } else {
        lv_obj_set_pos(s->obj, shape_x[i], shape_y[i]);
        lv_obj_set_style_border_width(s->obj, 3, 0);
        lv_obj_set_style_border_color(s->obj, lv_color_hex(0x999999), 0);
        if (t == T_CIRCLE)
            lv_obj_set_style_radius(s->obj, LV_RADIUS_CIRCLE, 0);
        else
            lv_obj_set_style_radius(s->obj, 5, 0);
    }

    lv_obj_add_event_cb(s->obj, [](lv_event_t *e) {
        if (locked) return;
        int idx = (int)(intptr_t)lv_event_get_user_data(e);
        Shape *s2 = &shapes[idx];
        if (s2->painted) return;

        if (rst_tmr) {
            lv_timer_del(rst_tmr);
            rst_tmr = NULL;
            restore_shape(&shapes[rst_idx]);
            rst_idx = -1;
            lv_label_set_text(fb_lab, "");
        }

        int tt = round_order[qn];
        if (shape_types[idx] == tt) {
            paint_shape(s2, round_color[qn]);
            painted_cnt++;
            painted_total[tt]++;

            int total = 0;
            for (int j = 0; j < shape_count; j++)
                if (shape_types[j] == tt) total++;

            char buf[24];
            snprintf(buf, sizeof(buf), "%s %d / %d", LV_SYMBOL_OK, painted_cnt, total);
            lv_label_set_text(fb_lab, buf);
            lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x2ECC71), 0);
            lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_16, 0);

            if (painted_cnt >= total) {
                locked = true;
                lv_label_set_text(fb_lab, "Great!");
                lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x2ECC71), 0);
                lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_20, 0);

                adv_tmr = lv_timer_create([](lv_timer_t *tm) {
                    lv_timer_del(tm);
                    adv_tmr = NULL;
                    next_q();
                }, 1200, NULL);
                lv_timer_set_repeat_count(adv_tmr, 1);
            }
        } else {
            rst_idx = idx;
            flash_wrong(s2);
            lv_label_set_text(fb_lab, "Try Again");
            lv_obj_set_style_text_color(fb_lab, lv_color_hex(0xE74C3C), 0);
            lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_16, 0);

            rst_tmr = lv_timer_create([](lv_timer_t *tm) {
                lv_timer_del(tm);
                rst_tmr = NULL;
                restore_shape(&shapes[rst_idx]);
                rst_idx = -1;
                lv_label_set_text(fb_lab, "");
            }, 500, NULL);
            lv_timer_set_repeat_count(rst_tmr, 1);
        }
    }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
}

// ---- Clear ----

static void clear_shapes(void)
{
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].obj) {
            lv_obj_del(shapes[i].obj);
            shapes[i].obj = NULL;
            shapes[i].outline = NULL;
            shapes[i].fill_line = NULL;
        }
    }
    shape_count = 0;
}

// ---- Game flow ----

static void start_round(void)
{
    if (overlay) {
        lv_obj_del(overlay);
        overlay = NULL;
    }

    if (rst_tmr) { lv_timer_del(rst_tmr); rst_tmr = NULL; rst_idx = -1; }
    locked = false;

    int ttype = round_order[qn];
    lv_color_t color = round_color[qn];
    int ci = -1;
    for (int i = 0; i < COLORS; i++)
        if (lv_color_eq(color, palette[i])) { ci = i; break; }

    painted_cnt = 0;

    char buf[48];
    snprintf(buf, sizeof(buf), "Paint all the %s", type_names[ttype]);
    lv_label_set_text(prompt_lab, buf);

    if (ci >= 0) {
        lv_label_set_text(color_lab, color_names[ci]);
        lv_obj_set_style_text_color(color_lab, color, 0);
    } else {
        lv_label_set_text(color_lab, "");
    }

    snprintf(buf, sizeof(buf), "%d/%d", qn + 1, type_count);
    lv_label_set_text(prog_lab, buf);

    lv_label_set_text(fb_lab, "");
}

static void show_summary(void)
{
    overlay = lv_obj_create(parent_content);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, 480, 276);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0xF0FFF0), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ml = lv_label_create(overlay);
    lv_label_set_text(ml, "Great Painting!\nYou painted all the shapes!");
    lv_obj_set_style_text_font(ml, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(ml, lv_color_hex(0x27AE60), 0);
    lv_obj_set_style_text_align(ml, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ml, LV_ALIGN_TOP_MID, 0, 16);

    char summary[256];
    int pos = 0;
    for (int i = 0; i < type_count; i++) {
        int t = active_types[i];
        pos += snprintf(summary + pos, sizeof(summary) - pos,
                        "%d %s  ", painted_total[t], type_singular[t]);
        if (pos >= (int)sizeof(summary)) break;
    }
    lv_obj_t *sl = lv_label_create(overlay);
    lv_label_set_text(sl, summary);
    lv_obj_set_style_text_font(sl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sl, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_align(sl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(sl, LV_ALIGN_TOP_MID, 0, 80);

    lv_obj_t *pb = lv_btn_create(overlay);
    lv_obj_set_style_bg_color(pb, lv_color_hex(0x27AE60), 0);
    lv_obj_set_size(pb, 160, 52);
    lv_obj_set_style_radius(pb, 26, 0);
    lv_obj_set_style_shadow_width(pb, 0, 0);
    lv_obj_align(pb, LV_ALIGN_TOP_MID, 0, 140);

    lv_obj_add_event_cb(pb, [](lv_event_t *) {
        lv_obj_del(overlay);
        overlay = NULL;
        clear_shapes();
        for (int i = 0; i < TYPES; i++) painted_total[i] = 0;
        generate_shapes();
        for (int i = 0; i < shape_count; i++)
            create_shape(i, parent_content);
        for (int i = 0; i < type_count; i++) {
            round_order[i] = i;
            round_color[i] = palette[i];
        }
        shuf(round_order, type_count);
        qn = 0;
        start_round();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *pl = lv_label_create(pb);
    lv_label_set_text(pl, "Play Again");
    lv_obj_set_style_text_color(pl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(pl, &lv_font_montserrat_20, 0);
    lv_obj_center(pl);
}

static void next_q(void)
{
    qn++;
    if (qn >= type_count) {
        show_summary();
        return;
    }
    start_round();
}

// ---- App entry ----

void app_shape_paint_create(lv_obj_t *content)
{
    parent_content = content;
    qn = 0;
    locked = false;
    overlay = NULL;
    adv_tmr = NULL;
    rst_tmr = NULL;
    rst_idx = -1;
    shape_count = 0;
    for (int i = 0; i < TYPES; i++) painted_total[i] = 0;
    for (int i = 0; i < MAX_SHAPES; i++) shapes[i].obj = NULL;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(content, [](lv_event_t *) {
        if (adv_tmr) { lv_timer_del(adv_tmr); adv_tmr = NULL; }
        if (rst_tmr) { lv_timer_del(rst_tmr); rst_tmr = NULL; }
    }, LV_EVENT_DELETE, NULL);

    // ---- Prompt ----
    prompt_lab = lv_label_create(content);
    lv_label_set_text(prompt_lab, "Paint all the ...");
    lv_obj_set_style_text_font(prompt_lab, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(prompt_lab, lv_color_hex(0x444444), 0);
    lv_obj_align(prompt_lab, LV_ALIGN_TOP_LEFT, 0, 6);

    color_lab = lv_label_create(content);
    lv_label_set_text(color_lab, "");
    lv_obj_set_style_text_font(color_lab, &lv_font_montserrat_28, 0);
    lv_obj_align(color_lab, LV_ALIGN_TOP_LEFT, 0, 32);

    // ---- Progress ----
    prog_lab = lv_label_create(content);
    lv_label_set_text(prog_lab, "1/5");
    lv_obj_set_style_text_font(prog_lab, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(prog_lab, lv_color_hex(0x888888), 0);
    lv_obj_align(prog_lab, LV_ALIGN_TOP_RIGHT, -8, 10);

    // ---- Feedback ----
    fb_lab = lv_label_create(content);
    lv_label_set_text(fb_lab, "");
    lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_16, 0);
    lv_obj_align(fb_lab, LV_ALIGN_TOP_MID, 0, 236);

    // ---- Setup ----
    level_active_types(3);
    for (int i = 0; i < type_count; i++) {
        round_order[i] = i;
        round_color[i] = palette[i];
    }
    shuf(round_order, type_count);

    generate_shapes();
    for (int i = 0; i < shape_count; i++)
        create_shape(i, content);

    start_round();
}
