#include "app_word_spy.h"
#include "storage/storage.h"
#include "utils/sd_utils.h"
#include "nino_colors.h"
#include <Arduino.h>

#define WORD_AREA_H  140
#define BTN_AREA_H   60

// ---- Picture drawing helpers (LVGL primitives, no canvas needed) ----

static lv_obj_t *add_fill_circle(lv_obj_t *parent, int x, int y, int r, lv_color_t c)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_size(o, r * 2, r * 2);
    lv_obj_set_pos(o, x - r, y - r);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static lv_obj_t *add_fill_rect(lv_obj_t *parent, int x, int y, int w, int h, lv_color_t c)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, 0, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

static void draw_picture_log(lv_obj_t *parent)
{
    add_fill_rect(parent, 20, 40, 100, 30, lv_color_hex(0x8B4513));
    add_fill_rect(parent, 30, 30, 10, 50, lv_color_hex(0x6B3410));
    add_fill_rect(parent, 80, 25, 10, 55, lv_color_hex(0x6B3410));
    add_fill_rect(parent, 55, 30, 8, 50, lv_color_hex(0x6B3410));
}

static void draw_picture_mug(lv_obj_t *parent)
{
    add_fill_rect(parent, 40, 30, 60, 60, lv_color_hex(0xDDDDDD));
    add_fill_rect(parent, 100, 45, 20, 15, lv_color_hex(0xDDDDDD));
    add_fill_rect(parent, 45, 35, 50, 50, lv_color_hex(0xCC9944));
}

static void draw_picture_dog(lv_obj_t *parent)
{
    add_fill_circle(parent, 70, 45, 25, lv_color_hex(0xD4A574));
    add_fill_circle(parent, 70, 75, 20, lv_color_hex(0xD4A574));
    add_fill_circle(parent, 60, 37, 6, lv_color_hex(0x222222));
    add_fill_circle(parent, 80, 37, 6, lv_color_hex(0x222222));
    add_fill_circle(parent, 70, 50, 5, lv_color_hex(0x222222));
}

static void draw_picture_pig(lv_obj_t *parent)
{
    add_fill_circle(parent, 70, 65, 30, lv_color_hex(0xFFB6C1));
    add_fill_circle(parent, 70, 55, 8, lv_color_hex(0xFF8CA0));
    add_fill_circle(parent, 55, 50, 3, lv_color_hex(0x222));
    add_fill_circle(parent, 85, 50, 3, lv_color_hex(0x222));
    add_fill_circle(parent, 70, 80, 3, lv_color_hex(0xFF8CA0));
}

static void draw_picture_map(lv_obj_t *parent)
{
    add_fill_rect(parent, 30, 25, 80, 70, lv_color_hex(0x90EE90));
    add_fill_rect(parent, 30, 25, 25, 30, lv_color_hex(0x228B22));
    add_fill_rect(parent, 60, 65, 30, 20, lv_color_hex(0x8B4513));
    add_fill_rect(parent, 75, 25, 20, 20, lv_color_hex(0xADD8E6));
}

static void draw_picture_cat(lv_obj_t *parent)
{
    add_fill_circle(parent, 70, 50, 22, lv_color_hex(0xFFA500));
    lv_obj_t *e1 = add_fill_circle(parent, 55, 30, 12, lv_color_hex(0xFFA500));
    lv_obj_t *e2 = add_fill_circle(parent, 85, 30, 12, lv_color_hex(0xFFA500));
    (void)e1; (void)e2;
    add_fill_circle(parent, 60, 47, 3, lv_color_hex(0x222));
    add_fill_circle(parent, 80, 47, 3, lv_color_hex(0x222));
    add_fill_circle(parent, 70, 55, 4, lv_color_hex(0xFF69B4));
}

static void draw_picture_fox(lv_obj_t *parent)
{
    add_fill_circle(parent, 70, 55, 22, lv_color_hex(0xFF6600));
    add_fill_circle(parent, 60, 45, 3, lv_color_hex(0x222));
    add_fill_circle(parent, 80, 45, 3, lv_color_hex(0x222));
    add_fill_circle(parent, 70, 58, 4, lv_color_hex(0x222));
    add_fill_rect(parent, 65, 42, 10, 4, lv_color_hex(0xFFFFFF));
}

