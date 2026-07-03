#include "app_story_time.h"
#include "nino_colors.h"
#include <Arduino.h>

#define MAX_QUIZ  12

typedef enum { QT_COVER, QT_WORD, QT_LETTER, QT_SPACE, QT_PERIOD } quiz_type_t;

static lv_obj_t *instruction_label;
static lv_obj_t *quiz_area;
static lv_obj_t *score_label;
static lv_obj_t *content_ref;
static int current_q = 0;
static int correct_count = 0;

// ---- Book Parts Quiz ----

static void next_question(void);

static void on_correct_tap(lv_event_t *e)
{
    (void)e;
    correct_count++;
    char buf[24];
    snprintf(buf, sizeof(buf), "%d / %d", correct_count, MAX_QUIZ);
    lv_label_set_text(score_label, buf);
    lv_async_call([](void *p) {
        (void)p;
        next_question();
    }, NULL);
}

static void on_wrong_tap(lv_event_t *e)
{
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_set_style_bg_color(target, lv_color_hex(0xCC4444), 0);
    lv_async_call([](void *p) {
        lv_obj_set_style_bg_color((lv_obj_t *)p, lv_color_hex(0xCCCCCC), 0);
    }, (void *)target);
}

static lv_obj_t *make_tap_target(lv_obj_t *parent, int x, int y, int w, int h, bool correct)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_bg_color(o, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, 4, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(o, correct ? on_correct_tap : on_wrong_tap, LV_EVENT_CLICKED, NULL);
    return o;
}

static void draw_book_quiz(void)
{
    lv_obj_clean(quiz_area);
    lv_obj_set_style_bg_color(quiz_area, lv_color_hex(0xFFFFFF), 0);
    lv_obj_clear_flag(quiz_area, LV_OBJ_FLAG_SCROLLABLE);

    // Draw book shape
    lv_obj_t *cover = lv_obj_create(quiz_area);
    lv_obj_remove_style_all(cover);
    lv_obj_set_size(cover, 90, 130);
    lv_obj_set_pos(cover, 40, 10);
    lv_obj_set_style_bg_color(cover, lv_color_hex(0x2E86C1), 0);
    lv_obj_set_style_bg_opa(cover, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cover, 2, 0);
    lv_obj_clear_flag(cover, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *pages = lv_obj_create(quiz_area);
    lv_obj_remove_style_all(pages);
    lv_obj_set_size(pages, 160, 120);
    lv_obj_set_pos(pages, 130, 15);
    lv_obj_set_style_bg_color(pages, lv_color_hex(0xFFF8E7), 0);
    lv_obj_set_style_bg_opa(pages, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pages, 2, 0);
    lv_obj_clear_flag(pages, LV_OBJ_FLAG_SCROLLABLE);

    static const char *lines[] = {"The cat", "sat on", "the mat."};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *l = lv_label_create(pages);
        lv_label_set_text(l, lines[i]);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_pos(l, 8, 8 + i * 28);
    }

    // Tap zones
    make_tap_target(quiz_area, 40, 10, 90, 130, true);
    make_tap_target(quiz_area, 130, 15, 160, 120, false);
}

static void draw_sentence_quiz(quiz_type_t qt)
{
    lv_obj_clean(quiz_area);
    lv_obj_clear_flag(quiz_area, LV_OBJ_FLAG_SCROLLABLE);

    const char *sentence = "The cat sat.";
    const struct { const char *text; int is_target; } parts[] = {
        {"The ", 0}, {"cat", 0}, {" ", 1}, {"sat", 0}, {".", 1}
    };
    int target_idx;
    if (qt == QT_SPACE) target_idx = 2;
    else if (qt == QT_PERIOD) target_idx = 4;
    else target_idx = -1;

    lv_obj_t *sentence_cont = lv_obj_create(quiz_area);
    lv_obj_remove_style_all(sentence_cont);
    lv_obj_set_size(sentence_cont, 340, 60);
    lv_obj_center(sentence_cont);
    lv_obj_set_flex_flow(sentence_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sentence_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (unsigned i = 0; i < sizeof(parts) / sizeof(parts[0]); i++) {
        lv_obj_t *p = lv_label_create(sentence_cont);
        lv_label_set_text(p, parts[i].text);
        lv_obj_set_style_text_font(p, &lv_font_montserrat_28, 0);
        lv_obj_set_style_bg_color(p, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(p, 4, 0);
        if ((int)i == target_idx) {
            lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(p, on_correct_tap, LV_EVENT_CLICKED, NULL);
        } else {
            lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(p, on_wrong_tap, LV_EVENT_CLICKED, NULL);
        }
    }
}

static void draw_letter_quiz(void)
{
    lv_obj_clean(quiz_area);
    lv_obj_clear_flag(quiz_area, LV_OBJ_FLAG_SCROLLABLE);

    const char *word = "cat";
    int target_letter = 1;

    lv_obj_t *word_cont = lv_obj_create(quiz_area);
    lv_obj_remove_style_all(word_cont);
    lv_obj_set_size(word_cont, 200, 60);
    lv_obj_center(word_cont);
    lv_obj_set_flex_flow(word_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(word_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; word[i]; i++) {
        char buf[2] = {word[i], 0};
        lv_obj_t *l = lv_label_create(word_cont);
        lv_label_set_text(l, buf);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);
        lv_obj_set_style_bg_color(l, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_bg_opa(l, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(l, 6, 0);
        lv_obj_add_flag(l, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(l, (i == target_letter) ? on_correct_tap : on_wrong_tap, LV_EVENT_CLICKED, NULL);
    }
}

static void draw_word_quiz(void)
{
    lv_obj_clean(quiz_area);
    lv_obj_clear_flag(quiz_area, LV_OBJ_FLAG_SCROLLABLE);

    const char *target_word = "sat";
    static const char *choices[] = {"sat", "run"};
    int correct_idx = 0;

    lv_obj_t *cont = lv_obj_create(quiz_area);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, 340, 80);
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x3498DB), 0);
        lv_obj_set_size(btn, 120, 60);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_24, 0);
        lv_obj_add_event_cb(btn, (i == correct_idx) ? on_correct_tap : on_wrong_tap, LV_EVENT_CLICKED, NULL);
        lv_obj_t *lab = lv_label_create(btn);
        lv_label_set_text(lab, choices[i]);
        lv_obj_center(lab);
    }
}

