#include "app_word_spy.h"
#include "nino_colors.h"
#include <Arduino.h>
#include <string.h>

#define ROUNDS_PER_GAME 10
#define TOTAL_ROUNDS    15

// ---- Drawing helpers ----

static lv_obj_t *circ(lv_obj_t *p, int x, int y, int r, lv_color_t c)
{
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, r*2, r*2);
    lv_obj_set_pos(o, x - r, y - r);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static lv_obj_t *rect(lv_obj_t *p, int x, int y, int w, int h, lv_color_t c)
{
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, 2, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

// ---- Picture drawings (all draw into ~150x110 area) ----

static void dr_dog(lv_obj_t *p)
{
    circ(p, 75, 60, 32, lv_color_hex(0xD4A574));
    circ(p, 80, 60, 22, lv_color_hex(0xE8C9A0));
    circ(p, 63, 50, 5, lv_color_hex(0x222222));
    circ(p, 87, 50, 5, lv_color_hex(0x222222));
    circ(p, 75, 70, 8, lv_color_hex(0x222222));
    rect(p, 70, 79, 10, 6, lv_color_hex(0xE74C3C));
    rect(p, 42, 42, 14, 24, lv_color_hex(0x8B6914));
    rect(p, 94, 42, 14, 24, lv_color_hex(0x8B6914));
}

static void dr_pig(lv_obj_t *p)
{
    circ(p, 75, 58, 30, lv_color_hex(0xFFB6C1));
    circ(p, 75, 58, 24, lv_color_hex(0xFFD1DC));
    circ(p, 75, 68, 12, lv_color_hex(0xFF8CA0));
    circ(p, 75, 68, 6, lv_color_hex(0x444444));
    circ(p, 60, 48, 4, lv_color_hex(0x222222));
    circ(p, 90, 48, 4, lv_color_hex(0x222222));
    rect(p, 52, 30, 8, 14, lv_color_hex(0xFFB6C1));
    rect(p, 90, 30, 8, 14, lv_color_hex(0xFFB6C1));
}

static void dr_cat(lv_obj_t *p)
{
    circ(p, 75, 58, 28, lv_color_hex(0xFF8C00));
    rect(p, 48, 28, 12, 20, lv_color_hex(0xFF8C00));
    rect(p, 90, 28, 12, 20, lv_color_hex(0xFF8C00));
    circ(p, 63, 50, 4, lv_color_hex(0x222222));
    circ(p, 87, 50, 4, lv_color_hex(0x222222));
    circ(p, 75, 60, 4, lv_color_hex(0xFF69B4));
    rect(p, 50, 58, 6, 2, lv_color_hex(0x222222));
    rect(p, 94, 58, 6, 2, lv_color_hex(0x222222));
}

static void dr_fox(lv_obj_t *p)
{
    circ(p, 75, 58, 26, lv_color_hex(0xFF6600));
    rect(p, 50, 30, 12, 18, lv_color_hex(0xFF6600));
    rect(p, 88, 30, 12, 18, lv_color_hex(0xFF6600));
    circ(p, 65, 50, 4, lv_color_hex(0x222222));
    circ(p, 85, 50, 4, lv_color_hex(0x222222));
    circ(p, 75, 60, 5, lv_color_hex(0x222222));
    rect(p, 62, 44, 26, 5, lv_color_hex(0xFFFFFF));
}

static void dr_hen(lv_obj_t *p)
{
    circ(p, 75, 62, 24, lv_color_hex(0xFFFF00));
    circ(p, 75, 48, 14, lv_color_hex(0xFFFF00));
    circ(p, 65, 46, 3, lv_color_hex(0x222222));
    circ(p, 85, 46, 3, lv_color_hex(0x222222));
    rect(p, 100, 56, 14, 7, lv_color_hex(0xFFA500));
    circ(p, 75, 34, 6, lv_color_hex(0xFF0000));
    circ(p, 90, 52, 10, lv_color_hex(0xFFDD00));
}

static void dr_bug(lv_obj_t *p)
{
    circ(p, 75, 55, 22, lv_color_hex(0x32CD32));
    circ(p, 75, 55, 14, lv_color_hex(0x228B22));
    circ(p, 58, 42, 10, lv_color_hex(0x32CD32));
    circ(p, 92, 42, 10, lv_color_hex(0x32CD32));
    circ(p, 58, 42, 5, lv_color_hex(0xFFFFFF));
    circ(p, 92, 42, 5, lv_color_hex(0xFFFFFF));
    circ(p, 58, 42, 3, lv_color_hex(0x222222));
    circ(p, 92, 42, 3, lv_color_hex(0x222222));
    rect(p, 65, 14, 4, 14, lv_color_hex(0x222222));
    rect(p, 81, 14, 4, 14, lv_color_hex(0x222222));
}

static void dr_mug(lv_obj_t *p)
{
    rect(p, 42, 30, 60, 62, lv_color_hex(0xDDDDDD));
    rect(p, 48, 36, 48, 50, lv_color_hex(0xCC8844));
    rect(p, 100, 42, 22, 16, lv_color_hex(0xDDDDDD));
    rect(p, 36, 26, 72, 8, lv_color_hex(0xAAAAAA));
    circ(p, 62, 82, 6, lv_color_hex(0xCC8844));
    circ(p, 82, 82, 6, lv_color_hex(0xCC8844));
}

static void dr_log(lv_obj_t *p)
{
    rect(p, 18, 42, 114, 34, lv_color_hex(0x8B4513));
    rect(p, 28, 32, 14, 52, lv_color_hex(0x6B3410));
    rect(p, 76, 32, 14, 52, lv_color_hex(0x6B3410));
    rect(p, 50, 36, 10, 48, lv_color_hex(0x6B3410));
    circ(p, 18, 59, 20, lv_color_hex(0x8B4513));
    circ(p, 132, 59, 20, lv_color_hex(0x8B4513));
    circ(p, 18, 59, 14, lv_color_hex(0xA0764A));
    circ(p, 132, 59, 14, lv_color_hex(0xA0764A));
}

static void dr_map(lv_obj_t *p)
{
    rect(p, 30, 20, 90, 80, lv_color_hex(0x90EE90));
    rect(p, 30, 20, 28, 34, lv_color_hex(0x228B22));
    rect(p, 68, 68, 34, 22, lv_color_hex(0x8B4513));
    rect(p, 86, 20, 22, 22, lv_color_hex(0x87CEEB));
    rect(p, 36, 56, 22, 22, lv_color_hex(0xFFD700));
    rect(p, 30, 20, 4, 80, lv_color_hex(0x444444));
    rect(p, 30, 96, 90, 4, lv_color_hex(0x444444));
    circ(p, 76, 60, 4, lv_color_hex(0xE74C3C));
}

static void dr_hat(lv_obj_t *p)
{
    rect(p, 26, 68, 98, 8, lv_color_hex(0x8B4513));
    rect(p, 40, 22, 70, 48, lv_color_hex(0xE74C3C));
    rect(p, 40, 22, 70, 6, lv_color_hex(0xFF69B4));
    rect(p, 46, 30, 58, 3, lv_color_hex(0xFFFFFF));
    circ(p, 75, 72, 4, lv_color_hex(0xFFFFFF));
}

static void dr_bed(lv_obj_t *p)
{
    rect(p, 14, 62, 122, 20, lv_color_hex(0xFFFFFF));
    rect(p, 14, 34, 122, 32, lv_color_hex(0x3498DB));
    rect(p, 18, 38, 36, 22, lv_color_hex(0xFFFFFF));
    rect(p, 16, 82, 8, 18, lv_color_hex(0x8B4513));
    rect(p, 126, 82, 8, 18, lv_color_hex(0x8B4513));
    circ(p, 48, 48, 6, lv_color_hex(0xFF69B4));
    circ(p, 60, 48, 6, lv_color_hex(0xFF69B4));
    rect(p, 50, 68, 6, 4, lv_color_hex(0x8B4513));
}

static void dr_fish(lv_obj_t *p)
{
    rect(p, 24, 42, 72, 26, lv_color_hex(0x4A90D9));
    circ(p, 96, 48, 16, lv_color_hex(0x4A90D9));
    rect(p, 98, 32, 22, 30, lv_color_hex(0x4A90D9));
    rect(p, 108, 38, 12, 18, lv_color_hex(0x3A7BC8));
    circ(p, 40, 48, 6, lv_color_hex(0xFFFFFF));
    circ(p, 40, 48, 4, lv_color_hex(0x222222));
    rect(p, 46, 38, 30, 6, lv_color_hex(0x87CEEB));
}

static void dr_sun(lv_obj_t *p)
{
    rect(p, 62, 6, 6, 14, lv_color_hex(0xFFD700));
    rect(p, 62, 90, 6, 14, lv_color_hex(0xFFD700));
    rect(p, 16, 56, 14, 6, lv_color_hex(0xFFD700));
    rect(p, 100, 56, 14, 6, lv_color_hex(0xFFD700));
    rect(p, 28, 22, 6, 10, lv_color_hex(0xFFD700));
    rect(p, 96, 22, 6, 10, lv_color_hex(0xFFD700));
    rect(p, 28, 78, 6, 10, lv_color_hex(0xFFD700));
    rect(p, 96, 78, 6, 10, lv_color_hex(0xFFD700));
    circ(p, 65, 55, 24, lv_color_hex(0xFFD700));
    circ(p, 65, 55, 18, lv_color_hex(0xFFF176));
}

static void dr_van(lv_obj_t *p)
{
    rect(p, 12, 30, 106, 40, lv_color_hex(0x3498DB));
    rect(p, 50, 14, 56, 22, lv_color_hex(0x2980B9));
    rect(p, 54, 18, 20, 14, lv_color_hex(0x87CEEB));
    rect(p, 80, 18, 20, 14, lv_color_hex(0x87CEEB));
    circ(p, 32, 72, 10, lv_color_hex(0x333333));
    circ(p, 90, 72, 10, lv_color_hex(0x333333));
    circ(p, 32, 72, 5, lv_color_hex(0x888888));
    circ(p, 90, 72, 5, lv_color_hex(0x888888));
    rect(p, 12, 44, 20, 10, lv_color_hex(0x87CEEB));
}

static void dr_jam(lv_obj_t *p)
{
    rect(p, 36, 36, 64, 48, lv_color_hex(0xFFFFFF));
    rect(p, 30, 28, 76, 12, lv_color_hex(0xE74C3C));
    rect(p, 36, 52, 64, 24, lv_color_hex(0xFF69B4));
    rect(p, 40, 56, 10, 4, lv_color_hex(0xFFFFFF));
    rect(p, 56, 56, 10, 4, lv_color_hex(0xFFFFFF));
    rect(p, 72, 56, 10, 4, lv_color_hex(0xFFFFFF));
    circ(p, 52, 68, 6, lv_color_hex(0xE74C3C));
    circ(p, 78, 68, 6, lv_color_hex(0xE74C3C));
}

// ---- Round data ----

struct Round {
    const char *correct;
    const char *wrong;
    void (*draw)(lv_obj_t *);
};

static const Round rounds[TOTAL_ROUNDS] = {
    {"dog", "jog", dr_dog}, {"pig", "dig", dr_pig}, {"cat", "mat", dr_cat},
    {"fox", "vex", dr_fox}, {"hen", "ten", dr_hen}, {"bug", "beg", dr_bug},
    {"mug", "rug", dr_mug}, {"log", "fog", dr_log}, {"map", "rap", dr_map},
    {"hat", "bat", dr_hat}, {"bed", "fed", dr_bed}, {"fish","dish",dr_fish},
    {"sun", "run", dr_sun}, {"van", "man", dr_van}, {"jam", "ram", dr_jam},
};

// ---- State ----

static lv_obj_t *parent_content;
static int game_order[ROUNDS_PER_GAME];
static int qn, score;
static bool locked;
static int correct_btn;

static lv_obj_t *pic_cont;
static lv_obj_t *btn1, *btn2;
static lv_obj_t *btn1_lab, *btn2_lab;
static lv_obj_t *fb_lab, *score_lab;
static lv_obj_t *overlay;
static lv_timer_t *adv_tmr;

// ---- Helpers ----

static void shuf(int a[], int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = random(i + 1);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

// ---- Animations ----

static void shake_btn(lv_obj_t *btn)
{
    lv_coord_t ox = lv_obj_get_x(btn);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn);
    lv_anim_set_exec_cb(&a, [](void *v, int32_t x) { lv_obj_set_x((lv_obj_t *)v, x); });
    lv_anim_set_values(&a, ox - 6, ox + 6);
    lv_anim_set_time(&a, 30);
    lv_anim_set_reverse_duration(&a, 30);
    lv_anim_set_repeat_count(&a, 3);
    lv_anim_set_user_data(&a, (void *)(intptr_t)ox);
    lv_anim_set_ready_cb(&a, [](lv_anim_t *a) {
        lv_obj_set_x((lv_obj_t *)a->var, (intptr_t)lv_anim_get_user_data(a));
    });
    lv_anim_start(&a);
}

static void pop_checkmark(void)
{
    lv_obj_t *ck = lv_obj_create(pic_cont);
    lv_obj_remove_style_all(ck);
    lv_obj_set_size(ck, 64, 64);
    lv_obj_center(ck);
    lv_obj_set_style_radius(ck, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ck, lv_color_hex(0x2ECC71), 0);
    lv_obj_set_style_bg_opa(ck, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ck, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *l = lv_label_create(ck);
    lv_label_set_text(l, LV_SYMBOL_OK);
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);
    lv_obj_center(l);

    // Fade in — opacity does not require a layer buffer
    lv_obj_set_style_opa(ck, 0, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ck);
    lv_anim_set_exec_cb(&a, [](void *v, int32_t opa) {
        lv_obj_set_style_opa((lv_obj_t *)v, (lv_opa_t)opa, 0);
    });
    lv_anim_set_values(&a, 0, LV_OPA_COVER);
    lv_anim_set_time(&a, 200);
    lv_anim_start(&a);
}

// ---- Game ----

static void show_round(void);

static void on_choice(lv_event_t *e)
{
    if (locked) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);

    if (idx == correct_btn) {
        locked = true;
        lv_obj_set_style_bg_color(idx == 0 ? btn1 : btn2, lv_color_hex(0x27AE60), 0);
        pop_checkmark();
        score++;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d / %d", score, ROUNDS_PER_GAME);
        lv_label_set_text(score_lab, buf);
        lv_label_set_text(fb_lab, "Found it!");
        lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x27AE60), 0);

        adv_tmr = lv_timer_create([](lv_timer_t *tm) {
            lv_timer_del(tm);
            adv_tmr = NULL;
            qn++;
            if (qn >= ROUNDS_PER_GAME) {
                overlay = lv_obj_create(parent_content);
                lv_obj_remove_style_all(overlay);
                lv_obj_set_size(overlay, 480, 276);
                lv_obj_set_pos(overlay, 0, 0);
                lv_obj_set_style_bg_color(overlay, lv_color_hex(0xF0FFF0), 0);
                lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
                lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

                char stars[24];
                int n = (score * 5 + ROUNDS_PER_GAME / 2) / ROUNDS_PER_GAME;
                if (n < 1) n = 1;
                int sp = 0;
                for (int i = 0; i < n; i++)
                    sp += snprintf(stars + sp, sizeof(stars) - sp, "\xE2\x98\x85 ");
                stars[sp] = 0;

                lv_obj_t *sl = lv_label_create(overlay);
                lv_label_set_text(sl, stars);
                lv_obj_set_style_text_font(sl, &lv_font_montserrat_28, 0);
                lv_obj_set_style_text_color(sl, lv_color_hex(0xF1C40F), 0);
                lv_obj_align(sl, LV_ALIGN_TOP_MID, 0, 24);

                lv_obj_t *ml = lv_label_create(overlay);
                lv_label_set_text(ml, "Case Closed!");
                lv_obj_set_style_text_font(ml, &lv_font_montserrat_28, 0);
                lv_obj_set_style_text_color(ml, lv_color_hex(0x27AE60), 0);
                lv_obj_align(ml, LV_ALIGN_TOP_MID, 0, 70);

                lv_obj_t *ssl = lv_label_create(overlay);
                char smsg[48];
                snprintf(smsg, sizeof(smsg), "You found %d words!", score);
                lv_label_set_text(ssl, smsg);
                lv_obj_set_style_text_font(ssl, &lv_font_montserrat_16, 0);
                lv_obj_set_style_text_color(ssl, lv_color_hex(0x666666), 0);
                lv_obj_align(ssl, LV_ALIGN_TOP_MID, 0, 110);

                lv_obj_t *pb = lv_btn_create(overlay);
                lv_obj_set_style_bg_color(pb, lv_color_hex(0x3498DB), 0);
                lv_obj_set_size(pb, 200, 52);
                lv_obj_set_style_radius(pb, 26, 0);
                lv_obj_set_style_shadow_width(pb, 0, 0);
                lv_obj_align(pb, LV_ALIGN_TOP_MID, 0, 150);
                lv_obj_add_event_cb(pb, [](lv_event_t *) {
                    lv_obj_del(overlay);
                    overlay = NULL;
                    qn = 0; score = 0;
                    shuf(game_order, ROUNDS_PER_GAME);
                    show_round();
                }, LV_EVENT_CLICKED, NULL);

                lv_obj_t *pl = lv_label_create(pb);
                lv_label_set_text(pl, "Play Again");
                lv_obj_set_style_text_color(pl, lv_color_hex(0xFFFFFF), 0);
                lv_obj_set_style_text_font(pl, &lv_font_montserrat_20, 0);
                lv_obj_center(pl);
            } else {
                show_round();
            }
        }, 900, NULL);
        lv_timer_set_repeat_count(adv_tmr, 1);
    } else {
        lv_obj_t *btn = idx == 0 ? btn1 : btn2;
        shake_btn(btn);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xE74C3C), 0);
        lv_timer_t *rt = lv_timer_create([](lv_timer_t *tm) {
            lv_timer_del(tm);
            int btn_idx = (int)(intptr_t)lv_timer_get_user_data(tm);
            lv_obj_set_style_bg_color(btn_idx == 0 ? btn1 : btn2, lv_color_hex(0x3498DB), 0);
        }, 400, (void *)(intptr_t)idx);
        lv_timer_set_repeat_count(rt, 1);
    }
}

