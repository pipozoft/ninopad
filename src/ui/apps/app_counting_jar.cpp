#include "app_counting_jar.h"
#include "nino_colors.h"
#include "scr_congrats.h"
#include <Arduino.h>

#define TQ 10

struct MathRound {
    int a, b;
    int answer;
    int wrong;
};

// Comparison modes: groups of shapes, up to SHPE per group
#define SHPE 8
#define CELL 42
#define CGAP 4
#define COLS 4
#define GGW  (COLS * CELL + (COLS - 1) * CGAP)  // 180
#define GGH  (2 * CELL + CGAP)                  // 88
#define GSGAP 24
#define GOX  ((464 - 2 * GGW - GSGAP) / 2)       // 40
#define GOY  96

static int g_mode;
static MathRound g_rounds[TQ];
static int g_qn;
static int g_correct_btn;
static int g_correct_count;
static bool g_locked;
static lv_obj_t *g_eq_lab;
static lv_obj_t *g_btns[2];
static lv_obj_t *g_blabs[2];
static lv_obj_t *g_prog_lab;
static lv_obj_t *g_fb_lab;
static lv_obj_t *g_overlay;
static lv_timer_t *g_timer;

static lv_obj_t *g_shapes[2][SHPE];
static int g_lcnt, g_rcnt;
static int g_lshape, g_rshape;          // 0 = circle, 1 = square
static lv_color_t g_lcol, g_rcol;

static void cancel_timer(void)
{
    if (g_timer) { lv_timer_del(g_timer); g_timer = nullptr; }
}

static void shuf(int arr[], int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = random(i + 1);
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}

static void generate_rounds(void)
{
    for (int i = 0; i < TQ; i++) {
        if (g_mode == 0) {
            int target = random(0, 2) ? 8 : 9;
            int a = random(1, target);
            g_rounds[i] = {a, target - a, target, 0};
            int w = (random(0, 2) ? target + 1 : target - 1);
            if (w == target || w < 1) w = target + 1;
            if (w > 12) w = target - 1;
            g_rounds[i].wrong = w;
        } else if (g_mode == 1) {
            int target = random(0, 2) ? 3 : 4;
            int m = random(target + 1, target + 6);
            g_rounds[i] = {m, m - target, target, 0};
            int w = (random(0, 2) ? target + 1 : target - 1);
            if (w == target || w < 1) w = target + 1;
            if (w > 10) w = target - 1;
            g_rounds[i].wrong = w;
        } else {
            // Comparison: two unequal groups of 1..8 shapes
            int x, y;
            do { x = random(1, 9); y = random(1, 9); } while (x == y);
            bool big_left = random(0, 2);
            int L = big_left ? max(x, y) : min(x, y);
            int R = big_left ? min(x, y) : max(x, y);
            bool more = (g_mode == 3);
            int corr = more ? (L > R ? 0 : 1) : (L < R ? 0 : 1);
            g_rounds[i] = {L, R, corr, 1 - corr};
        }
    }
}

static void color_side(int side, lv_color_t c)
{
    for (int k = 0; k < SHPE; k++) {
        lv_obj_set_style_bg_color(g_shapes[side][k], c, 0);
    }
}

static void set_shape_style(lv_obj_t *o, bool circular, lv_color_t c)
{
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, circular ? 21 : 7, 0);
}

