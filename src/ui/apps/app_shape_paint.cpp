#include "app_shape_paint.h"
#include "nino_colors.h"
#include "utils/anim_utils.h"
#include "scr_congrats.h"
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
    int type;
    int orig_x;
    bool painted;
    lv_color_t paint_color;
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

// ---- Custom shape drawing (scanline fill + midpoint ellipse) ----

// Interpolate x along a line segment from (ax,ay) to (bx,by) at y-coordinate cy
static int32_t interp_x(int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t cy)
{
    if (by == ay) return ax;
    return ax + (bx - ax) * (cy - ay) / (by - ay);
}

// Draw a single horizontal line on the layer using lv_draw_rect
static void draw_hline(lv_layer_t *layer, int32_t x1, int32_t x2, int32_t y,
                        lv_color_t color, int32_t ox, int32_t oy)
{
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa = LV_OPA_COVER;

    lv_area_t a;
    a.x1 = x1 + ox;
    a.y1 = y + oy;
    a.x2 = x2 + ox;
    a.y2 = y + oy;
    lv_draw_rect(layer, &dsc, &a);
}

// Draw filled triangle using scanline fill algorithm
static void draw_triangle_filled(lv_layer_t *layer, int32_t w, int32_t h,
                                  lv_color_t color, int32_t ox, int32_t oy)
{
    int32_t x0 = w / 2,  y0 = h / 10;
    int32_t x1 = 2,      y1 = h - 3;
    int32_t x2 = w - 3,  y2 = h - 3;

    for (int32_t y = y0; y <= y2; y++) {
        int32_t xa, xb;
        if (y <= y1)
            xa = interp_x(x0, y0, x1, y1, y);
        else
            xa = x1;
        if (y <= y2)
            xb = interp_x(x0, y0, x2, y2, y);
        else
            xb = x2;
        if (xa > xb) { int32_t t = xa; xa = xb; xb = t; }
        if (xa < 0) xa = 0;
        if (xb >= w) xb = w - 1;
        draw_hline(layer, xa, xb, y, color, ox, oy);
    }
}

// Draw filled ellipse using scanline fill
static void draw_ellipse_filled(lv_layer_t *layer, int32_t cx, int32_t cy,
                                 int32_t rx, int32_t ry, lv_color_t color,
                                 int32_t ox, int32_t oy)
{
    if (rx <= 0 || ry <= 0) return;
    int32_t ry2 = ry * ry;
    int32_t rx2 = rx * rx;
    for (int32_t y = -ry; y <= ry; y++) {
        int32_t dy2 = y * y;
        int32_t x_sq = (rx2 * (ry2 - dy2) + ry2 / 2) / ry2;
        int32_t x = 0;
        while (x * x < x_sq && x <= rx) x++;
        draw_hline(layer, cx - x, cx + x, cy + y, color, ox, oy);
    }
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

// ---- Shape actions ----

static void flash_wrong(Shape *s)
{
    lv_obj_set_style_bg_color(s->obj, NINO_COLOR_DANGER, 0);
    nino_anim_shake(s->obj);
}

static void restore_shape(Shape *s)
{
    if (s->painted) return;
    lv_obj_set_style_bg_color(s->obj, lv_color_hex(0xDDDDDD), 0);
}

static void paint_shape(Shape *s, lv_color_t color)
{
    s->painted = true;
    s->paint_color = color;
    if (s->type == T_TRIANGLE || s->type == T_OVAL) {
        // Custom-drawn shapes: invalidate to trigger redraw in filled state
        lv_obj_invalidate(s->obj);
    } else {
        paint_anim_shape(s->obj, color);
    }
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
    s->paint_color = lv_color_hex(0x999999);

    s->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(s->obj);
    lv_obj_set_size(s->obj, sw, sh);
    s->orig_x = shape_x[i];
    lv_obj_clear_flag(s->obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s->obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(s->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s->obj, 0, 0);

    if (t == T_OVAL) {
        // True ellipse drawn via custom draw event
        lv_obj_set_pos(s->obj, shape_x[i], shape_y[i]);

        lv_obj_add_event_cb(s->obj, [](lv_event_t *e) {
            lv_layer_t *layer = lv_event_get_layer(e);
            int idx = (int)(intptr_t)lv_event_get_user_data(e);
            Shape *sh = &shapes[idx];
            lv_obj_t *obj = sh->obj;
            int32_t w = lv_obj_get_width(obj);
            int32_t h = lv_obj_get_height(obj);

            lv_area_t coords;
            lv_obj_get_coords(obj, &coords);
            int32_t ox = coords.x1;
            int32_t oy = coords.y1;

            if (sh->painted) {
                draw_ellipse_filled(layer, w / 2, h / 2, w / 2 - 2, h / 2 - 2, sh->paint_color, ox, oy);
            } else {
                draw_ellipse_filled(layer, w / 2, h / 2, w / 2 - 2, h / 2 - 2, lv_color_hex(0xDDDDDD), ox, oy);
            }
        }, LV_EVENT_DRAW_MAIN, (void *)(intptr_t)i);

    } else if (t == T_TRIANGLE) {
        // True triangle drawn via custom draw event
        lv_obj_set_pos(s->obj, shape_x[i], shape_y[i]);

        lv_obj_add_event_cb(s->obj, [](lv_event_t *e) {
            lv_layer_t *layer = lv_event_get_layer(e);
            int idx = (int)(intptr_t)lv_event_get_user_data(e);
            Shape *sh = &shapes[idx];
            lv_obj_t *obj = sh->obj;
            int32_t w = lv_obj_get_width(obj);
            int32_t h = lv_obj_get_height(obj);

            lv_area_t coords;
            lv_obj_get_coords(obj, &coords);
            int32_t ox = coords.x1;
            int32_t oy = coords.y1;

            if (sh->painted) {
                draw_triangle_filled(layer, w, h, sh->paint_color, ox, oy);
            } else {
                draw_triangle_filled(layer, w, h, lv_color_hex(0xDDDDDD), ox, oy);
            }
        }, LV_EVENT_DRAW_MAIN, (void *)(intptr_t)i);

    } else {
        // Circle, square, rectangle: filled gray initially
        lv_obj_set_pos(s->obj, shape_x[i], shape_y[i]);
        lv_obj_set_style_bg_opa(s->obj, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(s->obj, lv_color_hex(0xDDDDDD), 0);
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
            lv_obj_set_style_text_color(fb_lab, NINO_COLOR_DANGER, 0);
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
    char subtitle[256];
    int pos = 0;
    for (int i = 0; i < type_count; i++) {
        int t = active_types[i];
        pos += snprintf(subtitle + pos, sizeof(subtitle) - pos,
                        "%d %s  ", painted_total[t], type_singular[t]);
        if (pos >= (int)sizeof(subtitle)) break;
    }

    overlay = nino_congrats_create(parent_content,
        "Great Painting!\nYou painted all the shapes!", 0, subtitle,
        NINO_COLOR_SUCCESS, 160, [](lv_event_t *) {
            lv_obj_del(overlay); overlay = NULL;
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
        });
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
