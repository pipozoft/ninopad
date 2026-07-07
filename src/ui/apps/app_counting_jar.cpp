#include "app_counting_jar.h"
#include "nino_colors.h"
#include <Arduino.h>

#define ITEM_MAX 10
#define TQ 10

// ---- State ----
static int qn;
static int item_cnt;
static int bvals[4];
static int cidx;
static volatile bool locked;
static lv_obj_t *items[ITEM_MAX];
static lv_obj_t *btns[4];
static lv_obj_t *blabs[4];
static lv_obj_t *jar;
static lv_obj_t *prog_lab;
static lv_obj_t *fb_lab;
static lv_obj_t *overlay;

// ---- Helpers ----

static int max_for_q(int q)
{
    if (q < 3) return 3;
    if (q < 6) return 5;
    return 10;
}

static void shuf(int arr[], int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = random(i + 1);
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}

static void start_q(void)
{
    if (overlay) {
        lv_obj_del(overlay);
        overlay = NULL;
    }

    int maxv = max_for_q(qn);
    item_cnt = random(1, maxv + 1);

    for (int i = 0; i < ITEM_MAX; i++) {
        if (i < item_cnt)
            lv_obj_clear_flag(items[i], LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(items[i], LV_OBJ_FLAG_HIDDEN);
    }

    // 4 unique answers: correct + 3 distractors from expanded pool
    int pool[12], np = 0;
    int hi = maxv < 4 ? 6 : maxv + 2;
    for (int i = 1; i <= hi; i++) {
        if (i != item_cnt) pool[np++] = i;
    }
    shuf(pool, np);

    bvals[0] = item_cnt;
    for (int i = 0; i < 3; i++) bvals[i + 1] = pool[i % np];
    shuf(bvals, 4);

    cidx = 0;
    for (int i = 0; i < 4; i++) {
        if (bvals[i] == item_cnt) { cidx = i; break; }
    }

    for (int i = 0; i < 4; i++) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", bvals[i]);
        lv_label_set_text(blabs[i], buf);
        lv_obj_set_style_bg_color(btns[i], lv_color_hex(0x3498DB), 0);
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%d / %d", qn + 1, TQ);
    lv_label_set_text(prog_lab, buf);

    lv_label_set_text(fb_lab, "");

    locked = false;
}

static void show_congrats(void)
{
    overlay = lv_obj_create(lv_obj_get_parent(jar));
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, 480, 276);
    lv_obj_align(overlay, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0xF0FFF0), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ml = lv_label_create(overlay);
    lv_label_set_text(ml, "Great Job!\nYou counted all the coins!");
    lv_obj_set_style_text_font(ml, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(ml, lv_color_hex(0x27AE60), 0);
    lv_obj_set_style_text_align(ml, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ml, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *pb = lv_btn_create(overlay);
    lv_obj_set_style_bg_color(pb, lv_color_hex(0x27AE60), 0);
    lv_obj_set_size(pb, 160, 52);
    lv_obj_set_style_radius(pb, 26, 0);
    lv_obj_set_style_shadow_width(pb, 0, 0);
    lv_obj_align(pb, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_event_cb(pb, [](lv_event_t *e) {
        (void)e;
        lv_obj_del(overlay);
        overlay = NULL;
        qn = 0;
        start_q();
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
    if (qn >= TQ) {
        show_congrats();
        return;
    }
    start_q();
}

static void on_btn_tap(lv_event_t *e)
{
    if (locked) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);

    if (idx == cidx) {
        locked = true;
        lv_obj_set_style_bg_color(btns[idx], lv_color_hex(0x2ECC71), 0);
        lv_label_set_text(fb_lab, LV_SYMBOL_OK);
        lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x2ECC71), 0);
        lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_28, 0);

        lv_timer_t *t = lv_timer_create([](lv_timer_t *tm) {
            lv_timer_del(tm);
            next_q();
        }, 1000, NULL);
        lv_timer_set_repeat_count(t, 1);
    } else {
        lv_obj_set_style_bg_color(btns[idx], lv_color_hex(0xE74C3C), 0);
        lv_label_set_text(fb_lab, "Try Again");
        lv_obj_set_style_text_color(fb_lab, lv_color_hex(0xE74C3C), 0);
        lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_16, 0);

        lv_timer_t *t = lv_timer_create([](lv_timer_t *tm) {
            lv_timer_del(tm);
            int idx2 = (int)(intptr_t)lv_timer_get_user_data(tm);
            lv_obj_set_style_bg_color(btns[idx2], lv_color_hex(0x3498DB), 0);
            lv_label_set_text(fb_lab, "");
        }, 600, (void *)(intptr_t)idx);
        lv_timer_set_repeat_count(t, 1);
    }
}

