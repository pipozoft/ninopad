#include "app_ten_frame.h"
#include "nino_colors.h"
#include <Arduino.h>

#define SUN_MAX     20
#define CELL_W      42
#define CELL_GAP     4
#define FRAME_W    226
#define TWO_GAP     12
#define TWO_W      464
#define AVAIL_W    464
#define TQ          10

// ---- State ----
static int qn;
static int sun_cnt;
static int bvals[4];
static int cidx;
static bool locked;
static lv_obj_t *body[SUN_MAX];
static lv_obj_t *btns[4];
static lv_obj_t *blabs[4];
static lv_obj_t *prog_lab;
static lv_obj_t *fb_lab;
static lv_obj_t *overlay;
static lv_timer_t *adv_tmr;
static lv_timer_t *rst_tmr;

// ---- Helpers ----

static int max_for_q(int q)
{
    if (q < 3) return 5;
    if (q < 6) return 10;
    if (q < 8) return 15;
    return 20;
}

static int min_for_q(int q)
{
    if (q < 3) return 1;
    if (q < 6) return 6;
    if (q < 8) return 11;
    return 16;
}

static void shuf(int arr[], int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = random(i + 1);
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}

static void position_suns(void)
{
    bool dual = sun_cnt > 10;
    int fx0 = dual ? (AVAIL_W - TWO_W) / 2 : (AVAIL_W - FRAME_W) / 2;
    int fx1 = fx0 + FRAME_W + TWO_GAP;
    int fy = 50;

    for (int i = 0; i < SUN_MAX; i++) {
        int frame = i / 10;
        int off = i % 10;
        int col = off % 5;
        int row = off / 5;
        int x = (frame == 0 ? fx0 : fx1) + col * (CELL_W + CELL_GAP);
        int y = fy + row * (CELL_W + CELL_GAP);
        lv_obj_set_pos(body[i], x, y);
    }
}

