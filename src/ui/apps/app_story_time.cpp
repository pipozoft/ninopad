#include "app_story_time.h"
#include "nino_colors.h"
#include <Arduino.h>
#include <string.h>

#define Q_PER_GAME 10
#define LEVELS      3

enum Illus {
    ILL_DOG, ILL_CAT, ILL_BIRD, ILL_FISH, ILL_RABBIT, ILL_SUN,
    ILL_STAR, ILL_TREE, ILL_FLOWER, ILL_BOOK, ILL_BALL, ILL_CAR,
    ILL_HOUSE, ILL_BED, ILL_WATER, ILL_HAT, ILL_CAKE, ILL_ICE,
    ILL_COOKIE, ILL_BANANA, ILL_COUNT
};

struct Question {
    const char *s;
    const char *c0, *c1, *c2;
    uint8_t correct;
    uint8_t ill;
};

// ---- Level 1: Beginner ----
static const Question L1[] = {
    {"The cat ___ jump.",  "can",  "like",  "not",  0, ILL_CAT},
    {"I ___ ice cream.",   "like", "not",   "see",  0, ILL_ICE},
    {"I ___ a dog.",       "see",  "run",   "like", 0, ILL_DOG},
    {"The bird can ___.",  "fly",  "jump",  "eat",  0, ILL_BIRD},
    {"A fish can ___.",    "swim", "fly",   "jump", 0, ILL_FISH},
    {"The sun is ___.",    "hot",  "little","blue", 0, ILL_SUN},
    {"I like to ___.",     "play", "run",   "see",  0, ILL_BALL},
    {"My ___ is red.",     "ball", "cat",   "book", 0, ILL_BALL},
    {"I see a ___.",       "car",  "dog",   "tree", 0, ILL_CAR},
    {"Look at the ___.",   "flower","bird", "fish", 0, ILL_FLOWER},
    {"The star is ___.",   "bright","big",  "small",0, ILL_STAR},
    {"The ___ is big.",    "tree", "cat",   "ball", 0, ILL_TREE},
    {"I have a ___.",      "book", "ball",  "cat",  0, ILL_BOOK},
    {"The car is ___.",    "red",  "fast",  "hot",  0, ILL_CAR},
    {"We ___ to the park.","go",   "run",   "see",  0, ILL_TREE},
    {"I ___ a bird.",      "see",  "like",  "have", 0, ILL_BIRD},
    {"My cat is ___.",     "little","big",  "red",  0, ILL_CAT},
};

// ---- Level 2: Intermediate ----
static const Question L2[] = {
    {"She ___ a red ball.",     "has",  "like",  "sees",  0, ILL_BALL},
    {"The dog can ___ fast.",   "run",  "eat",   "sleep", 0, ILL_DOG},
    {"He ___ to read.",         "likes","goes",  "sees",  0, ILL_BOOK},
    {"The fish lives in ___.",  "water","a bowl","the sea",0, ILL_FISH},
    {"We ___ to the store.",    "go",   "run",   "see",   0, ILL_CAR},
    {"The rabbit ___ fast.",    "hops", "runs",  "eats",  0, ILL_RABBIT},
    {"Dad makes ___ for us.",   "pizza","cake",  "cookies",0, ILL_CAKE},
    {"She ___ on her bed.",     "sleeps","reads","jumps", 0, ILL_BED},
    {"He ___ with his ball.",   "plays","runs",  "eats",  0, ILL_BALL},
    {"I ___ my teeth.",         "brush","see",   "like",  0, ILL_WATER},
    {"The sun is very ___.",    "bright","big",  "hot",   0, ILL_SUN},
    {"We ___ cookies.",         "eat",  "make",  "like",  0, ILL_COOKIE},
    {"The cat ___ on the bed.", "sleeps","jumps","sits",  0, ILL_CAT},
    {"I ___ ice cream.",        "like", "want",  "see",   0, ILL_ICE},
    {"We live in a ___.",       "house","school","store", 0, ILL_HOUSE},
    {"The bird ___ in the tree.","sits", "flies", "sings", 0, ILL_BIRD},
    {"Mom ___ a flower.",       "picks","sees",  "likes", 0, ILL_FLOWER},
};

