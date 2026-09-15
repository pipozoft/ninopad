// Picture artwork is from OpenMoji (CC BY-SA 4.0): https://openmoji.org/
#include "app_phonics.h"
#include "nino_colors.h"
#include "scr_congrats.h"
#include "storage/storage.h"
#include "utils/anim_utils.h"
#include <Arduino.h>
#include <string.h>

#define PHONICS_ROUNDS 8
#define SOUND_COUNT 12
#define WORD_COUNT 15

enum PhonicsMode {
    MODE_SOUNDS = 0,
    MODE_VOWELS,
    MODE_READING,
};

struct SoundGroup {
    char letter;
    const char *sound;
    const char *words[3];
};

struct WordRound {
    const char *word;
    const char *prompt;
    const char *choices[3];
};

static const SoundGroup sound_groups[SOUND_COUNT] = {
    {'m', "/m/", {"map", "mop", "mug"}},
    {'s', "/s/", {"sun", "sock", "sail"}},
    {'t', "/t/", {"tap", "top", "train"}},
    {'p', "/p/", {"pan", "pig", "pot"}},
    {'n', "/n/", {"nest", "net", "nut"}},
    {'b', "/b/", {"ball", "bed", "bus"}},
    {'c', "/k/", {"cap", "cat", "cup"}},
    {'d', "/d/", {"dog", "door", "duck"}},
    {'f', "/f/", {"fish", "fox", "frog"}},
    {'h', "/h/", {"hat", "hen", "house"}},
    {'r', "/r/", {"rain", "row", "run"}},
    {'l', "/l/", {"lamp", "lip", "log"}},
};

static const WordRound vowel_rounds[WORD_COUNT] = {
    {"cat", "c _ t", {"a", "e", "o"}},
    {"dog", "d _ g", {"o", "a", "i"}},
    {"hen", "h _ n", {"e", "i", "a"}},
    {"pig", "p _ g", {"i", "e", "o"}},
    {"sun", "s _ n", {"u", "a", "o"}},
    {"bed", "b _ d", {"e", "a", "u"}},
    {"bug", "b _ g", {"u", "a", "i"}},
    {"fox", "f _ x", {"o", "i", "a"}},
    {"map", "m _ p", {"a", "o", "u"}},
    {"lip", "l _ p", {"i", "a", "e"}},
    {"mop", "m _ p", {"o", "a", "i"}},
    {"nut", "n _ t", {"u", "e", "a"}},
    {"cap", "c _ p", {"a", "i", "o"}},
    {"net", "n _ t", {"e", "a", "u"}},
    {"pot", "p _ t", {"o", "i", "a"}},
};

static const WordRound reading_rounds[WORD_COUNT] = {
    {"cat", "Which word?", {"cat", "cap", "cup"}},
    {"dog", "Which word?", {"dog", "dig", "dot"}},
    {"hen", "Which word?", {"hen", "hop", "hut"}},
    {"pig", "Which word?", {"pig", "pin", "pot"}},
    {"sun", "Which word?", {"sun", "sit", "sap"}},
    {"bed", "Which word?", {"bed", "bad", "bud"}},
    {"bug", "Which word?", {"bug", "bag", "big"}},
    {"fox", "Which word?", {"fox", "fix", "fax"}},
    {"map", "Which word?", {"map", "mop", "mug"}},
    {"lip", "Which word?", {"lip", "log", "lap"}},
    {"mop", "Which word?", {"mop", "map", "mug"}},
    {"nut", "Which word?", {"nut", "net", "not"}},
    {"cap", "Which word?", {"cap", "cat", "cup"}},
    {"net", "Which word?", {"net", "nut", "nap"}},
    {"pot", "Which word?", {"pot", "pig", "pan"}},
};

static lv_obj_t *g_content;
static lv_obj_t *option_buttons[3];
static lv_obj_t *feedback_label;
static lv_timer_t *advance_timer;
static PhonicsMode current_mode;
static int round_order[WORD_COUNT];
static int round_number;
static int first_try_score;
static int correct_option;
static bool first_try;
static bool locked;
static uint32_t app_generation;
static uint8_t *image_data[3];
static lv_image_dsc_t image_descriptors[3];

static void show_picker(void);
static void show_round(void);

static void shuffle(int values[], int count)
{
    for (int i = count - 1; i > 0; i--) {
        int j = random(i + 1);
        int value = values[i];
        values[i] = values[j];
        values[j] = value;
    }
}

static void cancel_advance(void)
{
    if (advance_timer) {
        lv_timer_del(advance_timer);
        advance_timer = NULL;
    }
}

