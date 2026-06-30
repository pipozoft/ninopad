/**
 * nino_icons.cpp — simple recognizable line-art icons via lv_line primitives.
 * Coordinates assume a 50x50 parent (canvas-anchored into a flex column btn).
 */
#include "nino_icons.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const int ICON_BOX = 50;

// Add one straight line to parent.
static lv_obj_t *add_line(lv_obj_t *parent, const lv_point_precise_t *pts, int cnt,
                          lv_color_t color, int width)
{
    lv_obj_t *ln = lv_line_create(parent);
    lv_line_set_points(ln, pts, cnt);
    lv_obj_set_style_line_color(ln, color, 0);
    lv_obj_set_style_line_width(ln, width, 0);
    lv_obj_set_style_line_rounded(ln, true, 0);
    lv_obj_set_style_opa(ln, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ln, LV_OBJ_FLAG_CLICKABLE);
    return ln;
}

// Build an arc as N short straight segments into parent.
static void add_arc(lv_obj_t *parent, float cx, float cy, float r,
                    float a0_deg, float a1_deg, int segs,
                    lv_color_t color, int width)
{
    if (segs < 1) segs = 1;
    lv_point_precise_t *pts = (lv_point_precise_t *)lv_malloc(sizeof(lv_point_precise_t) * (segs + 1));
    if (!pts) return;
    for (int i = 0; i <= segs; ++i) {
        float t = a0_deg + (a1_deg - a0_deg) * i / segs;
        float rad = t * (float)M_PI / 180.0f;
        pts[i].x = (lv_value_precise_t)(cx + r * cosf(rad));
        pts[i].y = (lv_value_precise_t)(cy + r * sinf(rad));
    }
    add_line(parent, pts, segs + 1, color, width);
    lv_free(pts);
}

// Smiley: circle face, two eyes, smiling arc.
void nino_icon_smiley(lv_obj_t *p, lv_color_t ink)
{
    (void)ICON_BOX;
    add_arc(p, 25, 25, 18, 0, 360, 40, ink, 2);
    lv_point_precise_t eye_l[2] = {{18, 16}, {22, 16}};
    lv_point_precise_t eye_r[2] = {{28, 16}, {32, 16}};
    add_line(p, eye_l, 2, ink, 2);
    add_line(p, eye_r, 2, ink, 2);
    add_arc(p, 25, 22, 10, 30, 150, 12, ink, 2);
}

// Flame: stylized teardrop made of arcs.
void nino_icon_flame(lv_obj_t *p, lv_color_t ink)
{
    lv_point_precise_t outer[8] = {
        {25, 8}, {35, 25}, {33, 33}, {33, 40},
        {25, 44}, {17, 40}, {17, 33}, {15, 25}
    };
    add_line(p, outer, 8, ink, 2);
    add_arc(p, 25, 30, 8, 30, 150, 10, ink, 2);
}

// Eye: almond + pupil.
void nino_icon_eye(lv_obj_t *p, lv_color_t ink)
{
    add_arc(p, 25, 24, 16, -20, 200, 16, ink, 2);
    add_arc(p, 25, 24, 14, 200, -20, 16, ink, 2);
    add_arc(p, 25, 24, 4, 0, 360, 16, ink, 2);
}

// Money jar: trapezoid jar outline.
void nino_icon_money(lv_obj_t *p, lv_color_t ink)
{
    lv_point_precise_t lip[5] = {{17, 12}, {33, 12}, {33, 16}, {17, 16}, {17, 12}};
    add_line(p, lip, 5, ink, 2);
    lv_point_precise_t jar[5] = {
        {19, 16}, {31, 16}, {33, 40}, {17, 40}, {19, 16}
    };
    add_line(p, jar, 5, ink, 2);
    // $ icon as two arcs + line (approx).
    add_arc(p, 25, 28, 6, 70, 290, 10, ink, 2);
    lv_point_precise_t s_stem[2] = {{25, 22}, {25, 34}};
    add_line(p, s_stem, 2, ink, 2);
}