// ---- Level 3: Advanced ----
static const Question L3[] = {
    {"The bird can fly ___.",     "high", "fast", "away",   0, ILL_BIRD},
    {"I brush my ___ every day.", "teeth","hair", "cat",    0, ILL_WATER},
    {"She goes to ___ every day.","school","bed", "home",   0, ILL_BOOK},
    {"A fish lives in the ___.",  "ocean","pond", "bowl",   0, ILL_FISH},
    {"The rabbit ___ very fast.", "hops", "runs", "jumps",  0, ILL_RABBIT},
    {"We ___ milk with dinner.",  "drink","eat",  "have",   0, ILL_WATER},
    {"She puts on her ___.",      "shoes","hat",  "coat",   0, ILL_HAT},
    {"The stars ___ at night.",   "shine","come", "move",   0, ILL_STAR},
    {"My hat is on the ___.",     "table","bed",  "floor",  0, ILL_HAT},
    {"The cat sits on the ___.",  "mat",  "chair","bed",    0, ILL_CAT},
    {"We ___ cake at the party.", "eat",  "make", "like",   0, ILL_CAKE},
    {"A bird ___ in the sky.",    "flies","sings","sits",   0, ILL_BIRD},
    {"I ___ my mom a flower.",    "give", "see",  "pick",   0, ILL_FLOWER},
    {"The dog ___ in the park.",  "runs", "sleeps","eats",  0, ILL_DOG},
    {"She ___ a story to us.",    "reads","tells","writes", 0, ILL_BOOK},
    {"We ___ on the swings.",     "play", "sit",  "jump",   0, ILL_BALL},
};

static const Question *levels[LEVELS] = {L1, L2, L3};
static const int lq_sizes[LEVELS] = {
    sizeof(L1)/sizeof(Question),
    sizeof(L2)/sizeof(Question),
    sizeof(L3)/sizeof(Question)
};

// ---- State ----
static lv_obj_t *parent_content;
static int level;
static int game_qs[Q_PER_GAME];
static int qn, score;
static bool locked;
static int correct_btn;

static lv_obj_t *sent_before, *sent_blank, *sent_after;
static lv_obj_t *btn_cont;
static lv_obj_t *ill_cont;
static lv_obj_t *fb_lab, *score_lab;
static lv_obj_t *overlay;
static lv_obj_t *next_btn;
static lv_timer_t *rst_tmr;

// ---- Helpers ----

