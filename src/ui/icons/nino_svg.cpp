#include "nino_svg.h"
#include <math.h>
#include <stdlib.h>

#define MAX_PTS 1024
#define CURVE_SEGS 8
#define ICON_SZ 50.0f

typedef struct { float x, y; } pt_t;

static pt_t pts[MAX_PTS];
static int pt_count;

static float cur_x, cur_y;
static float start_x, start_y;

static float parse_num(const char **sp)
{
    const char *s = *sp;
    while (*s == ' ' || *s == ',' || *s == '\t') s++;
    char *end;
    float v = strtof(s, &end);
    *sp = end;
    return v;
}

static void add_pt(float x, float y)
{
    if (pt_count < MAX_PTS) {
        pts[pt_count].x = x;
        pts[pt_count].y = y;
        pt_count++;
    }
}

static void tessellate_cubic(float x1, float y1, float x2, float y2, float x3, float y3, int segs)
{
    for (int i = 1; i <= segs; i++) {
        float t = (float)i / segs;
        float u = 1.0f - t;
        float x = u*u*u*cur_x + 3*u*u*t*x1 + 3*u*t*t*x2 + t*t*t*x3;
        float y = u*u*u*cur_y + 3*u*u*t*y1 + 3*u*t*t*y2 + t*t*t*y3;
        add_pt(x, y);
    }
    cur_x = x3; cur_y = y3;
}

static void tessellate_quad(float x1, float y1, float x2, float y2, int segs)
{
    for (int i = 1; i <= segs; i++) {
        float t = (float)i / segs;
        float u = 1.0f - t;
        float x = u*u*cur_x + 2*u*t*x1 + t*t*x2;
        float y = u*u*cur_y + 2*u*t*y1 + t*t*y2;
        add_pt(x, y);
    }
    cur_x = x2; cur_y = y2;
}

static float vec_angle(float ux, float uy)
{
    float a = atan2f(uy, ux);
    if (a < 0) a += 2.0f * (float)M_PI;
    return a;
}

static void tessellate_arc(float rx, float ry, float rot, int large, int sweep, float x2, float y2)
{
    // SVG arc to line segments via center parameterization
    float x1 = cur_x, y1 = cur_y;
    float dx2 = (x1 - x2) / 2.0f, dy2 = (y1 - y2) / 2.0f;

    float rad = rot * (float)M_PI / 180.0f;
    float c = cosf(rad), s = sinf(rad);
    float x1p = c * dx2 + s * dy2;
    float y1p = -s * dx2 + c * dy2;

    // Clamp radii
    float L = x1p*x1p/(rx*rx) + y1p*y1p/(ry*ry);
    if (L > 1.0f) { float sr = sqrtf(L); rx *= sr; ry *= sr; }

    float rx2 = rx*rx, ry2 = ry*ry;
    float x1p2 = x1p*x1p, y1p2 = y1p*y1p;
    float cp = sqrtf(fmaxf(0.0f, (rx2*ry2 - rx2*y1p2 - ry2*x1p2) / (rx2*y1p2 + ry2*x1p2)));
    if (large == sweep) cp = -cp;

    float cxp = cp * rx * y1p / ry;
    float cyp = -cp * ry * x1p / rx;

    float cx = c * cxp - s * cyp + (x1 + x2) / 2.0f;
    float cy = s * cxp + c * cyp + (y1 + y2) / 2.0f;

    float ux = (x1p - cxp) / rx, uy = (y1p - cyp) / ry;
    float vx = (-x1p - cxp) / rx, vy = (-y1p - cyp) / ry;
    float a1 = vec_angle(ux, uy);
    float da = vec_angle(ux*vx + uy*vy, ux*vy - uy*vx);
    if (sweep == 0 && da > 0) da -= 2.0f * (float)M_PI;
    if (sweep == 1 && da < 0) da += 2.0f * (float)M_PI;

    int segs = (int)(fabsf(da) / ((float)M_PI / 180.0f * 15.0f)) + 1;
    if (segs < 2) segs = 2;

    for (int i = 1; i <= segs; i++) {
        float t = a1 + da * (float)i / segs;
        float ex = cx + rx * cosf(rad) * cosf(t) - ry * sinf(rad) * sinf(t);
        float ey = cy + rx * sinf(rad) * cosf(t) + ry * cosf(rad) * sinf(t);
        add_pt(ex, ey);
    }
    cur_x = x2; cur_y = y2;
}

