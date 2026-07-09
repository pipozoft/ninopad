// Word Spy uses emoji artwork from OpenMoji (CC BY-SA 4.0)
// https://openmoji.org/  -  the open-source emoji and icon project
#include "app_word_spy.h"
#include "nino_colors.h"
#include <Arduino.h>
#include <string.h>

#define ROUNDS_PER_GAME 10

#define LVL3_COUNT 40
#define LVL4_COUNT 40
#define LVL5_COUNT 30

// ---- Round data ----

struct Round {
    const char *correct;
    const char *wrong;
    const char *img_path;
};

static const Round words_3lvl[LVL3_COUNT] = {
    // original 20
    {"dog", "jog", "S:/words/dog.bin"}, {"pig", "dig", "S:/words/pig.bin"},
    {"cat", "mat", "S:/words/cat.bin"}, {"fox", "box", "S:/words/fox.bin"},
    {"hen", "ten", "S:/words/hen.bin"}, {"bug", "beg", "S:/words/bug.bin"},
    {"mug", "rug", "S:/words/mug.bin"}, {"log", "fog", "S:/words/log.bin"},
    {"map", "rap", "S:/words/map.bin"}, {"hat", "pat", "S:/words/hat.bin"},
    {"bed", "fed", "S:/words/bed.bin"}, {"sun", "fun", "S:/words/sun.bin"},
    {"van", "man", "S:/words/van.bin"}, {"cup", "pup", "S:/words/cup.bin"},
    {"pot", "hot", "S:/words/pot.bin"}, {"bus", "pus", "S:/words/bus.bin"},
    {"net", "pet", "S:/words/net.bin"}, {"cap", "gap", "S:/words/cap.bin"},
    {"cow", "how", "S:/words/cow.bin"}, {"bee", "see", "S:/words/bee.bin"},
    // +20
    {"ant", "and", "S:/words/ant.bin"}, {"axe", "ask", "S:/words/axe.bin"},
    {"boy", "joy", "S:/words/boy.bin"}, {"car", "far", "S:/words/car.bin"},
    {"egg", "leg", "S:/words/egg.bin"}, {"fly", "fry", "S:/words/fly.bin"},
    {"jam", "ham", "S:/words/jam.bin"}, {"jar", "bar", "S:/words/jar.bin"},
    {"lip", "tip", "S:/words/lip.bin"}, {"mop", "hop", "S:/words/mop.bin"},
    {"nut", "but", "S:/words/nut.bin"}, {"pan", "can", "S:/words/pan.bin"},
    {"pie", "tie", "S:/words/pie.bin"}, {"pin", "win", "S:/words/pin.bin"},
    {"rat", "sat", "S:/words/rat.bin"}, {"row", "bow", "S:/words/row.bin"},
    {"run", "fun", "S:/words/run.bin"}, {"sea", "see", "S:/words/sea.bin"},
    {"tap", "nap", "S:/words/tap.bin"}, {"top", "mop", "S:/words/top.bin"},
};

static const Round words_4lvl[LVL4_COUNT] = {
    // original 20
    {"fish","dish","S:/words/fish.bin"}, {"cake","bake","S:/words/cake.bin"},
    {"book","look","S:/words/book.bin"}, {"bird","word","S:/words/bird.bin"},
    {"lamp","camp","S:/words/lamp.bin"}, {"milk","silk","S:/words/milk.bin"},
    {"nest","rest","S:/words/nest.bin"}, {"ring","sing","S:/words/ring.bin"},
    {"sail","tail","S:/words/sail.bin"}, {"bell","sell","S:/words/bell.bin"},
    {"door","floor","S:/words/door.bin"},{"hand","sand","S:/words/hand.bin"},
    {"kite","bite","S:/words/kite.bin"}, {"sock","lock","S:/words/sock.bin"},
    {"star","scar","S:/words/star.bin"}, {"moon","noon","S:/words/moon.bin"},
    {"rain","pain","S:/words/rain.bin"}, {"tree","free","S:/words/tree.bin"},
    {"wolf","woof","S:/words/wolf.bin"}, {"frog","fog", "S:/words/frog.bin"},
    // +20
    {"ball","call","S:/words/ball.bin"}, {"barn","warn","S:/words/barn.bin"},
    {"bath","math","S:/words/bath.bin"}, {"bear","fear","S:/words/bear.bin"},
    {"bike","like","S:/words/bike.bin"}, {"crab","grab","S:/words/crab.bin"},
    {"duck","luck","S:/words/duck.bin"}, {"fire","wire","S:/words/fire.bin"},
    {"food","mood","S:/words/food.bin"}, {"gift","lift","S:/words/gift.bin"},
    {"goat","coat","S:/words/goat.bin"}, {"gold","mold","S:/words/gold.bin"},
    {"king","wing","S:/words/king.bin"}, {"lion","line","S:/words/lion.bin"},
    {"lock","dock","S:/words/lock.bin"}, {"nose","rose","S:/words/nose.bin"},
    {"owl", "foul","S:/words/owl.bin"},  {"pear","tear","S:/words/pear.bin"},
    {"ship","chip","S:/words/ship.bin"}, {"snow","blow","S:/words/snow.bin"},
};