static void shuf(int a[], int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = random(i + 1);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

static void parse_sentence(const char *s, char *buf, int sz,
                           const char **before, const char **after)
{
    const char *p = strstr(s, "___");
    if (!p) { *before = s; *after = ""; return; }
    int pre_len = p - s;
    strncpy(buf, s, pre_len);
    buf[pre_len] = 0;
    *before = buf;
    *after = p + 3;
}

// ---- Drawing primitives ----

static lv_obj_t *dr_circ(lv_obj_t *p, int x, int y, int r, uint32_t c)
{
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, r*2, r*2);
    lv_obj_set_pos(o, x - r, y - r);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(c), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_obj_t *dr_rect(lv_obj_t *p, int x, int y, int w, int h, uint32_t c)
{
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_bg_color(o, lv_color_hex(c), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, 3, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

// ---- Illustrations ----

static void draw_illus(int id)
{
    lv_obj_clean(ill_cont);
    lv_obj_t *p = ill_cont;

    switch (id) {
    case ILL_DOG:
        dr_circ(p, 24,16,12,0x8B4513);
        dr_rect(p, 36,18,30,18,0x8B4513);
        dr_rect(p, 38,36,4,12,0x8B4513);
        dr_rect(p, 46,36,4,12,0x8B4513);
        dr_rect(p, 54,36,4,12,0x8B4513);
        dr_rect(p, 62,36,4,12,0x8B4513);
        dr_rect(p, 66,16,3,14,0x8B4513);
        dr_circ(p, 22,14,3,0x000000);
        break;
    case ILL_CAT:
        dr_circ(p, 24,16,12,0xFF8C00);
        dr_rect(p, 16,5,5,8,0xFF8C00);
        dr_rect(p, 27,5,5,8,0xFF8C00);
        dr_rect(p, 36,18,28,16,0xFF8C00);
        dr_rect(p, 64,16,3,16,0xFF8C00);
        dr_circ(p, 22,14,3,0x000000);
        break;
    case ILL_BIRD:
        dr_rect(p, 32,16,28,14,0xFFD700);
        dr_rect(p, 32,16,28,14,0xFFD700);
        lv_obj_set_style_radius(dr_circ(p, 18,12,8,0xFFD700), LV_RADIUS_CIRCLE, 0);
        dr_rect(p, 14,18,6,8,0xFF8C00);
        dr_rect(p, 40,14,16,8,0xFFEB3B);
        dr_rect(p, 60,16,10,10,0xFFD700);
        dr_circ(p, 16,10,2,0x000000);
        break;
    case ILL_FISH:
        dr_rect(p, 18,16,34,16,0x4A90D9);
        lv_obj_set_style_radius(dr_circ(p, 46,20,8,0x4A90D9), 0, 1);
        dr_rect(p, 52,14,14,20,0x4A90D9);
        dr_circ(p, 22,18,3,0xFFFFFF);
        dr_circ(p, 22,18,2,0x000000);
        dr_circ(p, 14,8,3,0x87CEEB);
        dr_circ(p, 20,4,2,0x87CEEB);
        dr_circ(p, 10,12,2,0x87CEEB);
        break;
    case ILL_RABBIT:
        dr_circ(p, 24,14,10,0xFFFFFF);
        dr_rect(p, 18,0,5,16,0xFFFFFF);
        dr_rect(p, 25,2,5,14,0xFFFFFF);
        dr_rect(p, 36,16,24,16,0xFFFFFF);
        dr_rect(p, 38,32,4,12,0xFFFFFF);
        dr_rect(p, 48,32,4,12,0xFFFFFF);
        dr_circ(p, 22,12,2,0x000000);
        dr_circ(p, 26,12,2,0xFFB6C1);
        break;
    case ILL_SUN:
        dr_rect(p, 35,4,4,10,0xFFD700);
        dr_rect(p, 35,46,4,10,0xFFD700);
        dr_rect(p, 4,35,10,4,0xFFD700);
        dr_rect(p, 46,35,10,4,0xFFD700);
        dr_rect(p, 10,10,4,8,0xFFD700);
        dr_rect(p, 46,10,4,8,0xFFD700);
        dr_rect(p, 10,42,4,8,0xFFD700);
        dr_rect(p, 46,42,4,8,0xFFD700);
        dr_circ(p, 28,28,16,0xFFD700);
        break;
    case ILL_STAR:
        dr_circ(p, 36,24,18,0xFFD700);
        dr_circ(p, 36,24,12,0xFFF176);
        dr_circ(p, 36,24,6,0xFFFFFF);
        break;
    case ILL_TREE:
        dr_rect(p, 36,32,6,22,0x8B4513);
        dr_circ(p, 39,20,16,0x2ECC71);
        dr_circ(p, 28,24,12,0x27AE60);
        dr_circ(p, 50,24,12,0x27AE60);
        break;
    case ILL_FLOWER:
        dr_rect(p, 34,32,4,20,0x2ECC71);
        dr_circ(p, 26,24,8,0xFF69B4);
        dr_circ(p, 42,24,8,0xFF69B4);
        dr_circ(p, 34,16,8,0xFF69B4);
        dr_circ(p, 18,28,8,0xFFB6C1);
        dr_circ(p, 50,28,8,0xFFB6C1);
        dr_circ(p, 34,24,6,0xFFD700);
        break;
    case ILL_BOOK:
        dr_rect(p, 12,8,48,38,0x3498DB);
        dr_rect(p, 16,12,40,34,0xFFFFFF);
        dr_rect(p, 16,16,36,3,0xCCCCCC);
        dr_rect(p, 16,22,36,3,0xCCCCCC);
        dr_rect(p, 16,28,36,3,0xCCCCCC);
        dr_rect(p, 16,34,36,3,0xCCCCCC);
        break;
    case ILL_BALL:
        dr_circ(p, 36,26,16,0xE74C3C);
        dr_rect(p, 34,10,4,32,0xFFFFFF);
        dr_rect(p, 20,24,32,4,0xFFFFFF);
        break;
    case ILL_CAR:
        dr_rect(p, 12,14,42,18,0xE74C3C);
        dr_rect(p, 24,6,22,12,0x3498DB);
        dr_circ(p, 20,32,6,0x333333);
        dr_circ(p, 46,32,6,0x333333);
        dr_circ(p, 20,32,3,0xFFFFFF);
        dr_circ(p, 46,32,3,0xFFFFFF);
        dr_rect(p, 14,16,8,6,0x87CEEB);
        dr_rect(p, 42,16,8,6,0x87CEEB);
        break;
    case ILL_HOUSE:
        dr_rect(p, 16,18,40,26,0xF5DEB3);
        dr_rect(p, 16,6,40,14,0xE74C3C);
        dr_rect(p, 32,28,10,16,0x8B4513);
        dr_circ(p, 37,36,2,0xFFD700);
        dr_rect(p, 20,22,10,8,0x87CEEB);
        dr_rect(p, 22,24,6,6,0xFFFFFF);
        break;
    case ILL_BED:
        dr_rect(p, 14,22,46,14,0xFFFFFF);
        dr_rect(p, 14,8,46,16,0x3498DB);
        dr_rect(p, 16,12,14,10,0xFFFFFF);
        dr_rect(p, 14,36,4,10,0x8B4513);
        dr_rect(p, 56,36,4,10,0x8B4513);
        dr_circ(p, 34,10,3,0xFF69B4);
        dr_circ(p, 42,10,3,0xFF69B4);
        break;
    case ILL_WATER:
        dr_rect(p, 12,10,50,40,0x4A90D9);
        dr_rect(p, 16,14,42,6,0x87CEEB);
        dr_rect(p, 16,24,42,6,0x87CEEB);
        dr_rect(p, 16,34,42,6,0x87CEEB);
        dr_circ(p, 14,8,4,0xFFFFFF);
        dr_circ(p, 8,18,3,0xFFFFFF);
        dr_circ(p, 56,12,3,0xFFFFFF);
        break;
    case ILL_HAT:
        dr_rect(p, 16,24,44,6,0x8B4513);
        dr_rect(p, 24,6,24,20,0xE74C3C);
        dr_rect(p, 24,6,24,4,0xFF69B4);
        dr_circ(p, 36,28,2,0xFFFFFF);
        break;
    case ILL_CAKE:
        dr_rect(p, 18,22,38,18,0xFF69B4);
        dr_rect(p, 22,14,30,10,0xFFB6C1);
        dr_rect(p, 36,7,4,10,0xFFFFFF);
        dr_circ(p, 38,6,3,0xFFD700);
        dr_rect(p, 14,40,46,4,0x8B4513);
        dr_circ(p, 26,18,3,0xE74C3C);
        dr_circ(p, 48,18,3,0xE74C3C);
        break;
    case ILL_ICE:
        dr_rect(p, 20,28,32,20,0xD2B48C);
        dr_rect(p, 24,32,24,14,0xC4A882);
        dr_circ(p, 36,16,12,0xFFB6C1);
        dr_circ(p, 28,20,8,0xFF69B4);
        dr_circ(p, 44,18,7,0xFFFFFF);
        break;
    case ILL_COOKIE:
        dr_circ(p, 36,26,18,0xD2B48C);
        dr_circ(p, 28,20,4,0x8B4513);
        dr_circ(p, 44,20,4,0x8B4513);
        dr_circ(p, 36,16,3,0x8B4513);
        dr_circ(p, 36,30,4,0x8B4513);
        dr_circ(p, 28,32,3,0x8B4513);
        dr_circ(p, 44,32,3,0x8B4513);
        break;
    case ILL_BANANA:
        dr_rect(p, 14,18,40,16,0xFFD700);
        lv_obj_set_style_radius(dr_circ(p, 34,24,12,0xFFD700), 0, 2);
        dr_circ(p, 12,22,4,0x8B4513);
        break;
    default: break;
    }
}

// ---- Shake ----

static void shake_obj(lv_obj_t *obj)
{
    lv_coord_t ox = lv_obj_get_x(obj);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        lv_obj_set_x((lv_obj_t *)var, v);
    });
    lv_anim_set_values(&a, ox - 6, ox + 6);
    lv_anim_set_time(&a, 40);
    lv_anim_set_reverse_duration(&a, 40);
    lv_anim_set_repeat_count(&a, 2);
    lv_anim_set_ready_cb(&a, [](lv_anim_t *a) {
        lv_obj_set_x((lv_obj_t *)a->var, (intptr_t)lv_anim_get_user_data(a));
    });
    lv_anim_set_user_data(&a, (void *)(intptr_t)ox);
    lv_anim_start(&a);
}

// ---- Game ----

static void show_q(void);

static void on_choice(lv_event_t *e)
{
    if (locked) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);

    if (idx == correct_btn) {
        locked = true;
        const Question *q = &levels[level][game_qs[qn]];
        const char *word = ((const char *[]){q->c0, q->c1, q->c2})[q->correct];

        lv_label_set_text(sent_blank, word);
        lv_obj_set_style_text_color(sent_blank, lv_color_hex(0x27AE60), 0);
        lv_obj_set_style_text_font(sent_blank, &lv_font_montserrat_28, 0);

        lv_obj_add_flag(btn_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ill_cont, LV_OBJ_FLAG_HIDDEN);
        draw_illus(q->ill);

        score++;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d / %d", score, Q_PER_GAME);
        lv_label_set_text(score_lab, buf);

        lv_label_set_text(fb_lab, "Great Reading!");
        lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x27AE60), 0);
        lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_20, 0);

        lv_obj_clear_flag(next_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        shake_obj(lv_obj_get_child(btn_cont, idx));
        lv_obj_set_style_bg_color(lv_obj_get_child(btn_cont, idx), lv_color_hex(0xE74C3C), 0);

        if (rst_tmr) lv_timer_del(rst_tmr);
        rst_tmr = lv_timer_create([](lv_timer_t *tm) {
            lv_timer_del(tm);
            rst_tmr = NULL;
            int idx = (int)(intptr_t)lv_timer_get_user_data(tm);
            lv_obj_set_style_bg_color(lv_obj_get_child(btn_cont, idx), lv_color_hex(0x3498DB), 0);
        }, 400, (void *)(intptr_t)idx);
        lv_timer_set_repeat_count(rst_tmr, 1);
    }
}

