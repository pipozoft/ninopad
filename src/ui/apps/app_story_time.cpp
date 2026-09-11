#include "app_story_time.h"
#include "nino_colors.h"
#include "scr_congrats.h"
#include "storage/storage.h"
#include "utils/anim_utils.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>

#define STORIES_PATH       "/reading/stories.json"
#define MAX_STORIES        16
#define QUESTIONS_PER_STORY 3
#define CHOICES_PER_QUESTION 3

struct StorySummary {
    uint16_t source_index;
    uint8_t level;
    char theme[24];
    char title[32];
};

struct ReadingQuestion {
    char prompt[80];
    char choices[CHOICES_PER_QUESTION][40];
    uint8_t answer;
};

struct ReadingStory {
    char title[32];
    char theme[24];
    char image[80];
    char image_large[80];
    char passage[384];
    ReadingQuestion questions[QUESTIONS_PER_STORY];
};

static StorySummary library[MAX_STORIES];
static int library_count;
static int current_level;
static int selected_story;
static int question_index;
static int first_try_answers;
static bool question_missed;
static bool answer_locked;
static int choice_order[CHOICES_PER_QUESTION];
static ReadingStory story;

static lv_obj_t *g_content;
static lv_obj_t *feedback_label;
static lv_obj_t *choice_buttons[CHOICES_PER_QUESTION];
static lv_obj_t *read_again_button;
static lv_obj_t *next_button;
static lv_obj_t *overlay;
static lv_obj_t *image_dialog;
static char image_source[84];
static char large_image_source[84];

static void show_level_picker(void);
static void show_story_picker(void);
static void show_reading(bool review);
static void show_question(bool new_question);

static bool copy_text(char *dest, size_t size, const char *source)
{
    if (!source || strlen(source) >= size) return false;
    strcpy(dest, source);
    return true;
}

static lv_color_t level_color(int level)
{
    static const uint32_t colors[] = {0x2ECC71, 0x3498DB, 0x9B59B6};
    return lv_color_hex(colors[level - 1]);
}

static lv_obj_t *make_button(lv_obj_t *parent, const char *text,
                             int x, int y, int width, int height,
                             lv_color_t color, lv_event_cb_t callback, void *data)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_radius(button, height / 2, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_set_style_bg_color(button, color, 0);
    lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, data);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(label);
    return button;
}

static bool reset_content(void)
{
    if (!g_content || !lv_obj_is_valid(g_content)) return false;
    image_dialog = nullptr;
    lv_obj_clean(g_content);
    lv_obj_set_style_bg_color(g_content, lv_color_hex(0xFFF8F0), 0);
    return true;
}

static bool load_library(void)
{
    JsonDocument doc;
    if (!nino_storage_read(STORIES_PATH, doc) || !doc.is<JsonArray>()) return false;

    library_count = 0;
    uint16_t source_index = 0;
    bool levels_found[3] = {false, false, false};
    for (JsonObject item : doc.as<JsonArray>()) {
        int level = item["level"] | 0;
        if (library_count < MAX_STORIES && level >= 1 && level <= 3) {
            StorySummary &summary = library[library_count];
            if (!copy_text(summary.theme, sizeof(summary.theme), item["theme"]) ||
                !copy_text(summary.title, sizeof(summary.title), item["title"])) {
                source_index++;
                continue;
            }
            summary.source_index = source_index;
            summary.level = level;
            levels_found[level - 1] = true;
            library_count++;
        }
        source_index++;
    }
    return library_count > 0 && levels_found[0] && levels_found[1] && levels_found[2];
}

