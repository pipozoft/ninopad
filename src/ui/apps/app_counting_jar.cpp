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
        } else {
            int target = random(0, 2) ? 3 : 4;
            int m = random(target + 1, target + 6);
            g_rounds[i] = {m, m - target, target, 0};
            int w = (random(0, 2) ? target + 1 : target - 1);
            if (w == target || w < 1) w = target + 1;
            if (w > 10) w = target - 1;
            g_rounds[i].wrong = w;
        }
    }
}

static void start_q(void)
{
    cancel_timer();
    const MathRound &r = g_rounds[g_qn];

    char buf[32];
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

    snprintf(buf, sizeof(buf), "%d / %d", g_qn + 1, TQ);
    lv_label_set_text(g_prog_lab, buf);
    lv_label_set_text(g_fb_lab, "");

    lv_obj_set_style_bg_color(g_btns[0], lv_color_hex(0x8B5A2B), 0);
    lv_obj_set_style_bg_color(g_btns[1], lv_color_hex(0xF5E6C8), 0);
    lv_obj_set_style_text_color(g_blabs[0], lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(g_blabs[1], lv_color_hex(0x333333), 0);

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
        lv_obj_set_style_bg_color(g_btns[idx], NINO_COLOR_SUCCESS, 0);
        lv_label_set_text(g_fb_lab, "Correct!");
        lv_obj_set_style_text_color(g_fb_lab, NINO_COLOR_SUCCESS, 0);
        g_timer = lv_timer_create([](lv_timer_t *t) {
            lv_timer_del(t);
            g_timer = nullptr;
            next_q();
        }, 900, nullptr);
        lv_timer_set_repeat_count(g_timer, 1);
    } else {
        lv_obj_set_style_bg_color(g_btns[idx], NINO_COLOR_DANGER, 0);
        lv_label_set_text(g_fb_lab, "Try again");
        lv_obj_set_style_text_color(g_fb_lab, NINO_COLOR_DANGER, 0);
        g_timer = lv_timer_create([](lv_timer_t *t) {
            lv_timer_del(t);
            g_timer = nullptr;
            int ridx = (int)(intptr_t)lv_timer_get_user_data(t);
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

    // ---- Equation ----
    g_eq_lab = lv_label_create(content);
    lv_obj_set_size(g_eq_lab, 220, 72);
    lv_obj_align(g_eq_lab, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_color(g_eq_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(g_eq_lab, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_eq_lab, 2, 0);
    lv_obj_set_style_border_color(g_eq_lab, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(g_eq_lab, 14, 0);
    lv_obj_set_style_text_align(g_eq_lab, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(g_eq_lab, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(g_eq_lab, lv_color_hex(0x333333), 0);
    lv_obj_set_style_pad_top(g_eq_lab, 20, 0);

    // ---- Cone buttons ----
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
    lv_obj_align(tl, LV_ALIGN_TOP_MID, 0, 50);

    static const lv_color_t mode_colors[] = { lv_color_hex(0x6B3E0D), lv_color_hex(0xF5E6C8) };
    static const char *mode_names[] = { "Chocolate", "Vanilla" };

    int mw = 180, mgap = 24;
    int mox = (480 - 2 * mw - mgap) / 2;
    for (int i = 0; i < 2; i++) {
        lv_obj_t *b = lv_btn_create(g_overlay);
        lv_obj_set_size(b, mw, 72);
        lv_obj_set_pos(b, mox + i * (mw + mgap), 110);
        lv_obj_set_style_radius(b, 20, 0);
        lv_obj_set_style_shadow_width(b, 0, 0);
        lv_obj_set_style_bg_color(b, mode_colors[i], 0);

        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, mode_names[i]);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(l,
            i == 0 ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x333333), 0);
        lv_obj_center(l);

        lv_obj_add_event_cb(b, [](lv_event_t *e) {
            g_mode = (int)(intptr_t)lv_event_get_user_data(e);
            lv_obj_del(g_overlay);
            g_overlay = nullptr;
            generate_rounds();
            start_q();
        }, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}