static void show_q(void)
{
    locked = false;
    correct_btn = 0;

    const Question *q = &levels[level][game_qs[qn]];

    const char *before, *after;
    static char pre_buf[48];
    parse_sentence(q->s, pre_buf, sizeof(pre_buf), &before, &after);

    lv_label_set_text(sent_before, before);
    lv_label_set_text(sent_blank, "____");
    lv_obj_set_style_text_color(sent_blank, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(sent_blank, &lv_font_montserrat_28, 0);
    lv_label_set_text(sent_after, after);

    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ill_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clean(btn_cont);

    int order[3] = {0, 1, 2};
    shuf(order, 3);
    for (int i = 0; i < 3; i++)
        if (order[i] == q->correct) { correct_btn = i; break; }

    int bw = 130, bh = 60;
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(btn_cont);
        lv_obj_set_size(btn, bw, bh);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x3498DB), 0);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_24, 0);
        lv_obj_add_event_cb(btn, on_choice, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t *lab = lv_label_create(btn);
        lv_label_set_text(lab, ((const char *[]){q->c0, q->c1, q->c2})[order[i]]);
        lv_obj_center(lab);
    }

    lv_label_set_text(fb_lab, "");
    lv_obj_add_flag(next_btn, LV_OBJ_FLAG_HIDDEN);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d / %d", score, Q_PER_GAME);
    lv_label_set_text(score_lab, buf);
}