static void draw_quiz(int q)
{
    if (q >= MAX_QUIZ) {
        lv_obj_clean(quiz_area);
        lv_obj_t *done = lv_label_create(quiz_area);
        lv_label_set_text(done, "\xE2\x98\x85 All done!");
        lv_obj_set_style_text_font(done, &lv_font_montserrat_28, 0);
        lv_obj_center(done);
        return;
    }

    // Cycle through quiz types
    static const quiz_type_t cycle[] = {QT_COVER, QT_COVER, QT_LETTER, QT_LETTER, QT_SPACE, QT_PERIOD, QT_WORD, QT_WORD};
    quiz_type_t qt = cycle[q % (sizeof(cycle) / sizeof(cycle[0]))];

    switch (qt) {
        case QT_COVER:   draw_book_quiz(); break;
        case QT_LETTER:  draw_letter_quiz(); break;
        case QT_SPACE:
        case QT_PERIOD:  draw_sentence_quiz(qt); break;
        case QT_WORD:    draw_word_quiz(); break;
    }
}

static void next_question(void)
{
    current_q++;
    if (current_q < MAX_QUIZ) {
        char buf[48];
        static const char *instructions[] = {
            "Tap the front cover!",
            "Where is the front cover?",
            "Tap the letter 'a'!",
            "Find the letter 'a'!",
            "Tap a space between words!",
            "Tap the period!",
            "Tap the word 'sat'!",
            "Which word is 'sat'?",
        };
        int idx = current_q % (sizeof(instructions) / sizeof(instructions[0]));
        lv_label_set_text(instruction_label, instructions[idx]);
        draw_quiz(current_q);
    } else {
        lv_label_set_text(instruction_label, "\xE2\x98\x85 Great job!");
        draw_quiz(current_q);
    }
}

void app_story_time_create(lv_obj_t *content)
{
    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    content_ref = content;
    current_q = 0;
    correct_count = 0;

    // Instruction label
    instruction_label = lv_label_create(content);
    lv_label_set_text(instruction_label, "Tap the front cover!");
    lv_obj_set_style_text_font(instruction_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(instruction_label, lv_color_hex(0x333333), 0);
    lv_obj_set_width(instruction_label, 460);
    lv_obj_set_style_text_align(instruction_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(instruction_label, LV_ALIGN_TOP_MID, 0, 8);

    // Quiz area
    quiz_area = lv_obj_create(content);
    lv_obj_remove_style_all(quiz_area);
    lv_obj_set_size(quiz_area, 460, 170);
    lv_obj_align(quiz_area, LV_ALIGN_TOP_MID, 0, 44);
    lv_obj_set_style_bg_color(quiz_area, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(quiz_area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(quiz_area, 8, 0);
    lv_obj_clear_flag(quiz_area, LV_OBJ_FLAG_SCROLLABLE);

    // Score
    score_label = lv_label_create(content);
    lv_label_set_text(score_label, "0 / 12");
    lv_obj_set_style_text_font(score_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(score_label, lv_color_hex(0x888888), 0);
    lv_obj_align(score_label, LV_ALIGN_BOTTOM_MID, 0, -8);

    draw_quiz(0);
}