static void free_images(void)
{
    for (int i = 0; i < 3; i++) {
        if (image_data[i]) {
            lv_free(image_data[i]);
            image_data[i] = NULL;
        }
        memset(&image_descriptors[i], 0, sizeof(image_descriptors[i]));
    }
}

static bool load_image(int slot, const char *word, bool sound_choice)
{
    char path[48];
    snprintf(path, sizeof(path), "S:/phonics/%s/%s.bin",
             sound_choice ? "sounds" : "images", word);

    lv_fs_file_t file;
    if (lv_fs_open(&file, path, LV_FS_MODE_RD) != LV_FS_RES_OK) return false;

    lv_image_header_t header;
    uint32_t bytes_read = 0;
    bool ok = lv_fs_read(&file, &header, sizeof(header), &bytes_read) == LV_FS_RES_OK &&
              bytes_read == sizeof(header) && header.magic == LV_IMAGE_HEADER_MAGIC &&
              header.cf == LV_COLOR_FORMAT_RGB565;
    uint32_t data_size = ok ? header.stride * header.h : 0;
    uint8_t *data = data_size ? (uint8_t *)lv_malloc(data_size) : NULL;
    if (!data) ok = false;
    if (ok) {
        ok = lv_fs_read(&file, data, data_size, &bytes_read) == LV_FS_RES_OK &&
             bytes_read == data_size;
    }
    lv_fs_close(&file);

    if (!ok) {
        if (data) lv_free(data);
        return false;
    }

    image_data[slot] = data;
    image_descriptors[slot].header = header;
    image_descriptors[slot].data_size = data_size;
    image_descriptors[slot].data = data;
    return true;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                            const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

static lv_obj_t *make_button(lv_obj_t *parent, int x, int y, int width, int height,
                             lv_color_t color, const char *text, const lv_font_t *font)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_remove_style_all(button);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, width, height);
    lv_obj_set_style_bg_color(button, color, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);

    if (text) {
        lv_obj_t *label = make_label(button, text, font, NINO_COLOR_WHITE);
        lv_obj_center(label);
    }
    return button;
}

static void show_missing_assets(void)
{
    lv_obj_clean(g_content);
    lv_obj_t *title = make_label(g_content, "Phonics pictures are missing",
                                 &lv_font_montserrat_24, lv_color_hex(0x333333));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 70);

    lv_obj_t *help = make_label(g_content,
        "Copy the /phonics folder to the SD card,\nthen reopen Phonics.",
        &lv_font_montserrat_16, lv_color_hex(0x666666));
    lv_obj_set_style_text_align(help, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(help, LV_ALIGN_TOP_MID, 0, 120);
}

static void begin_mode(void *data)
{
    current_mode = (PhonicsMode)(intptr_t)data;
    int count = current_mode == MODE_SOUNDS ? SOUND_COUNT : WORD_COUNT;
    for (int i = 0; i < count; i++) round_order[i] = i;
    shuffle(round_order, count);
    round_number = 0;
    first_try_score = 0;
    show_round();
}

static void begin_mode_deferred(void *data)
{
    uintptr_t token = (uintptr_t)data;
    if ((uint32_t)(token >> 8) != app_generation || !g_content) return;
    begin_mode((void *)(token & 0xFF));
}

static void defer_begin_mode(PhonicsMode mode)
{
    uintptr_t token = ((uintptr_t)app_generation << 8) | (uintptr_t)mode;
    lv_async_call(begin_mode_deferred, (void *)token);
}

static void on_mode_pick(lv_event_t *event)
{
    defer_begin_mode((PhonicsMode)(intptr_t)lv_event_get_user_data(event));
}

static void on_skills(lv_event_t *)
{
    lv_async_call([](void *data) {
        if ((uint32_t)(uintptr_t)data != app_generation || !g_content) return;
        show_picker();
    }, (void *)(uintptr_t)app_generation);
}

static void add_game_header(const char *title)
{
    lv_obj_t *skills = make_button(g_content, 8, 5, 82, 30,
                                   lv_color_hex(0x6C7A89), "Skills",
                                   &lv_font_montserrat_14);
    lv_obj_add_event_cb(skills, on_skills, LV_EVENT_CLICKED, NULL);

    lv_obj_t *title_label = make_label(g_content, title, &lv_font_montserrat_20,
                                       lv_color_hex(0x333333));
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 8);

    char progress[20];
    snprintf(progress, sizeof(progress), "%d/%d  Stars:%d",
             round_number + 1, PHONICS_ROUNDS, first_try_score);
    lv_obj_t *progress_label = make_label(g_content, progress, &lv_font_montserrat_12,
                                          lv_color_hex(0x777777));
    lv_obj_align(progress_label, LV_ALIGN_TOP_RIGHT, -8, 13);
}