static const Round words_5lvl[LVL5_COUNT] = {
    // original 16
    {"house","mouse","S:/words/house.bin"}, {"bread","thread","S:/words/bread.bin"},
    {"candy","handy","S:/words/candy.bin"}, {"clock","block","S:/words/clock.bin"},
    {"crown","brown","S:/words/crown.bin"}, {"grape","shape","S:/words/grape.bin"},
    {"heart","smart","S:/words/heart.bin"}, {"lemon","demon","S:/words/lemon.bin"},
    {"pizza","pita","S:/words/pizza.bin"}, {"sheep","jeep", "S:/words/sheep.bin"},
    {"tiger","lion", "S:/words/tiger.bin"}, {"train","plane","S:/words/train.bin"},
    {"ghost","toast","S:/words/ghost.bin"}, {"mouse","house","S:/words/mouse.bin"},
    {"ocean","motion","S:/words/ocean.bin"},{"onion","union","S:/words/onion.bin"},
    // +14
    {"apple","maple","S:/words/apple.bin"}, {"beach","reach","S:/words/beach.bin"},
    {"brain","crane","S:/words/brain.bin"}, {"chair","chain","S:/words/chair.bin"},
    {"cloud","loud", "S:/words/cloud.bin"}, {"happy","sappy","S:/words/happy.bin"},
    {"horse","worse","S:/words/horse.bin"}, {"phone","tone", "S:/words/phone.bin"},
    {"shell","bell", "S:/words/shell.bin"}, {"shirt","skirt","S:/words/shirt.bin"},
    {"skate","gate", "S:/words/skate.bin"}, {"snake","snack","S:/words/snake.bin"},
    {"whale","while","S:/words/whale.bin"}, {"wheat","cheat","S:/words/wheat.bin"},
};

// ---- State ----

static const Round *cur_bank;
static int cur_count;
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

#define MAX_WORDS 40

// ---- Helpers ----

static void shuf(int a[], int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = random(i + 1);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

static void pick_rounds(void)
{
    int pool[MAX_WORDS];
    for (int i = 0; i < cur_count; i++)
        pool[i] = i;
    shuf(pool, cur_count);
    for (int i = 0; i < ROUNDS_PER_GAME; i++)
        game_order[i] = pool[i];
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

                char stars[16];
                int n = (score * 5 + ROUNDS_PER_GAME / 2) / ROUNDS_PER_GAME;
                if (n < 1) n = 1;
                snprintf(stars, sizeof(stars), "%.*s", n, "***");

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
                    pick_rounds();
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

    const Round *r = &cur_bank[game_order[qn]];

    lv_obj_clean(pic_cont);
    lv_obj_set_style_bg_color(pic_cont, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *img = lv_image_create(pic_cont);
    lv_image_set_src(img, r->img_path);
    lv_obj_center(img);

    int order[2] = {0, 1};
    shuf(order, 2);
    correct_btn = order[0] == 0 ? 0 : 1;

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

static void start_level(int level)
{
    if (level == 3)      { cur_bank = words_3lvl; cur_count = LVL3_COUNT; }
    else if (level == 4) { cur_bank = words_4lvl; cur_count = LVL4_COUNT; }
    else                 { cur_bank = words_5lvl; cur_count = LVL5_COUNT; }
    qn = 0; score = 0;
    pick_rounds();
    lv_obj_del(overlay);
    overlay = NULL;
    show_round();
}

// ---- Level picker ----

static void show_level_picker(void)
{
    overlay = lv_obj_create(parent_content);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, 480, 276);
    lv_obj_align(overlay, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_pos(overlay, 0, -8);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tl = lv_label_create(overlay);
    lv_label_set_text(tl, "Pick your level!");
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(tl, lv_color_hex(0x444444), 0);
    lv_obj_align(tl, LV_ALIGN_TOP_MID, 0, 24);

    static const char *labs[] = {"3 Letters", "4 Letters", "5 Letters"};
    static const uint32_t colors[] = {0x2ECC71, 0x3498DB, 0xE74C3C};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(overlay);
        lv_obj_set_size(btn, 280, 52);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_radius(btn, 26, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 80 + i * 64);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            start_level((int)(intptr_t)lv_event_get_user_data(e));
        }, LV_EVENT_CLICKED, (void *)(intptr_t)(i == 0 ? 3 : i == 1 ? 4 : 5));

        lv_obj_t *ll = lv_label_create(btn);
        lv_label_set_text(ll, labs[i]);
        lv_obj_set_style_text_color(ll, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(ll, &lv_font_montserrat_20, 0);
        lv_obj_center(ll);
    }
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
    lv_obj_set_size(pic_cont, 128, 96);
    lv_obj_align(pic_cont, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_set_style_bg_color(pic_cont, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(pic_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pic_cont, 12, 0);
    lv_obj_clear_flag(pic_cont, LV_OBJ_FLAG_SCROLLABLE);

    int bw = 150, bh = 66;
    int bx = (464 - bw * 2 - 20) / 2;

    btn1 = lv_btn_create(content);
    lv_obj_remove_style_all(btn1);
    lv_obj_set_size(btn1, bw, bh);
    lv_obj_set_pos(btn1, bx, 130);
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
    lv_obj_set_pos(btn2, bx + bw + 20, 130);
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
    lv_obj_align(fb_lab, LV_ALIGN_TOP_MID, 0, 224);

    show_level_picker();
}