// Sun: center circle + 8 spokes.
void nino_icon_sun(lv_obj_t *p, lv_color_t ink)
{
    add_arc(p, 25, 25, 8, 0, 360, 24, ink, 2);
    for (int i = 0; i < 8; ++i) {
        float a = i * 45.0f * (float)M_PI / 180.0f;
        lv_point_precise_t sp[2];
        sp[0].x = (int)(25 + 12 * cosf(a));
        sp[0].y = (int)(25 + 12 * sinf(a));
        sp[1].x = (int)(25 + 18 * cosf(a));
        sp[1].y = (int)(25 + 18 * sinf(a));
        add_line(p, sp, 2, ink, 2);
    }
}

// Paintbrush: diagonal handle + tip.
void nino_icon_brush(lv_obj_t *p, lv_color_t ink)
{
    lv_point_precise_t handle[2] = {{15, 35}, {33, 17}};
    add_line(p, handle, 2, ink, 3);
    lv_point_precise_t tip[4] = {{12, 38}, {16, 38}, {17, 33}, {13, 33}};
    add_line(p, tip, 4, ink, 2);
}

// Scissors: two finger loops + crossing blades.
void nino_icon_scissors(lv_obj_t *p, lv_color_t ink)
{
    add_arc(p, 14, 16, 6, 0, 360, 16, ink, 2);
    add_arc(p, 36, 16, 6, 0, 360, 16, ink, 2);
    lv_point_precise_t blade_a[2] = {{14, 22}, {38, 36}};
    lv_point_precise_t blade_b[2] = {{36, 22}, {12, 36}};
    add_line(p, blade_a, 2, ink, 2);
    add_line(p, blade_b, 2, ink, 2);
}

// Book: rectangle outline + spine.
void nino_icon_book(lv_obj_t *p, lv_color_t ink)
{
    lv_point_precise_t rect[5] = {
        {14, 12}, {36, 12}, {36, 40}, {14, 40}, {14, 12}
    };
    add_line(p, rect, 5, ink, 2);
    lv_point_precise_t spine[2] = {{25, 12}, {25, 40}};
    add_line(p, spine, 2, ink, 2);
}

// Gear: center circle with 8 teeth as short lines around the rim.
void nino_icon_gear(lv_obj_t *p, lv_color_t ink)
{
    add_arc(p, 25, 25, 9, 0, 360, 28, ink, 2);
    add_arc(p, 25, 25, 3, 0, 360, 16, ink, 2);
    for (int i = 0; i < 8; ++i) {
        float a = (i * 45.0f + 22.5f) * (float)M_PI / 180.0f;
        lv_point_precise_t t[2];
        t[0].x = (int)(25 + 9 * cosf(a));
        t[0].y = (int)(25 + 9 * sinf(a));
        t[1].x = (int)(25 + 14 * cosf(a));
        t[1].y = (int)(25 + 14 * sinf(a));
        add_line(p, t, 2, ink, 3);
    }
}

// Status: wifi — three arcs + dot.
void nino_icon_wifi(lv_obj_t *p, lv_color_t ink)
{
    (void)ICON_BOX;
    add_arc(p, 14, 14, 9, 200, 340, 10, ink, 1);
    add_arc(p, 14, 14, 6, 200, 340, 10, ink, 1);
    add_arc(p, 14, 14, 3, 200, 340, 8, ink, 1);
    lv_point_precise_t dot[2] = {{14, 14}, {14, 14}};
    add_line(p, dot, 1, ink, 2);
}

// Status: battery — body rect + terminal nub.
void nino_icon_battery(lv_obj_t *p, lv_color_t ink)
{
    (void)ICON_BOX;
    lv_point_precise_t body[5] = {
        {4, 6}, {20, 6}, {20, 22}, {4, 22}, {4, 6}
    };
    add_line(p, body, 5, ink, 1);
    lv_point_precise_t nub[2] = {{22, 11}, {22, 17}};
    add_line(p, nub, 2, ink, 1);
    // inner fill bar
    lv_point_precise_t fill[5] = {{6, 9}, {6, 19}, {13, 19}, {13, 9}, {6, 9}};
    add_line(p, fill, 5, ink, 1);
}