static void parse_svg(const char *d)
{
    cur_x = cur_y = start_x = start_y = 0;
    pt_count = 0;

    while (*d) {
        while (*d == ' ' || *d == ',' || *d == '\t') d++;
        if (!*d) break;

        char cmd = *d++;
        bool rel = (cmd >= 'a' && cmd <= 'z');
        char abs_cmd = rel ? (cmd - 'a' + 'A') : cmd;

        switch (abs_cmd) {
        case 'M': {
            float x = parse_num(&d);
            float y = parse_num(&d);
            if (rel) { x += cur_x; y += cur_y; }
            cur_x = start_x = x;
            cur_y = start_y = y;
            add_pt(x, y);
            break;
        }
        case 'L': {
            float x = parse_num(&d);
            float y = parse_num(&d);
            if (rel) { x += cur_x; y += cur_y; }
            add_pt(x, y);
            cur_x = x; cur_y = y;
            break;
        }
        case 'C': {
            float x1 = parse_num(&d), y1 = parse_num(&d);
            float x2 = parse_num(&d), y2 = parse_num(&d);
            float x = parse_num(&d), y = parse_num(&d);
            if (rel) { x1 += cur_x; y1 += cur_y; x2 += cur_x; y2 += cur_y; x += cur_x; y += cur_y; }
            tessellate_cubic(x1, y1, x2, y2, x, y, CURVE_SEGS);
            break;
        }
        case 'Q': {
            float x1 = parse_num(&d), y1 = parse_num(&d);
            float x = parse_num(&d), y = parse_num(&d);
            if (rel) { x1 += cur_x; y1 += cur_y; x += cur_x; y += cur_y; }
            tessellate_quad(x1, y1, x, y, CURVE_SEGS);
            break;
        }
        case 'A': {
            float rx = fabsf(parse_num(&d));
            float ry = fabsf(parse_num(&d));
            float rot = parse_num(&d);
            int large = (int)parse_num(&d);
            int sweep = (int)parse_num(&d);
            float x = parse_num(&d), y = parse_num(&d);
            if (rel) { x += cur_x; y += cur_y; }
            tessellate_arc(rx, ry, rot, large, sweep, x, y);
            break;
        }
        case 'Z': {
            add_pt(start_x, start_y);
            cur_x = start_x; cur_y = start_y;
            break;
        }
        default:
            break;
        }
    }
}

void nino_svg_draw(lv_obj_t *parent, const char *svg_path, lv_color_t ink, int width, float viewbox)
{
    parse_svg(svg_path);
    if (pt_count < 2) return;

    float scale = ICON_SZ / viewbox;

    int n = pt_count;
    lv_point_precise_t *lpts = (lv_point_precise_t *)lv_malloc(sizeof(lv_point_precise_t) * n);
    if (!lpts) return;

    for (int i = 0; i < n; i++) {
        lpts[i].x = (lv_value_precise_t)(pts[i].x * scale);
        lpts[i].y = (lv_value_precise_t)(pts[i].y * scale);
    }

    lv_obj_t *ln = lv_line_create(parent);
    lv_line_set_points(ln, lpts, n);
    lv_obj_set_style_line_color(ln, ink, 0);
    lv_obj_set_style_line_width(ln, width, 0);
    lv_obj_set_style_line_rounded(ln, true, 0);
    lv_obj_set_style_opa(ln, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ln, LV_OBJ_FLAG_CLICKABLE);
    lv_free(lpts);
}