static void draw_picture_hen(lv_obj_t *parent)
{
    add_fill_circle(parent, 70, 60, 22, lv_color_hex(0xFFFF00));
    add_fill_circle(parent, 70, 45, 12, lv_color_hex(0xFFFF00));
    add_fill_circle(parent, 60, 42, 3, lv_color_hex(0x222));
    add_fill_circle(parent, 80, 42, 3, lv_color_hex(0x222));
    lv_obj_t *comb = add_fill_circle(parent, 70, 33, 5, lv_color_hex(0xFF0000));
    (void)comb;
    lv_obj_t *beak = add_fill_rect(parent, 95, 55, 12, 6, lv_color_hex(0xFFA500));
    (void)beak;
}

static void draw_picture_bug(lv_obj_t *parent)
{
    add_fill_circle(parent, 70, 55, 18, lv_color_hex(0x32CD32));
    add_fill_circle(parent, 60, 50, 8, lv_color_hex(0x32CD32));
    add_fill_circle(parent, 80, 50, 8, lv_color_hex(0x32CD32));
    add_fill_circle(parent, 70, 55, 8, lv_color_hex(0x228B22));
    add_fill_circle(parent, 85, 65, 5, lv_color_hex(0x32CD32));
    add_fill_circle(parent, 55, 65, 5, lv_color_hex(0x32CD32));
}

typedef void (*pic_draw_fn)(lv_obj_t *);
typedef struct { const char *word; pic_draw_fn draw; } pic_word_t;

static const pic_word_t pic_words[] = {
    {"log", draw_picture_log},
    {"mug", draw_picture_mug},
    {"dog", draw_picture_dog},
    {"pig", draw_picture_pig},
    {"map", draw_picture_map},
    {"cat", draw_picture_cat},
    {"fox", draw_picture_fox},
    {"hen", draw_picture_hen},
    {"bug", draw_picture_bug},
};
#define PIC_COUNT (sizeof(pic_words) / sizeof(pic_words[0]))

// ---- Word list ----

#define MAX_WORDS 60
static char car_words[MAX_WORDS][24];
static int car_word_count = 0;
static int car_word_index = 0;

static const char *fallback_words[] = {
    "a", "and", "away", "big", "blue", "can", "come", "down", "find",
    "for", "funny", "go", "help", "here", "I", "in", "is", "it", "jump",
    "little", "look", "make", "me", "my", "not", "one", "play", "red",
    "run", "said", "see", "the", "three", "to", "two", "up", "we",
    "where", "yellow", "you"
};
#define FALLBACK_COUNT (sizeof(fallback_words) / sizeof(fallback_words[0]))

static void load_word_list(void)
{
    car_word_count = 0;
    car_word_index = 0;

    JsonDocument doc;
    if (nino_sd_is_mounted() && nino_storage_read("/word_lists/pre_primer.json", doc)) {
        JsonArray arr = doc.as<JsonArray>();
        for (JsonVariant item : arr) {
            if (car_word_count >= MAX_WORDS) break;
            bool enabled = item["enabled"] | true;
            if (!enabled) continue;
            const char *w = item["word"];
            if (!w) continue;
            strncpy(car_words[car_word_count], w, sizeof(car_words[0]) - 1);
            car_words[car_word_count][sizeof(car_words[0]) - 1] = 0;
            car_word_count++;
        }
    }

    if (car_word_count == 0) {
        for (unsigned i = 0; i < FALLBACK_COUNT && car_word_count < MAX_WORDS; i++) {
            strncpy(car_words[car_word_count], fallback_words[i], sizeof(car_words[0]) - 1);
            car_words[car_word_count][sizeof(car_words[0]) - 1] = 0;
            car_word_count++;
        }
    }
}

static const char *next_word(void)
{
    if (car_word_count == 0) return "";
    car_word_index = (car_word_index + 1) % car_word_count;
    return car_words[car_word_index];
}

// ---- UI state ----

static lv_obj_t *mode_label = NULL;
static lv_obj_t *word_display = NULL;
static lv_obj_t *la_vi_btn = NULL;
static lv_obj_t *location_area = NULL;
static lv_obj_t *timer_label = NULL;
static lv_obj_t *main_content = NULL;
static lv_obj_t *pic_display = NULL;
static lv_obj_t *choice_area = NULL;
static lv_timer_t *auto_timer = NULL;
static bool in_car_mode = true;
static int pic_index = 0;