// ---- Flow ----

static void next_q(void)
{
    qn++;
    if (qn >= Q_PER_GAME) {
        overlay = lv_obj_create(parent_content);
        lv_obj_remove_style_all(overlay);
        lv_obj_set_size(overlay, 480, 276);
        lv_obj_set_pos(overlay, 0, 0);
        lv_obj_set_style_bg_color(overlay, lv_color_hex(0xF0FFF0), 0);
        lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
        lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

        char stars[16];
        int n = (score * 5 + Q_PER_GAME / 2) / Q_PER_GAME;
        if (n < 1) n = 1;
        int p = 0;
        for (int i = 0; i < n; i++)
            p += snprintf(stars + p, sizeof(stars) - p, "\xE2\x98\x85 ");
        stars[p] = 0;

        lv_obj_t *sl = lv_label_create(overlay);
        lv_label_set_text(sl, stars);
        lv_obj_set_style_text_font(sl, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(sl, lv_color_hex(0xF1C40F), 0);
        lv_obj_align(sl, LV_ALIGN_TOP_MID, 0, 24);

        char msg[40];
        snprintf(msg, sizeof(msg), "You read %d sentences!", Q_PER_GAME);
        lv_obj_t *ml = lv_label_create(overlay);
        lv_label_set_text(ml, msg);
        lv_obj_set_style_text_font(ml, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(ml, lv_color_hex(0x444444), 0);
        lv_obj_set_style_text_align(ml, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(ml, LV_ALIGN_TOP_MID, 0, 80);

        lv_obj_t *pb = lv_btn_create(overlay);
        lv_obj_set_style_bg_color(pb, lv_color_hex(0x27AE60), 0);
        lv_obj_set_size(pb, 200, 52);
        lv_obj_set_style_radius(pb, 26, 0);
        lv_obj_set_style_shadow_width(pb, 0, 0);
        lv_obj_align(pb, LV_ALIGN_TOP_MID, 0, 140);

        lv_obj_add_event_cb(pb, [](lv_event_t *) {
            lv_obj_del(overlay);
            overlay = NULL;
            qn = 0; score = 0;
            shuf(game_qs, Q_PER_GAME);
            show_q();
        }, LV_EVENT_CLICKED, NULL);

        lv_obj_t *pl = lv_label_create(pb);
        lv_label_set_text(pl, "Play Again");
        lv_obj_set_style_text_color(pl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(pl, &lv_font_montserrat_20, 0);
        lv_obj_center(pl);
        return;
    }
    show_q();
}

static void on_next(lv_event_t *)
{
    if (!lv_obj_has_flag(next_btn, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(next_btn, LV_OBJ_FLAG_HIDDEN);
        next_q();
    }
}

// ---- Level picker ----

static void pick_level(int lvl)
{
    level = lvl;
    qn = 0; score = 0;
    int nq = lq_sizes[level];
    for (int i = 0; i < Q_PER_GAME; i++)
        game_qs[i] = i < nq ? i : nq - 1;
    shuf(game_qs, Q_PER_GAME);
    lv_obj_del(overlay);
    overlay = NULL;
    show_q();
}

static void show_level_picker(void)
{
    overlay = lv_obj_create(parent_content);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, 480, 276);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tl = lv_label_create(overlay);
    lv_label_set_text(tl, "Pick a Level");
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(tl, lv_color_hex(0x444444), 0);
    lv_obj_align(tl, LV_ALIGN_TOP_MID, 0, 24);

    static const char *labs[] = {"Beginner", "Intermediate", "Advanced"};
    static const uint32_t colors[] = {0x2ECC71, 0x3498DB, 0xE74C3C};

    for (int i = 0; i < LEVELS; i++) {
        lv_obj_t *btn = lv_btn_create(overlay);
        lv_obj_set_size(btn, 280, 52);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_radius(btn, 26, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 80 + i * 64);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            pick_level((int)(intptr_t)lv_event_get_user_data(e));
        }, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *ll = lv_label_create(btn);
        lv_label_set_text(ll, labs[i]);
        lv_obj_set_style_text_color(ll, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(ll, &lv_font_montserrat_20, 0);
        lv_obj_center(ll);
    }
}

// ---- Entry ----

void app_story_time_create(lv_obj_t *content)
{
    parent_content = content;
    qn = 0; score = 0; locked = false;
    level = 1;
    overlay = NULL;
    rst_tmr = NULL;
    next_btn = NULL;
    correct_btn = 0;

    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(content, [](lv_event_t *) {
        if (rst_tmr) { lv_timer_del(rst_tmr); rst_tmr = NULL; }
    }, LV_EVENT_DELETE, NULL);

    score_lab = lv_label_create(content);
    lv_label_set_text(score_lab, "0 / 10");
    lv_obj_set_style_text_font(score_lab, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(score_lab, lv_color_hex(0x888888), 0);
    lv_obj_align(score_lab, LV_ALIGN_TOP_RIGHT, -8, 8);

    lv_obj_t *sent_cont = lv_obj_create(content);
    lv_obj_remove_style_all(sent_cont);
    lv_obj_set_size(sent_cont, 460, 50);
    lv_obj_align(sent_cont, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_flex_flow(sent_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sent_cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(sent_cont, LV_OBJ_FLAG_SCROLLABLE);

    sent_before = lv_label_create(sent_cont);
    lv_obj_set_style_text_font(sent_before, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(sent_before, lv_color_hex(0x444444), 0);

    sent_blank = lv_label_create(sent_cont);
    lv_obj_set_style_text_font(sent_blank, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(sent_blank, lv_color_hex(0x888888), 0);

    sent_after = lv_label_create(sent_cont);
    lv_obj_set_style_text_font(sent_after, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(sent_after, lv_color_hex(0x444444), 0);

    btn_cont = lv_obj_create(content);
    lv_obj_remove_style_all(btn_cont);
    lv_obj_set_size(btn_cont, 460, 100);
    lv_obj_align(btn_cont, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);

    ill_cont = lv_obj_create(content);
    lv_obj_remove_style_all(ill_cont);
    lv_obj_set_size(ill_cont, 140, 90);
    lv_obj_align(ill_cont, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_add_flag(ill_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ill_cont, LV_OBJ_FLAG_SCROLLABLE);

    fb_lab = lv_label_create(content);
    lv_label_set_text(fb_lab, "");
    lv_obj_set_style_text_color(fb_lab, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(fb_lab, &lv_font_montserrat_20, 0);
    lv_obj_align(fb_lab, LV_ALIGN_TOP_MID, 0, 200);

    next_btn = lv_btn_create(content);
    lv_obj_set_size(next_btn, 160, 44);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_radius(next_btn, 22, 0);
    lv_obj_set_style_shadow_width(next_btn, 0, 0);
    lv_obj_align(next_btn, LV_ALIGN_TOP_MID, 0, 185);
    lv_obj_add_flag(next_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(next_btn, on_next, LV_EVENT_CLICKED, NULL);

    lv_obj_t *nl = lv_label_create(next_btn);
    lv_label_set_text(nl, "Next " LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(nl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_20, 0);
    lv_obj_center(nl);

    show_level_picker();
}