static void show_picture(lv_obj_t *parent, const char *word, int slot, bool sound_choice)
{
    if (load_image(slot, word, sound_choice)) {
        lv_obj_t *image = lv_image_create(parent);
        lv_image_set_src(image, &image_descriptors[slot]);
        lv_obj_center(image);
    } else {
        lv_obj_t *fallback = make_label(parent, word, &lv_font_montserrat_16,
                                        lv_color_hex(0x555555));
        lv_obj_center(fallback);
    }
}

static void on_choice(lv_event_t *event)
{
    if (locked) return;
    int selected = (int)(intptr_t)lv_event_get_user_data(event);

    if (selected != correct_option) {
        first_try = false;
        lv_obj_set_style_bg_color(option_buttons[selected], NINO_COLOR_DANGER, 0);
        nino_anim_shake(option_buttons[selected]);
        lv_label_set_text(feedback_label, "Try again. Say each sound slowly.");
        lv_obj_set_style_text_color(feedback_label, NINO_COLOR_DANGER, 0);
        return;
    }

    locked = true;
    if (first_try) first_try_score++;
    lv_obj_set_style_bg_color(option_buttons[selected], NINO_COLOR_SUCCESS, 0);
    lv_obj_set_style_text_color(feedback_label, NINO_COLOR_SUCCESS, 0);

    char feedback[64];
    if (current_mode == MODE_SOUNDS) {
        const SoundGroup &group = sound_groups[round_order[round_number]];
        snprintf(feedback, sizeof(feedback), "Yes! It starts with %s.", group.sound);
    } else {
        const WordRound *bank = current_mode == MODE_VOWELS ? vowel_rounds : reading_rounds;
        const WordRound &round = bank[round_order[round_number]];
        snprintf(feedback, sizeof(feedback), current_mode == MODE_VOWELS
                 ? "Blend it: %s. Great job!" : "You read %s!", round.word);
    }
    lv_label_set_text(feedback_label, feedback);

    advance_timer = lv_timer_create([](lv_timer_t *timer) {
        (void)timer;
        advance_timer = NULL;
        round_number++;
        if (round_number < PHONICS_ROUNDS) {
            show_round();
            return;
        }

        int stars = (first_try_score * 3 + PHONICS_ROUNDS / 2) / PHONICS_ROUNDS;
        if (stars < 1) stars = 1;
        char summary[48];
        snprintf(summary, sizeof(summary), "%d of %d on the first try!",
                 first_try_score, PHONICS_ROUNDS);
        lv_obj_t *completion = nino_congrats_create(
            g_content, "Phonics Path Complete!", stars, summary,
            NINO_COLOR_PHONICS, 190, [](lv_event_t *) {
            defer_begin_mode(current_mode);
        });
        lv_obj_set_pos(completion, -8, -8);
    }, 850, NULL);
    lv_timer_set_repeat_count(advance_timer, 1);
}