// ---- Picture match helpers ----

static void show_picture_match(void);

static void shuffle_pic_order(void)
{
    pic_index = random(PIC_COUNT);
}

static void on_pic_choice(lv_event_t *e)
{
    const char *chosen = (const char *)lv_event_get_user_data(e);
    const char *correct = pic_words[pic_index].word;
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    if (strcmp(chosen, correct) == 0) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x44BB44), 0);
        lv_async_call([](void*) {
            shuffle_pic_order();
            show_picture_match();
        }, NULL);
    } else {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xCC4444), 0);
    }
}

static void show_picture_match(void)
{
    if (!main_content) return;
    lv_obj_clean(main_content);

    lv_obj_set_flex_flow(main_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main_content, 4, 0);

    pic_display = lv_obj_create(main_content);
    lv_obj_remove_style_all(pic_display);
    lv_obj_set_size(pic_display, 460, 120);
    lv_obj_set_style_border_width(pic_display, 1, 0);
    lv_obj_set_style_border_color(pic_display, lv_color_hex(0xCCCCCC), 0);
    lv_obj_clear_flag(pic_display, LV_OBJ_FLAG_SCROLLABLE);

    pic_words[pic_index].draw(pic_display);

    choice_area = lv_obj_create(main_content);
    lv_obj_remove_style_all(choice_area);
    lv_obj_set_width(choice_area, 460);
    lv_obj_set_flex_flow(choice_area, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(choice_area, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(choice_area, 10, 0);

    const char *correct = pic_words[pic_index].word;
    int wrong_idx;
    do {
        wrong_idx = random(PIC_COUNT);
    } while (wrong_idx == pic_index);
    const char *wrong = pic_words[wrong_idx].word;

    const char *opts[2] = {correct, wrong};
    if (random(2) == 1) { opts[0] = wrong; opts[1] = correct; }

    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_btn_create(choice_area);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_size(btn, 120, 60);
        lv_obj_set_style_shadow_width(btn, 0, 0);

        lv_obj_t *lab = lv_label_create(btn);
        lv_label_set_text(lab, opts[i]);
        lv_obj_set_style_text_font(lab, &lv_font_montserrat_24, 0);
        lv_obj_center(lab);

        char *copy = (char *)lv_malloc(strlen(opts[i]) + 1);
        if (copy) {
            strcpy(copy, opts[i]);
            lv_obj_add_event_cb(btn, on_pic_choice, LV_EVENT_CLICKED, copy);
        }
    }
}

// ---- Car mode helpers ----

static void on_location_tap(lv_event_t *e)
{
    (void)e;
    const char *word = next_word();
    lv_label_set_text(word_display, word);
    lv_obj_add_flag(location_area, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(la_vi_btn, LV_OBJ_FLAG_HIDDEN);
}

static void on_la_vi_tap(lv_event_t *e)
{
    (void)e;
    lv_obj_add_flag(la_vi_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(location_area, LV_OBJ_FLAG_HIDDEN);
}

static void show_car_mode(void);

static void on_auto_timer(lv_timer_t *t)
{
    (void)t;
    show_car_mode();
}

static void on_main_content_delete(lv_event_t *e)
{
    (void)e;
    if (auto_timer) {
        lv_timer_del(auto_timer);
        auto_timer = NULL;
    }
    main_content = NULL;
}

static void show_car_mode(void)
{
    if (!main_content) return;
    if (auto_timer) {
        lv_timer_del(auto_timer);
        auto_timer = NULL;
    }

    lv_obj_clean(main_content);
    lv_obj_clear_flag(main_content, LV_OBJ_FLAG_SCROLLABLE);

    const char *word = next_word();

    word_display = lv_label_create(main_content);
    lv_label_set_text(word_display, word);
    lv_obj_set_style_text_font(word_display, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_align(word_display, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(word_display, 460, WORD_AREA_H);
    lv_obj_set_style_bg_color(word_display, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(word_display, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_top(word_display, 50, 0);
    lv_obj_center(word_display);

    la_vi_btn = lv_btn_create(main_content);
    lv_obj_set_style_bg_color(la_vi_btn, lv_color_hex(0x44BB44), 0);
    lv_obj_set_size(la_vi_btn, 220, BTN_AREA_H);
    lv_obj_set_style_radius(la_vi_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(la_vi_btn, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_t *la_lab = lv_label_create(la_vi_btn);
    lv_label_set_text(la_lab, "La vi!");
    lv_obj_set_style_text_font(la_lab, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(la_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(la_lab);
    lv_obj_add_event_cb(la_vi_btn, on_la_vi_tap, LV_EVENT_CLICKED, NULL);

    location_area = lv_obj_create(main_content);
    lv_obj_remove_style_all(location_area);
    lv_obj_set_size(location_area, 460, BTN_AREA_H + 20);
    lv_obj_align(location_area, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(location_area, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(location_area, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(location_area, 4, 0);
    lv_obj_add_flag(location_area, LV_OBJ_FLAG_HIDDEN);

    static const char *locations[] = {"Store", "Car", "Sign", "House"};
    static const lv_color_t loc_colors[] = {
        lv_color_hex(0xE74C3C), lv_color_hex(0x3498DB),
        lv_color_hex(0xF39C12), lv_color_hex(0x27AE60)
    };
    for (int i = 0; i < 4; i++) {
        lv_obj_t *loc = lv_btn_create(location_area);
        lv_obj_set_style_bg_color(loc, loc_colors[i], 0);
        lv_obj_set_size(loc, 90, 50);
        lv_obj_set_style_radius(loc, 12, 0);
        lv_obj_set_style_shadow_width(loc, 0, 0);
        lv_obj_add_event_cb(loc, on_location_tap, LV_EVENT_CLICKED, NULL);

        lv_obj_t *ll = lv_label_create(loc);
        lv_label_set_text(ll, locations[i]);
        lv_obj_set_style_text_color(ll, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(ll, &lv_font_montserrat_14, 0);
        lv_obj_center(ll);
    }

    auto_timer = lv_timer_create(on_auto_timer, 15000, NULL);
    lv_timer_set_repeat_count(auto_timer, 1);
}

// ---- Mode switching ----

static void on_mode_car(lv_event_t *e)
{
    (void)e;
    if (in_car_mode) return;
    in_car_mode = true;
    show_car_mode();
}

static void on_mode_match(lv_event_t *e)
{
    (void)e;
    if (!in_car_mode) return;
    in_car_mode = false;
    shuffle_pic_order();
    show_picture_match();
}

void app_word_spy_create(lv_obj_t *content)
{
    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    load_word_list();

    // Mode buttons
    lv_obj_t *mode_bar = lv_obj_create(content);
    lv_obj_remove_style_all(mode_bar);
    lv_obj_set_size(mode_bar, 460, 36);
    lv_obj_align(mode_bar, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_flex_flow(mode_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *car_btn = lv_btn_create(mode_bar);
    lv_obj_set_style_bg_color(car_btn, lv_color_hex(0x3498DB), 0);
    lv_obj_set_size(car_btn, 140, 32);
    lv_obj_set_style_radius(car_btn, 16, 0);
    lv_obj_set_style_shadow_width(car_btn, 0, 0);
    lv_obj_add_event_cb(car_btn, on_mode_car, LV_EVENT_CLICKED, NULL);
    lv_obj_t *car_lab = lv_label_create(car_btn);
    lv_label_set_text(car_lab, "Car Mode");
    lv_obj_set_style_text_color(car_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(car_lab);

    lv_obj_t *pic_btn = lv_btn_create(mode_bar);
    lv_obj_set_style_bg_color(pic_btn, lv_color_hex(0xE67E22), 0);
    lv_obj_set_size(pic_btn, 140, 32);
    lv_obj_set_style_radius(pic_btn, 16, 0);
    lv_obj_set_style_shadow_width(pic_btn, 0, 0);
    lv_obj_add_event_cb(pic_btn, on_mode_match, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pic_lab = lv_label_create(pic_btn);
    lv_label_set_text(pic_lab, "Picture Match");
    lv_obj_set_style_text_color(pic_lab, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(pic_lab);

    // Main content area (below mode bar)
    main_content = lv_obj_create(content);
    lv_obj_add_event_cb(main_content, on_main_content_delete, LV_EVENT_DELETE, NULL);
    lv_obj_remove_style_all(main_content);
    lv_obj_set_size(main_content, 480, 232);
    lv_obj_align(main_content, LV_ALIGN_TOP_MID, 0, 44);

    // Default: car mode
    in_car_mode = true;
    show_car_mode();
}