static void add_face(lv_obj_t *parent)
{
    lv_obj_t *el = lv_obj_create(parent);
    lv_obj_remove_style_all(el);
    lv_obj_set_size(el, 5, 5);
    lv_obj_set_pos(el, 12, 12);
    lv_obj_set_style_radius(el, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(el, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(el, LV_OPA_COVER, 0);
    lv_obj_clear_flag(el, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(el, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *er = lv_obj_create(parent);
    lv_obj_remove_style_all(er);
    lv_obj_set_size(er, 5, 5);
    lv_obj_set_pos(er, 25, 12);
    lv_obj_set_style_radius(er, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(er, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(er, LV_OPA_COVER, 0);
    lv_obj_clear_flag(er, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(er, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *sm = lv_label_create(parent);
    lv_label_set_text(sm, "~");
    lv_obj_set_pos(sm, 16, 23);
    lv_obj_set_style_text_color(sm, lv_color_hex(0x222222), 0);
    lv_obj_set_style_text_font(sm, &lv_font_montserrat_14, 0);
}

static void start_q(void)
{
    if (overlay) {
        lv_obj_del(overlay);
        overlay = NULL;
    }

    int maxv = max_for_q(qn);
    int minv = min_for_q(qn);
    sun_cnt = random(minv, maxv + 1);

    position_suns();

    int f1 = sun_cnt > 10 ? 10 : sun_cnt;
    int f2 = sun_cnt > 10 ? sun_cnt - 10 : 0;

    for (int i = 0; i < SUN_MAX; i++) {
        if (i < f1 || (i >= 10 && i < 10 + f2))
            lv_obj_clear_flag(body[i], LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(body[i], LV_OBJ_FLAG_HIDDEN);
    }

    int lo = max(1, sun_cnt - 3);
    int hi = min(20, sun_cnt + 3);
    int pool[12], np = 0;
    for (int v = lo; v <= hi; v++) {
        if (v != sun_cnt) pool[np++] = v;
    }
    shuf(pool, np);

    bvals[0] = sun_cnt;
    for (int i = 0; i < 3; i++) bvals[i + 1] = pool[i % np];
    shuf(bvals, 4);

    cidx = 0;
    for (int i = 0; i < 4; i++) {
        if (bvals[i] == sun_cnt) { cidx = i; break; }
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
    overlay = lv_obj_create(lv_obj_get_parent(body[0]));
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, 480, 276);
    lv_obj_align(overlay, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0xF0FFF0), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ml = lv_label_create(overlay);
    lv_label_set_text(ml, "Great Job!\nYou matched all the numbers!");
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

    if (rst_tmr) {
        lv_timer_del(rst_tmr);
        rst_tmr = NULL;
        lv_label_set_text(fb_lab, "");
        for (int i = 0; i < 4; i++)
            lv_obj_set_style_bg_color(btns[i], lv_color_hex(0x3498DB), 0);
    }

    if (idx == cidx) {
        locked = true;
        lv_obj_set_style_bg_color(btns[idx], lv_color_hex(0x2ECC71), 0);
        lv_label_set_text(fb_lab, LV_SYMBOL_OK);
        lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x2ECC71), 0);
        lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_28, 0);

        adv_tmr = lv_timer_create([](lv_timer_t *tm) {
            lv_timer_del(tm);
            adv_tmr = NULL;
            next_q();
        }, 1000, NULL);
        lv_timer_set_repeat_count(adv_tmr, 1);
    } else {
        lv_obj_set_style_bg_color(btns[idx], lv_color_hex(0xE74C3C), 0);
        lv_label_set_text(fb_lab, "Try Again");
        lv_obj_set_style_text_color(fb_lab, lv_color_hex(0xE74C3C), 0);
        lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_16, 0);

        rst_tmr = lv_timer_create([](lv_timer_t *tm) {
            lv_timer_del(tm);
            rst_tmr = NULL;
            int idx2 = (int)(intptr_t)lv_timer_get_user_data(tm);
            lv_obj_set_style_bg_color(btns[idx2], lv_color_hex(0x3498DB), 0);
            lv_label_set_text(fb_lab, "");
        }, 600, (void *)(intptr_t)idx);
        lv_timer_set_repeat_count(rst_tmr, 1);
    }
}

// ---- App entry ----

void app_ten_frame_create(lv_obj_t *content)
{
    qn = 0;
    locked = false;
    overlay = NULL;
    adv_tmr = NULL;
    rst_tmr = NULL;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(content, [](lv_event_t *) {
        if (adv_tmr) { lv_timer_del(adv_tmr); adv_tmr = NULL; }
        if (rst_tmr) { lv_timer_del(rst_tmr); rst_tmr = NULL; }
    }, LV_EVENT_DELETE, NULL);

    // ---- Title ----
    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, "10-Frame Sun");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF8C00), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    // ---- Instruction ----
    lv_obj_t *instr = lv_label_create(content);
    lv_label_set_text(instr, "Count the suns and choose the correct number.");
    lv_obj_set_style_text_font(instr, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(instr, lv_color_hex(0x666666), 0);
    lv_obj_align(instr, LV_ALIGN_TOP_MID, 0, 28);

    // ---- Progress ----
    prog_lab = lv_label_create(content);
    lv_label_set_text(prog_lab, "1 / 10");
    lv_obj_set_style_text_font(prog_lab, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(prog_lab, lv_color_hex(0x888888), 0);
    lv_obj_align(prog_lab, LV_ALIGN_TOP_RIGHT, -8, 8);

    // ---- Suns ----
    for (int i = 0; i < SUN_MAX; i++) {
        body[i] = lv_obj_create(content);
        lv_obj_remove_style_all(body[i]);
        lv_obj_set_size(body[i], CELL_W, CELL_W);
        lv_obj_set_style_radius(body[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(body[i], lv_color_hex(0xFFD700), 0);
        lv_obj_set_style_bg_opa(body[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(body[i], lv_color_hex(0xFFA500), 0);
        lv_obj_set_style_border_width(body[i], 3, 0);
        lv_obj_clear_flag(body[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(body[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(body[i], LV_OBJ_FLAG_HIDDEN);
        add_face(body[i]);
    }

    // ---- Answer buttons ----
    int bw = 88, bh = 48, bg = 10;
    int bx = (480 - 4 * bw - 3 * bg) / 2;

    for (int i = 0; i < 4; i++) {
        btns[i] = lv_btn_create(content);
        lv_obj_set_style_bg_color(btns[i], lv_color_hex(0x3498DB), 0);
        lv_obj_set_size(btns[i], bw, bh);
        lv_obj_set_pos(btns[i], bx + i * (bw + bg), 154);
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
    lv_obj_align(fb_lab, LV_ALIGN_TOP_MID, 0, 210);

    // ---- Start ----
    start_q();
}