static void show_round(void)
{
    locked = false;

    const Round *r = &rounds[game_order[qn]];

    lv_obj_clean(pic_cont);
    lv_obj_set_style_bg_color(pic_cont, lv_color_hex(0xFFFFFF), 0);
    r->draw(pic_cont);

    int order[2] = {0, 1};
    shuf(order, 2);
    correct_btn = 0;
    if (order[0] == 0) { correct_btn = 0; } else { correct_btn = 1; }

    const char *t0 = order[0] == 0 ? r->correct : r->wrong;
    const char *t1 = order[1] == 0 ? r->correct : r->wrong;

    lv_label_set_text(btn1_lab, t0);
    lv_label_set_text(btn2_lab, t1);
    lv_obj_set_style_bg_color(btn1, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_bg_color(btn2, lv_color_hex(0x3498DB), 0);

    lv_label_set_text(fb_lab, "");

    char buf[16];
    snprintf(buf, sizeof(buf), "%d / %d", score, ROUNDS_PER_GAME);
    lv_label_set_text(score_lab, buf);
}

// ---- Entry ----

void app_word_spy_create(lv_obj_t *content)
{
    parent_content = content;
    qn = 0; score = 0; locked = false;
    overlay = NULL; adv_tmr = NULL;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(content, [](lv_event_t *) {
        if (adv_tmr) { lv_timer_del(adv_tmr); adv_tmr = NULL; }
    }, LV_EVENT_DELETE, NULL);

    score_lab = lv_label_create(content);
    lv_label_set_text(score_lab, "0 / 10");
    lv_obj_set_style_text_font(score_lab, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(score_lab, lv_color_hex(0x888888), 0);
    lv_obj_align(score_lab, LV_ALIGN_TOP_RIGHT, -8, 8);

    pic_cont = lv_obj_create(content);
    lv_obj_remove_style_all(pic_cont);
    lv_obj_set_size(pic_cont, 160, 120);
    lv_obj_align(pic_cont, LV_ALIGN_TOP_MID, 0, 32);
    lv_obj_set_style_bg_color(pic_cont, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(pic_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pic_cont, 12, 0);
    lv_obj_clear_flag(pic_cont, LV_OBJ_FLAG_SCROLLABLE);

    int bw = 150, bh = 66;
    int bx = (464 - bw * 2 - 20) / 2;

    btn1 = lv_btn_create(content);
    lv_obj_remove_style_all(btn1);
    lv_obj_set_size(btn1, bw, bh);
    lv_obj_set_pos(btn1, bx, 178);
    lv_obj_set_style_bg_color(btn1, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_bg_opa(btn1, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn1, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(btn1, 0, 0);
    lv_obj_add_event_cb(btn1, on_choice, LV_EVENT_CLICKED, (void *)0);

    btn1_lab = lv_label_create(btn1);
    lv_obj_set_style_text_color(btn1_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(btn1_lab, &lv_font_montserrat_28, 0);
    lv_obj_center(btn1_lab);

    btn2 = lv_btn_create(content);
    lv_obj_remove_style_all(btn2);
    lv_obj_set_size(btn2, bw, bh);
    lv_obj_set_pos(btn2, bx + bw + 20, 178);
    lv_obj_set_style_bg_color(btn2, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_bg_opa(btn2, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn2, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(btn2, 0, 0);
    lv_obj_add_event_cb(btn2, on_choice, LV_EVENT_CLICKED, (void *)1);

    btn2_lab = lv_label_create(btn2);
    lv_obj_set_style_text_color(btn2_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(btn2_lab, &lv_font_montserrat_28, 0);
    lv_obj_center(btn2_lab);

    fb_lab = lv_label_create(content);
    lv_label_set_text(fb_lab, "");
    lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_20, 0);
    lv_obj_align(fb_lab, LV_ALIGN_TOP_MID, 0, 250);

    for (int i = 0; i < ROUNDS_PER_GAME; i++)
        game_order[i] = i;
    shuf(game_order, ROUNDS_PER_GAME);
    show_round();
}