static void show_sound_round(void)
{
    uint32_t load_started = millis();
    const SoundGroup &group = sound_groups[round_order[round_number]];
    char prompt[32];
    snprintf(prompt, sizeof(prompt), "%c says %s", (char)(group.letter - 32), group.sound);
    add_game_header(prompt);

    lv_obj_t *instruction = make_label(g_content, "Tap a picture that starts with this sound",
                                       &lv_font_montserrat_14, lv_color_hex(0x666666));
    lv_obj_align(instruction, LV_ALIGN_TOP_MID, 0, 40);

    int group_choices[3] = {round_order[round_number], 0, 0};
    do { group_choices[1] = random(SOUND_COUNT); } while (group_choices[1] == group_choices[0]);
    do { group_choices[2] = random(SOUND_COUNT); }
    while (group_choices[2] == group_choices[0] || group_choices[2] == group_choices[1]);
    shuffle(group_choices, 3);

    for (int i = 0; i < 3; i++) {
        option_buttons[i] = make_button(g_content, 14 + i * 150, 68, 136, 112,
                                        NINO_COLOR_WHITE, NULL, NULL);
        lv_obj_set_style_border_width(option_buttons[i], 2, 0);
        lv_obj_set_style_border_color(option_buttons[i], lv_color_hex(0xD7DEE5), 0);
        lv_obj_add_event_cb(option_buttons[i], on_choice, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        int group_index = group_choices[i];
        const char *word = sound_groups[group_index].words[random(3)];
        show_picture(option_buttons[i], word, i, true);
        if (group_index == round_order[round_number]) correct_option = i;
    }
    Serial.printf("[Phonics] Loaded 3 pictures in %lu ms, heap %u\n",
                  millis() - load_started, ESP.getFreeHeap());
}

static void show_word_round(void)
{
    const WordRound *bank = current_mode == MODE_VOWELS ? vowel_rounds : reading_rounds;
    const WordRound &round = bank[round_order[round_number]];
    add_game_header(current_mode == MODE_VOWELS ? "Build a Word" : "Read the Word");

    lv_obj_t *picture = lv_obj_create(g_content);
    lv_obj_remove_style_all(picture);
    lv_obj_set_pos(picture, 24, 74);
    lv_obj_set_size(picture, 136, 112);
    lv_obj_set_style_bg_color(picture, NINO_COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(picture, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(picture, 14, 0);
    lv_obj_clear_flag(picture, LV_OBJ_FLAG_SCROLLABLE);
    show_picture(picture, round.word, 0, false);

    lv_obj_t *prompt = make_label(g_content, round.prompt, &lv_font_montserrat_24,
                                  lv_color_hex(0x333333));
    lv_obj_set_width(prompt, 270);
    lv_obj_set_style_text_align(prompt, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(prompt, 178, 45);

    int choice_order[3] = {0, 1, 2};
    shuffle(choice_order, 3);
    for (int i = 0; i < 3; i++) {
        int choice = choice_order[i];
        option_buttons[i] = make_button(g_content, 190, 83 + i * 52, 245, 44,
                                        NINO_COLOR_PRIMARY, round.choices[choice],
                                        current_mode == MODE_VOWELS
                                            ? &lv_font_montserrat_24
                                            : &lv_font_montserrat_20);
        lv_obj_add_event_cb(option_buttons[i], on_choice, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
        if (choice == 0) correct_option = i;
    }
}

static void show_round(void)
{
    cancel_advance();
    lv_obj_clean(g_content);
    free_images();
    for (int i = 0; i < 3; i++) option_buttons[i] = NULL;
    locked = false;
    first_try = true;

    if (current_mode == MODE_SOUNDS) show_sound_round();
    else show_word_round();

    feedback_label = make_label(g_content, "", &lv_font_montserrat_16,
                                lv_color_hex(0x666666));
    lv_obj_set_width(feedback_label, 450);
    lv_obj_set_style_text_align(feedback_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(feedback_label, LV_ALIGN_BOTTOM_MID, 0, -12);
}

static void show_picker(void)
{
    cancel_advance();
    lv_obj_clean(g_content);
    free_images();

    lv_obj_t *title = make_label(g_content, "Choose your phonics step",
                                 &lv_font_montserrat_24, lv_color_hex(0x333333));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_t *subtitle = make_label(g_content, "Sounds first, then blend and read",
                                    &lv_font_montserrat_14, lv_color_hex(0x777777));
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 43);

    static const char *titles[] = {"Sound Match", "Build Words", "Read Words"};
    static const char *steps[] = {"1", "2", "3"};
    static const char *details[] = {
        "Match first\nsounds",
        "Add short\nvowels",
        "Read CVC\nwords",
    };
    static const uint32_t colors[] = {0x00A8A8, 0x5B6FCE, 0xE66A6A};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *card = make_button(g_content, 12 + i * 150, 72, 140, 172,
                                     lv_color_hex(colors[i]), NULL, NULL);
        lv_obj_add_event_cb(card, on_mode_pick, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *step = make_label(card, steps[i], &lv_font_montserrat_28, NINO_COLOR_WHITE);
        lv_obj_align(step, LV_ALIGN_TOP_MID, 0, 14);
        lv_obj_t *name = make_label(card, titles[i], &lv_font_montserrat_16, NINO_COLOR_WHITE);
        lv_obj_align(name, LV_ALIGN_TOP_MID, 0, 61);
        lv_obj_t *detail = make_label(card, details[i], &lv_font_montserrat_14, NINO_COLOR_WHITE);
        lv_obj_set_style_text_align(detail, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(detail, LV_ALIGN_TOP_MID, 0, 96);
    }
}

void app_phonics_create(lv_obj_t *content)
{
    app_generation++;
    g_content = content;
    advance_timer = NULL;
    memset(image_data, 0, sizeof(image_data));
    memset(image_descriptors, 0, sizeof(image_descriptors));
    lv_obj_set_style_bg_color(content, lv_color_hex(0xFFF9EE), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(content, [](lv_event_t *) {
        cancel_advance();
        free_images();
        g_content = NULL;
        app_generation++;
    }, LV_EVENT_DELETE, NULL);

    if (!nino_storage_exists("/phonics/images/cat.bin")) {
        show_missing_assets();
        return;
    }
    show_picker();
}