static bool load_selected_story(void)
{
    if (selected_story < 0 || selected_story >= library_count) return false;

    JsonDocument doc;
    if (!nino_storage_read(STORIES_PATH, doc) || !doc.is<JsonArray>()) return false;
    JsonArray stories = doc.as<JsonArray>();
    uint16_t source_index = library[selected_story].source_index;
    if (source_index >= stories.size()) return false;

    JsonObject item = stories[source_index];
    const char *image = item["image"] | "";
    const char *image_large = item["image_large"] | "";
    if (!copy_text(story.title, sizeof(story.title), item["title"]) ||
        !copy_text(story.theme, sizeof(story.theme), item["theme"]) ||
        !copy_text(story.image, sizeof(story.image), image) ||
        !copy_text(story.image_large, sizeof(story.image_large), image_large)) return false;

    story.passage[0] = '\0';
    int sentence_count = 0;
    for (const char *sentence : item["sentences"].as<JsonArray>()) {
        if (!sentence || sentence_count >= 3) continue;
        size_t used = strlen(story.passage);
        size_t remaining = sizeof(story.passage) - used;
        int written = snprintf(story.passage + used, remaining,
                               "%s%s", used ? " " : "", sentence);
        if (written < 0 || (size_t)written >= remaining) return false;
        sentence_count++;
    }

    JsonArray questions = item["questions"].as<JsonArray>();
    if (sentence_count != 3 || questions.size() != QUESTIONS_PER_STORY) return false;
    for (int i = 0; i < QUESTIONS_PER_STORY; i++) {
        JsonObject question = questions[i];
        if (!copy_text(story.questions[i].prompt, sizeof(story.questions[i].prompt),
                       question["prompt"])) return false;
        JsonArray choices = question["choices"].as<JsonArray>();
        if (choices.size() != CHOICES_PER_QUESTION) return false;
        for (int j = 0; j < CHOICES_PER_QUESTION; j++) {
            if (!copy_text(story.questions[i].choices[j], sizeof(story.questions[i].choices[j]),
                           choices[j])) return false;
        }
        int answer = question["answer"] | -1;
        if (answer < 0 || answer >= CHOICES_PER_QUESTION) return false;
        story.questions[i].answer = answer;
    }
    return true;
}

static void show_error(const char *title, const char *message)
{
    if (!reset_content()) return;
    lv_obj_t *title_label = lv_label_create(g_content);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title_label, NINO_COLOR_DANGER, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -28);

    lv_obj_t *message_label = lv_label_create(g_content);
    lv_label_set_text(message_label, message);
    lv_obj_set_width(message_label, 420);
    lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(message_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(message_label, lv_color_hex(0x555555), 0);
    lv_obj_align(message_label, LV_ALIGN_CENTER, 0, 24);
}

static void on_level(lv_event_t *event)
{
    current_level = (int)(intptr_t)lv_event_get_user_data(event);
    lv_async_call([](void *) { show_story_picker(); }, nullptr);
}

static void show_level_picker(void)
{
    if (!reset_content()) return;
    lv_obj_t *title = lv_label_create(g_content);
    lv_label_set_text(title, "Choose a reading level");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x444444), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

    static const char *labels[] = {"Starting Reader", "Growing Reader", "Super Reader"};
    for (int i = 0; i < 3; i++) {
        make_button(g_content, labels[i], 92, 66 + i * 62, 280, 50,
                    level_color(i + 1), on_level, (void *)(intptr_t)(i + 1));
    }
}

static void open_story(void *)
{
    if (!g_content || !lv_obj_is_valid(g_content)) return;
    if (!load_selected_story()) {
        show_error("Story could not open", "Check /reading/stories.json on the SD card.");
        return;
    }
    question_index = 0;
    first_try_answers = 0;
    show_reading(false);
}

static void on_story(lv_event_t *event)
{
    selected_story = (int)(intptr_t)lv_event_get_user_data(event);
    lv_async_call(open_story, nullptr);
}

static void on_surprise(lv_event_t *)
{
    int matching[MAX_STORIES];
    int count = 0;
    for (int i = 0; i < library_count; i++) {
        if (library[i].level == current_level) matching[count++] = i;
    }
    if (!count) return;
    selected_story = matching[random(count)];
    lv_async_call(open_story, nullptr);
}

static void on_levels(lv_event_t *)
{
    lv_async_call([](void *) { show_level_picker(); }, nullptr);
}