// ---- App entry ----

void app_counting_jar_create(lv_obj_t *content)
{
    qn = 0;
    locked = false;
    overlay = NULL;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // ---- Top: instruction + progress ----
    lv_obj_t *top = lv_obj_create(content);
    lv_obj_remove_style_all(top);
    lv_obj_set_size(top, 480, 18);
    lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *il = lv_label_create(top);
    lv_label_set_text(il, " Count the coins inside the jar.");
    lv_obj_set_style_text_color(il, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);

    lv_obj_t *sp = lv_obj_create(top);
    lv_obj_remove_style_all(sp);
    lv_obj_set_flex_grow(sp, 1);
    lv_obj_clear_flag(sp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(sp, LV_OBJ_FLAG_CLICKABLE);

    prog_lab = lv_label_create(top);
    lv_label_set_text(prog_lab, "1 / 10");
    lv_obj_set_style_text_color(prog_lab, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(prog_lab, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_right(prog_lab, 8, 0);

    // ---- Jar ----
    jar = lv_obj_create(content);
    lv_obj_remove_style_all(jar);
    lv_obj_set_size(jar, 340, 140);
    lv_obj_align(jar, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_border_color(jar, lv_color_hex(0x888888), 0);
    lv_obj_set_style_border_width(jar, 3, 0);
    lv_obj_set_style_radius(jar, 14, 0);
    lv_obj_set_style_bg_color(jar, lv_color_hex(0xEEEEEE), 0);
    lv_obj_set_style_bg_opa(jar, LV_OPA_COVER, 0);
    lv_obj_set_style_clip_corner(jar, true, 0);
    lv_obj_clear_flag(jar, LV_OBJ_FLAG_SCROLLABLE);

    // Coins: 5×2 grid, 32px circles
    const int sz = 32;
    const int step = 48;
    int ox = (340 - 4 * step - sz) / 2;
    int oy = (140 - 1 * step - sz) / 2;

    for (int i = 0; i < ITEM_MAX; i++) {
        int col = i % 5;
        int row = i / 5;
        int px = ox + col * step;
        int py = oy + row * step;

        items[i] = lv_obj_create(jar);
        lv_obj_remove_style_all(items[i]);
        lv_obj_set_size(items[i], sz, sz);
        lv_obj_set_pos(items[i], px, py);
        lv_obj_set_style_radius(items[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(items[i], 3, 0);
        lv_obj_set_style_border_color(items[i], lv_color_hex(0xDAA520), 0);
        lv_obj_set_style_bg_color(items[i], lv_color_hex(0xFFD700), 0);
        lv_obj_set_style_bg_opa(items[i], LV_OPA_COVER, 0);
        lv_obj_add_flag(items[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(items[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(items[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    // ---- Answer buttons ----
    int bw = 88, bh = 48, bg = 10;
    int bx = (480 - 4 * bw - 3 * bg) / 2;

    for (int i = 0; i < 4; i++) {
        btns[i] = lv_btn_create(content);
        lv_obj_set_style_bg_color(btns[i], lv_color_hex(0x3498DB), 0);
        lv_obj_set_size(btns[i], bw, bh);
        lv_obj_set_pos(btns[i], bx + i * (bw + bg), 168);
        lv_obj_set_style_radius(btns[i], 10, 0);
        lv_obj_set_style_shadow_width(btns[i], 0, 0);
        lv_obj_set_style_border_width(btns[i], 2, 0);
        lv_obj_set_style_border_color(btns[i], lv_color_hex(0x2980B9), 0);
        lv_obj_add_event_cb(btns[i], on_btn_tap, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        blabs[i] = lv_label_create(btns[i]);
        lv_label_set_text(blabs[i], "");
        lv_obj_set_style_text_color(blabs[i], lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(blabs[i], &lv_font_montserrat_24, 0);
        lv_obj_center(blabs[i]);
    }

    // ---- Feedback ----
    fb_lab = lv_label_create(content);
    lv_label_set_text(fb_lab, "");
    lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_16, 0);
    lv_obj_align(fb_lab, LV_ALIGN_TOP_MID, 0, 224);

    // ---- Start ----
    start_q();
}