static void style_groups(void)
{
    for (int i = 0; i < 2; i++) {
        lv_obj_remove_style_all(g_btns[i]);
        lv_obj_set_size(g_btns[i], GGW, GGH);
        lv_obj_set_pos(g_btns[i], GOX + i * (GGW + GSGAP), 96);
        lv_obj_set_style_bg_opa(g_btns[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(g_btns[i], 0, 0);
        lv_obj_add_flag(g_btns[i], LV_OBJ_FLAG_CLICKABLE);
    }
}

static void start_q(void)
{
    cancel_timer();
    const MathRound &r = g_rounds[g_qn];

    char buf[32];
    if (g_mode >= 2) {
        // Question card (single line, wider)
        lv_obj_set_size(g_eq_lab, 340, 56);
        lv_obj_align(g_eq_lab, LV_ALIGN_TOP_MID, 0, 8);
        lv_obj_set_style_text_font(g_eq_lab, &lv_font_montserrat_20, 0);
        lv_obj_set_style_pad_top(g_eq_lab, 12, 0);
        lv_obj_set_style_text_color(g_eq_lab, lv_color_hex(0x555555), 0);

        const char *q = (g_mode == 3) ? "Which group has more?" : "Which group has fewer?";
        lv_label_set_text(g_eq_lab, q);

        g_lcnt = r.a;
        g_rcnt = r.b;
        g_correct_btn = r.answer;

        // Random distinct colors + complementary shapes per round
        static const lv_color_t pal[] = {
            lv_color_hex(0x3498DB), lv_color_hex(0xE67E22), lv_color_hex(0x9B59B6),
            lv_color_hex(0x27AE60), lv_color_hex(0xE74C3C)
        };
        int ci = random(0, 5);
        int cj = (ci + 1 + random(0, 4)) % 5;
        g_lcol = pal[ci];
        g_rcol = pal[cj];

        if (random(0, 2)) { g_lshape = 0; g_rshape = 1; }
        else              { g_lshape = 1; g_rshape = 0; }

        for (int side = 0; side < 2; side++) {
            for (int k = 0; k < SHPE; k++) {
                set_shape_style(g_shapes[side][k],
                    side ? (g_rshape == 0) : (g_lshape == 0),
                    side ? g_rcol : g_lcol);
            }
        }

        for (int side = 0; side < 2; side++) {
            int cnt = side ? g_rcnt : g_lcnt;
            for (int k = 0; k < SHPE; k++) {
                if (k < cnt)
                    lv_obj_clear_flag(g_shapes[side][k], LV_OBJ_FLAG_HIDDEN);
                else
                    lv_obj_add_flag(g_shapes[side][k], LV_OBJ_FLAG_HIDDEN);
            }
        }

        style_groups();
    } else {
        // Equation card (two-line, compact)
        lv_obj_set_size(g_eq_lab, 220, 72);
        lv_obj_align(g_eq_lab, LV_ALIGN_TOP_MID, 0, 8);
        lv_obj_set_style_text_font(g_eq_lab, &lv_font_montserrat_28, 0);
        lv_obj_set_style_pad_top(g_eq_lab, 20, 0);
        lv_obj_set_style_text_color(g_eq_lab, lv_color_hex(0x333333), 0);

        if (g_mode == 0)
            snprintf(buf, sizeof(buf), "%d + %d = ?", r.a, r.b);
        else
            snprintf(buf, sizeof(buf), "%d - %d = ?", r.a, r.b);
        lv_label_set_text(g_eq_lab, buf);

        g_correct_btn = random(0, 2);
        char nb[8];
        snprintf(nb, sizeof(nb), "%d", r.answer);
        lv_label_set_text(g_blabs[g_correct_btn], nb);
        snprintf(nb, sizeof(nb), "%d", r.wrong);
        lv_label_set_text(g_blabs[1 - g_correct_btn], nb);

        lv_obj_set_style_bg_color(g_btns[0], lv_color_hex(0x8B5A2B), 0);
        lv_obj_set_style_bg_color(g_btns[1], lv_color_hex(0xF5E6C8), 0);
        lv_obj_set_style_text_color(g_blabs[0], lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(g_blabs[1], lv_color_hex(0x333333), 0);
    }

    snprintf(buf, sizeof(buf), "%d / %d", g_qn + 1, TQ);
    lv_label_set_text(g_prog_lab, buf);
    lv_label_set_text(g_fb_lab, "");

    g_locked = false;
}

static void next_q(void)
{
    g_qn++;
    if (g_qn >= TQ) {
        const char *msg = g_correct_count == TQ ? "Perfect!\nAll correct!" :
                          g_correct_count >= 7 ? "Great Job!" : "Good work!";
        int stars = g_correct_count <= 3 ? 1 : g_correct_count <= 7 ? 2 : 3;
        g_overlay = nino_congrats_create(lv_obj_get_parent(g_eq_lab),
            msg, stars, NULL, NINO_COLOR_SUCCESS, 160, [](lv_event_t *) {
                lv_obj_del(g_overlay); g_overlay = nullptr;
                g_qn = 0; g_correct_count = 0;
                generate_rounds();
                start_q();
            });
        return;
    }
    start_q();
}

static void on_btn_tap(lv_event_t *e)
{
    if (g_locked) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);

    if (idx == g_correct_btn) {
        g_locked = true;
        g_correct_count++;
        if (g_mode >= 2) {
            color_side(idx, NINO_COLOR_SUCCESS);
            char buf[24];
            int big = max(g_lcnt, g_rcnt);
            int small = min(g_lcnt, g_rcnt);
            if (g_mode == 3)
                snprintf(buf, sizeof(buf), "%d > %d", big, small);
            else
                snprintf(buf, sizeof(buf), "%d < %d", small, big);
            lv_label_set_text(g_eq_lab, buf);
            lv_obj_set_style_text_color(g_eq_lab, NINO_COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_bg_color(g_btns[idx], NINO_COLOR_SUCCESS, 0);
        }
        lv_label_set_text(g_fb_lab, "Correct!");
        lv_obj_set_style_text_color(g_fb_lab, NINO_COLOR_SUCCESS, 0);
        g_timer = lv_timer_create([](lv_timer_t *t) {
            lv_timer_del(t);
            g_timer = nullptr;
            next_q();
        }, 900, nullptr);
        lv_timer_set_repeat_count(g_timer, 1);
    } else {
        if (g_mode >= 2)
            color_side(idx, NINO_COLOR_DANGER);
        else
            lv_obj_set_style_bg_color(g_btns[idx], NINO_COLOR_DANGER, 0);
        lv_label_set_text(g_fb_lab, "Try again");
        lv_obj_set_style_text_color(g_fb_lab, NINO_COLOR_DANGER, 0);
        g_timer = lv_timer_create([](lv_timer_t *t) {
            int ridx = (int)(intptr_t)lv_timer_get_user_data(t);
            lv_timer_del(t);
            g_timer = nullptr;
            if (g_mode >= 2)
                color_side(ridx, ridx ? g_rcol : g_lcol);
            else
                lv_obj_set_style_bg_color(g_btns[ridx],
                    ridx == 0 ? lv_color_hex(0x8B5A2B) : lv_color_hex(0xF5E6C8), 0);
            lv_label_set_text(g_fb_lab, "");
            g_locked = false;
        }, 500, (void *)(intptr_t)idx);
        lv_timer_set_repeat_count(g_timer, 1);
    }
}

void app_counting_jar_create(lv_obj_t *content)
{
    g_qn = 0;
    g_correct_count = 0;
    g_locked = false;
    g_timer = nullptr;
    g_overlay = nullptr;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xFFF8F0), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(content, [](lv_event_t *) { cancel_timer(); },
                        LV_EVENT_DELETE, nullptr);

    // ---- Progress ----
    g_prog_lab = lv_label_create(content);
    lv_label_set_text(g_prog_lab, "");
    lv_obj_set_style_text_color(g_prog_lab, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(g_prog_lab, &lv_font_montserrat_14, 0);
    lv_obj_align(g_prog_lab, LV_ALIGN_TOP_RIGHT, -8, 4);

    // ---- Equation / question card ----
    g_eq_lab = lv_label_create(content);
    lv_obj_set_size(g_eq_lab, 220, 72);
    lv_obj_align(g_eq_lab, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(g_eq_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(g_eq_lab, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_eq_lab, 2, 0);
    lv_obj_set_style_border_color(g_eq_lab, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(g_eq_lab, 14, 0);
    lv_obj_set_style_text_align(g_eq_lab, LV_TEXT_ALIGN_CENTER, 0);

    // ---- Comparison shape groups (behind the answer buttons) ----
    for (int side = 0; side < 2; side++) {
        for (int k = 0; k < SHPE; k++) {
            lv_obj_t *o = lv_obj_create(content);
            lv_obj_remove_style_all(o);
            lv_obj_set_size(o, CELL, CELL);
            int col = k % COLS;
            int row = k / COLS;
            lv_obj_set_pos(o, GOX + side * (GGW + GSGAP) + col * (CELL + CGAP),
                              GOY + row * (CELL + CGAP));
            lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
            g_shapes[side][k] = o;
        }
    }

    // ---- Answer buttons ----
    int bw = 150, bh = 90, gap = 24;
    int ox = (464 - 2 * bw - gap) / 2;

    for (int i = 0; i < 2; i++) {
        g_btns[i] = lv_btn_create(content);
        lv_obj_set_size(g_btns[i], bw, bh);
        lv_obj_set_pos(g_btns[i], ox + i * (bw + gap), 90);
        lv_obj_set_style_radius(g_btns[i], 60, 0);
        lv_obj_set_style_shadow_width(g_btns[i], 0, 0);
        lv_obj_add_event_cb(g_btns[i], on_btn_tap, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        g_blabs[i] = lv_label_create(g_btns[i]);
        lv_label_set_text(g_blabs[i], "");
        lv_obj_set_style_text_font(g_blabs[i], &lv_font_montserrat_28, 0);
        lv_obj_center(g_blabs[i]);
    }

    lv_obj_set_style_bg_color(g_btns[0], lv_color_hex(0x8B5A2B), 0);
    lv_obj_set_style_bg_color(g_btns[1], lv_color_hex(0xF5E6C8), 0);
    lv_obj_set_style_text_color(g_blabs[0], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(g_blabs[1], lv_color_hex(0x333333), 0);

    // ---- Feedback ----
    g_fb_lab = lv_label_create(content);
    lv_label_set_text(g_fb_lab, "");
    lv_obj_set_style_text_font(g_fb_lab, &lv_font_montserrat_20, 0);
    lv_obj_align(g_fb_lab, LV_ALIGN_TOP_MID, 0, 192);

    // ---- Mode picker ----
    g_overlay = lv_obj_create(content);
    lv_obj_remove_style_all(g_overlay);
    lv_obj_set_size(g_overlay, 480, 276);
    lv_obj_align(g_overlay, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(g_overlay, lv_color_hex(0xFFF8F0), 0);
    lv_obj_set_style_bg_opa(g_overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(g_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tl = lv_label_create(g_overlay);
    lv_label_set_text(tl, "Pick your ice cream!");
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(tl, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_align(tl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(tl, LV_ALIGN_TOP_MID, 0, 30);

    static const lv_color_t mode_colors[] = {
        lv_color_hex(0x6B3E0D), lv_color_hex(0xF5E6C8),
        lv_color_hex(0xF68BBE), lv_color_hex(0xA8E6CF)
    };
    static const char *mode_names[]   = { "Chocolate", "Vanilla", "Strawberry", "Mint" };
    static const char *mode_subtitles[] = { "Addition", "Subtraction", "Which has more?", "Which has fewer?" };
    static const bool mode_light[]    = { true, false, true, false };

    int mw = 190, mh = 64, mgap = 14;
    int mox = (480 - 2 * mw - mgap) / 2;
    for (int i = 0; i < 4; i++) {
        int col = i % 2;
        int row = i / 2;
        lv_obj_t *b = lv_btn_create(g_overlay);
        lv_obj_set_size(b, mw, mh);
        lv_obj_set_pos(b, mox + col * (mw + mgap), (row == 0 ? 78 : 154));
        lv_obj_set_style_radius(b, 20, 0);
        lv_obj_set_style_shadow_width(b, 0, 0);
        lv_obj_set_style_bg_color(b, mode_colors[i], 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);

        lv_color_t fg = mode_light[i] ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x333333);

        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, mode_names[i]);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(l, fg, 0);
        lv_obj_align(l, LV_ALIGN_CENTER, 0, -6);

        lv_obj_t *st = lv_label_create(b);
        lv_label_set_text(st, mode_subtitles[i]);
        lv_obj_set_style_text_font(st, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(st, fg, 0);
        lv_obj_set_style_text_opa(st, LV_OPA_80, 0);
        lv_obj_align(st, LV_ALIGN_CENTER, 0, 16);

        lv_obj_add_event_cb(b, [](lv_event_t *e) {
            g_mode = (int)(intptr_t)lv_event_get_user_data(e);
            lv_obj_del(g_overlay);
            g_overlay = nullptr;
            generate_rounds();
            start_q();
        }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}