static void show_story_picker(void)
{
    if (!reset_content()) return;
    make_button(g_content, LV_SYMBOL_LEFT, 4, 2, 40, 32,
                lv_color_hex(0x888888), on_levels, nullptr);

    lv_obj_t *title = lv_label_create(g_content);
    lv_label_set_text(title, "Pick a story");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, level_color(current_level), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    int shown = 0;
    for (int i = 0; i < library_count && shown < 4; i++) {
        if (library[i].level != current_level) continue;
        int column = shown % 2;
        int row = shown / 2;
        lv_obj_t *card = lv_btn_create(g_content);
        lv_obj_set_pos(card, 8 + column * 228, 40 + row * 78);
        lv_obj_set_size(card, 220, 68);
        lv_obj_set_style_radius(card, 14, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_set_style_bg_color(card, lv_color_lighten(level_color(current_level), 45), 0);
        lv_obj_add_event_cb(card, on_story, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *theme = lv_label_create(card);
        lv_label_set_text(theme, library[i].theme);
        lv_obj_set_style_text_font(theme, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(theme, lv_color_hex(0x555555), 0);
        lv_obj_align(theme, LV_ALIGN_TOP_LEFT, 2, -2);

        lv_obj_t *story_title = lv_label_create(card);
        lv_label_set_text(story_title, library[i].title);
        lv_obj_set_width(story_title, 190);
        lv_label_set_long_mode(story_title, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(story_title, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(story_title, lv_color_hex(0x222222), 0);
        lv_obj_align(story_title, LV_ALIGN_BOTTOM_LEFT, 2, 2);
        shown++;
    }

    make_button(g_content, "Surprise Me", 142, 204, 180, 42,
                level_color(current_level), on_surprise, nullptr);
}

static void on_read_ready(lv_event_t *)
{
    question_index = 0;
    first_try_answers = 0;
    lv_async_call([](void *) { show_question(true); }, nullptr);
}

static void on_back_to_stories(lv_event_t *)
{
    lv_async_call([](void *) { show_story_picker(); }, nullptr);
}

static void on_back_to_question(lv_event_t *)
{
    lv_async_call([](void *) { show_question(false); }, nullptr);
}

static void close_image_dialog(void)
{
    if (!image_dialog || !lv_obj_is_valid(image_dialog)) return;
    lv_obj_t *dialog = image_dialog;
    image_dialog = nullptr;
    lv_obj_delete_async(dialog);
}

static void on_image_dialog_backdrop(lv_event_t *event)
{
    if (lv_event_get_target(event) == image_dialog) close_image_dialog();
}

static void on_image_dialog_close(lv_event_t *)
{
    close_image_dialog();
}

static void on_story_image(lv_event_t *)
{
    if (image_dialog || !story.image_large[0] ||
        !nino_storage_exists(story.image_large)) return;

    image_dialog = lv_obj_create(g_content);
    lv_obj_remove_style_all(image_dialog);
    lv_obj_set_size(image_dialog, 480, 276);
    lv_obj_set_pos(image_dialog, -8, -8);
    lv_obj_set_style_bg_color(image_dialog, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(image_dialog, LV_OPA_80, 0);
    lv_obj_add_flag(image_dialog, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(image_dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(image_dialog, on_image_dialog_backdrop, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *card = lv_obj_create(image_dialog);
    lv_obj_set_size(card, 336, 256);
    lv_obj_center(card);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x111111), 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    snprintf(large_image_source, sizeof(large_image_source), "S:%s", story.image_large);
    lv_obj_t *image = lv_image_create(card);
    lv_image_set_src(image, large_image_source);
    lv_obj_center(image);

    lv_obj_t *close_button = lv_btn_create(card);
    lv_obj_set_size(close_button, 40, 40);
    lv_obj_align(close_button, LV_ALIGN_TOP_RIGHT, -2, 2);
    lv_obj_set_style_radius(close_button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(close_button, 0, 0);
    lv_obj_set_style_bg_color(close_button, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(close_button, on_image_dialog_close, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *close_label = lv_label_create(close_button);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(close_label, lv_color_hex(0x222222), 0);
    lv_obj_center(close_label);
}

static void create_story_art(void)
{
    lv_obj_t *frame = lv_obj_create(g_content);
    lv_obj_set_pos(frame, 4, 48);
    lv_obj_set_size(frame, 128, 104);
    lv_obj_set_style_radius(frame, 16, 0);
    lv_obj_set_style_border_width(frame, 0, 0);
    lv_obj_set_style_bg_color(frame, lv_color_lighten(level_color(current_level), 55), 0);
    lv_obj_set_style_pad_all(frame, 0, 0);
    lv_obj_clear_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    if (story.image[0] && nino_storage_exists(story.image)) {
        snprintf(image_source, sizeof(image_source), "S:%s", story.image);
        lv_obj_t *image = lv_image_create(frame);
        lv_image_set_src(image, image_source);
        lv_obj_center(image);

        if (story.image_large[0] && nino_storage_exists(story.image_large)) {
            lv_obj_add_flag(frame, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(frame, on_story_image, LV_EVENT_CLICKED, nullptr);

            lv_obj_t *badge = lv_obj_create(frame);
            lv_obj_set_size(badge, 26, 26);
            lv_obj_align(badge, LV_ALIGN_BOTTOM_RIGHT, -3, -3);
            lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_width(badge, 0, 0);
            lv_obj_set_style_bg_color(badge, lv_color_hex(0x222222), 0);
            lv_obj_set_style_bg_opa(badge, LV_OPA_80, 0);
            lv_obj_set_style_pad_all(badge, 0, 0);
            lv_obj_clear_flag(badge, LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t *badge_label = lv_label_create(badge);
            lv_label_set_text(badge_label, LV_SYMBOL_PLUS);
            lv_obj_set_style_text_font(badge_label, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(badge_label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_center(badge_label);
        }
    } else {
        lv_obj_t *theme = lv_label_create(frame);
        lv_label_set_text(theme, story.theme);
        lv_obj_set_width(theme, 110);
        lv_label_set_long_mode(theme, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(theme, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(theme, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(theme, lv_color_hex(0x333333), 0);
        lv_obj_center(theme);
    }
}

static void show_reading(bool review)
{
    if (!reset_content()) return;

    lv_obj_t *theme = lv_label_create(g_content);
    lv_label_set_text(theme, story.theme);
    lv_obj_set_style_text_font(theme, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(theme, level_color(current_level), 0);
    lv_obj_set_pos(theme, 8, 6);

    lv_obj_t *title = lv_label_create(g_content);
    lv_label_set_text(title, story.title);
    lv_obj_set_width(title, 310);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x333333), 0);
    lv_obj_set_pos(title, 146, 4);

    create_story_art();

    lv_obj_t *passage = lv_label_create(g_content);
    lv_label_set_text(passage, story.passage);
    lv_obj_set_width(passage, 310);
    lv_label_set_long_mode(passage, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(passage, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(passage, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_line_space(passage, 5, 0);
    lv_obj_set_pos(passage, 146, 40);

    if (review) {
        make_button(g_content, "Back to Question", 246, 214, 200, 38,
                    level_color(current_level), on_back_to_question, nullptr);
    } else {
        make_button(g_content, "Stories", 18, 214, 130, 38,
                    lv_color_hex(0x888888), on_back_to_stories, nullptr);
        make_button(g_content, "I'm Ready", 286, 214, 160, 38,
                    level_color(current_level), on_read_ready, nullptr);
    }
}

static void on_read_again(lv_event_t *)
{
    lv_async_call([](void *) { show_reading(true); }, nullptr);
}

static void show_completion(void *)
{
    if (!g_content || !lv_obj_is_valid(g_content)) return;
    int stars = first_try_answers == 3 ? 3 : first_try_answers == 2 ? 2 : 1;
    overlay = nino_congrats_create(g_content, "Great reading!", stars,
                                    "You understood the story.",
                                    level_color(current_level), 190,
                                    [](lv_event_t *) {
        lv_async_call([](void *) {
            overlay = nullptr;
            show_story_picker();
        }, nullptr);
    });
}

static void on_next_question(lv_event_t *)
{
    question_index++;
    if (question_index >= QUESTIONS_PER_STORY) {
        lv_async_call(show_completion, nullptr);
    } else {
        lv_async_call([](void *) { show_question(true); }, nullptr);
    }
}

static void on_choice(lv_event_t *event)
{
    if (answer_locked) return;
    int button_index = (int)(intptr_t)lv_event_get_user_data(event);
    ReadingQuestion &question = story.questions[question_index];
    if (choice_order[button_index] != question.answer) {
        question_missed = true;
        lv_obj_set_style_bg_color(choice_buttons[button_index], NINO_COLOR_DANGER, 0);
        nino_anim_shake(choice_buttons[button_index]);
        lv_label_set_text(feedback_label, "Look in the story and try again.");
        return;
    }

    answer_locked = true;
    if (!question_missed) first_try_answers++;
    lv_obj_set_style_bg_color(choice_buttons[button_index], NINO_COLOR_SUCCESS, 0);
    lv_label_set_text(feedback_label, "Great thinking!");
    lv_obj_add_flag(read_again_button, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(next_button, LV_OBJ_FLAG_HIDDEN);
}

static void show_question(bool new_question)
{
    if (!reset_content()) return;
    answer_locked = false;
    if (new_question) {
        question_missed = false;
        for (int i = 0; i < CHOICES_PER_QUESTION; i++) choice_order[i] = i;
        for (int i = CHOICES_PER_QUESTION - 1; i > 0; i--) {
            int pick = random(i + 1);
            int temp = choice_order[i];
            choice_order[i] = choice_order[pick];
            choice_order[pick] = temp;
        }
    }

    char progress[24];
    snprintf(progress, sizeof(progress), "Question %d of %d", question_index + 1,
             QUESTIONS_PER_STORY);
    lv_obj_t *progress_label = lv_label_create(g_content);
    lv_label_set_text(progress_label, progress);
    lv_obj_set_style_text_font(progress_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(progress_label, lv_color_hex(0x888888), 0);
    lv_obj_align(progress_label, LV_ALIGN_TOP_RIGHT, -8, 2);

    ReadingQuestion &question = story.questions[question_index];
    lv_obj_t *prompt = lv_label_create(g_content);
    lv_label_set_text(prompt, question.prompt);
    lv_obj_set_width(prompt, 440);
    lv_label_set_long_mode(prompt, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(prompt, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(prompt, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(prompt, lv_color_hex(0x333333), 0);
    lv_obj_align(prompt, LV_ALIGN_TOP_MID, 0, 22);

    for (int i = 0; i < CHOICES_PER_QUESTION; i++) {
        choice_buttons[i] = make_button(g_content, question.choices[choice_order[i]],
                                        22, 72 + i * 47, 420, 42,
                                        level_color(current_level), on_choice,
                                        (void *)(intptr_t)i);
    }

    feedback_label = lv_label_create(g_content);
    lv_label_set_text(feedback_label, "");
    lv_obj_set_width(feedback_label, 240);
    lv_obj_set_style_text_align(feedback_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(feedback_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(feedback_label, lv_color_hex(0x666666), 0);
    lv_obj_set_pos(feedback_label, 112, 216);

    read_again_button = make_button(g_content, "Read Again", 8, 218, 112, 34,
                                    lv_color_hex(0x888888), on_read_again, nullptr);
    next_button = make_button(g_content,
                              question_index == QUESTIONS_PER_STORY - 1 ? "See Stars" : "Next",
                              350, 218, 106, 34, NINO_COLOR_SUCCESS,
                              on_next_question, nullptr);
    lv_obj_add_flag(next_button, LV_OBJ_FLAG_HIDDEN);
}

void app_story_time_create(lv_obj_t *content)
{
    g_content = content;
    overlay = nullptr;
    image_dialog = nullptr;
    library_count = 0;
    current_level = 1;
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(content, [](lv_event_t *) {
        g_content = nullptr;
        overlay = nullptr;
        image_dialog = nullptr;
    }, LV_EVENT_DELETE, nullptr);

    if (!load_library()) {
        show_error("Story library not found", "Add /reading/stories.json to the SD card.");
        return;
    }
    show_level_picker();
}
