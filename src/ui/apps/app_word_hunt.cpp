#include "app_word_hunt.h"
#include "nino_colors.h"
#include "scr_congrats.h"
#include "storage/storage.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ctype.h>
#include <string.h>

#define WORD_LIST_PATH "/word_lists/dolch_words_all.json"
#define GRID_SIZE       6
#define TARGET_COUNT    4
#define MAX_SOURCE    160
#define MAX_WORD_LEN    6
#define CELL_SIZE      38
#define CELL_GAP        3
#define CELL_PITCH     (CELL_SIZE + CELL_GAP)
#define BOARD_SIZE     (GRID_SIZE * CELL_SIZE + (GRID_SIZE - 1) * CELL_GAP)

static char source_words[MAX_SOURCE][MAX_WORD_LEN + 1];
static int source_count;
static char targets[TARGET_COUNT][MAX_WORD_LEN + 1];
static bool target_found[TARGET_COUNT];
static char letters[GRID_SIZE][GRID_SIZE];
static bool fixed_cells[GRID_SIZE][GRID_SIZE];

static lv_obj_t *g_content;
static lv_obj_t *board_area;
static lv_obj_t *cells[GRID_SIZE][GRID_SIZE];
static lv_obj_t *cell_labels[GRID_SIZE][GRID_SIZE];
static lv_obj_t *word_labels[TARGET_COUNT];
static lv_obj_t *feedback_label;
static lv_obj_t *overlay;

static bool selecting;
static bool selection_valid;
static int start_row, start_col, end_row, end_col;

static bool copy_word(const char *input, char *output)
{
    int len = 0;
    if (!input) return false;

    while (*input) {
        unsigned char ch = (unsigned char)*input++;
        if (!isalpha(ch) || len >= MAX_WORD_LEN) return false;
        output[len++] = (char)toupper(ch);
    }
    if (len < 2 || len > MAX_WORD_LEN) return false;
    output[len] = '\0';
    return true;
}

static bool load_words(void)
{
    JsonDocument doc;
    if (!nino_storage_read(WORD_LIST_PATH, doc) || !doc.is<JsonArray>()) return false;

    source_count = 0;
    for (JsonObject item : doc.as<JsonArray>()) {
        bool enabled = item["enabled"].isNull() || item["enabled"].as<bool>();
        if (source_count >= MAX_SOURCE || !enabled) continue;

        char normalized[MAX_WORD_LEN + 1];
        if (!copy_word(item["word"].as<const char *>(), normalized)) continue;

        bool duplicate = false;
        for (int i = 0; i < source_count; i++) {
            if (strcmp(source_words[i], normalized) == 0) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) strcpy(source_words[source_count++], normalized);
    }
    return source_count >= TARGET_COUNT;
}

static void choose_targets(void)
{
    for (int i = 0; i < TARGET_COUNT; i++) {
        int pick;
        bool used;
        do {
            pick = random(source_count);
            used = false;
            for (int j = 0; j < i; j++) {
                if (strcmp(targets[j], source_words[pick]) == 0) used = true;
            }
        } while (used);
        strcpy(targets[i], source_words[pick]);
    }
}

static bool place_word(const char *word)
{
    int len = strlen(word);
    for (int attempt = 0; attempt < 100; attempt++) {
        bool vertical = random(2) == 1;
        int row = random(vertical ? GRID_SIZE - len + 1 : GRID_SIZE);
        int col = random(vertical ? GRID_SIZE : GRID_SIZE - len + 1);
        bool fits = true;

        for (int i = 0; i < len; i++) {
            int r = row + (vertical ? i : 0);
            int c = col + (vertical ? 0 : i);
            if (letters[r][c] && letters[r][c] != word[i]) {
                fits = false;
                break;
            }
        }
        if (!fits) continue;

        for (int i = 0; i < len; i++) {
            int r = row + (vertical ? i : 0);
            int c = col + (vertical ? 0 : i);
            letters[r][c] = word[i];
        }
        return true;
    }
    return false;
}

static void build_puzzle(void)
{
    choose_targets();
    bool complete = false;
    for (int restart = 0; restart < 20; restart++) {
        memset(letters, 0, sizeof(letters));
        bool placed = true;
        for (int i = 0; i < TARGET_COUNT; i++) {
            if (!place_word(targets[i])) {
                placed = false;
                break;
            }
        }
        if (placed) {
            complete = true;
            break;
        }
    }

    if (!complete) {
        memset(letters, 0, sizeof(letters));
        for (int i = 0; i < TARGET_COUNT; i++) {
            int len = strlen(targets[i]);
            int col = random(GRID_SIZE - len + 1);
            for (int j = 0; j < len; j++) letters[i][col + j] = targets[i][j];
        }
    }

    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            if (!letters[r][c]) letters[r][c] = 'A' + random(26);
        }
    }
}

static void paint_cells(void)
{
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            lv_obj_set_style_bg_color(cells[r][c],
                fixed_cells[r][c] ? lv_color_hex(0xA8E6CF) : lv_color_hex(0xFFFFFF), 0);
        }
    }
}

static bool selection_step(int *dr, int *dc, int *length)
{
    int row_delta = end_row - start_row;
    int col_delta = end_col - start_col;
    int abs_row = abs(row_delta);
    int abs_col = abs(col_delta);
    if (row_delta != 0 && col_delta != 0 && abs_row != abs_col) return false;

    *dr = (row_delta > 0) - (row_delta < 0);
    *dc = (col_delta > 0) - (col_delta < 0);
    *length = max(abs_row, abs_col) + 1;
    return true;
}

static void show_selection(void)
{
    paint_cells();
    int dr, dc, length;
    selection_valid = selection_step(&dr, &dc, &length);
    if (!selection_valid) return;

    for (int i = 0; i < length; i++) {
        int r = start_row + i * dr;
        int c = start_col + i * dc;
        if (!fixed_cells[r][c]) {
            lv_obj_set_style_bg_color(cells[r][c], lv_color_hex(0xFFF3B0), 0);
        }
    }
}

static bool point_to_cell(int *row, int *col)
{
    lv_point_t point;
    lv_area_t area;
    lv_indev_get_point(lv_indev_active(), &point);
    lv_obj_get_coords(board_area, &area);
    int x = point.x - area.x1;
    int y = point.y - area.y1;
    if (x < 0 || y < 0 || x >= BOARD_SIZE || y >= BOARD_SIZE) return false;

    *col = x / CELL_PITCH;
    *row = y / CELL_PITCH;
    return *row < GRID_SIZE && *col < GRID_SIZE;
}

static void update_word_labels(void)
{
    for (int i = 0; i < TARGET_COUNT; i++) {
        char text[MAX_WORD_LEN + 4];
        snprintf(text, sizeof(text), "%s%s", target_found[i] ? "* " : "  ", targets[i]);
        lv_label_set_text(word_labels[i], text);
        lv_obj_set_style_text_color(word_labels[i],
            target_found[i] ? NINO_COLOR_SUCCESS : lv_color_hex(0x333333), 0);
    }
}

static void start_new_puzzle(void)
{
    build_puzzle();
    memset(target_found, 0, sizeof(target_found));
    memset(fixed_cells, 0, sizeof(fixed_cells));
    selecting = false;
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            char text[2] = {letters[r][c], '\0'};
            lv_label_set_text(cell_labels[r][c], text);
        }
    }
    update_word_labels();
    paint_cells();
    lv_label_set_text(feedback_label, "Drag across a word");
}

static void on_new_puzzle(lv_event_t *)
{
    if (overlay) {
        lv_obj_del(overlay);
        overlay = nullptr;
    }
    start_new_puzzle();
}

static void finish_selection(void)
{
    if (!selection_valid) {
        paint_cells();
        return;
    }

    int dr, dc, length;
    selection_step(&dr, &dc, &length);
    char selected[MAX_WORD_LEN + 1];
    if (length > MAX_WORD_LEN) {
        paint_cells();
        return;
    }
    for (int i = 0; i < length; i++) {
        selected[i] = letters[start_row + i * dr][start_col + i * dc];
    }
    selected[length] = '\0';

    int match = -1;
    for (int i = 0; i < TARGET_COUNT && match < 0; i++) {
        if (target_found[i] || (int)strlen(targets[i]) != length) continue;
        bool forward = strcmp(selected, targets[i]) == 0;
        bool reverse = true;
        for (int j = 0; j < length; j++) {
            if (selected[j] != targets[i][length - j - 1]) reverse = false;
        }
        if (forward || reverse) match = i;
    }

    if (match < 0) {
        lv_label_set_text(feedback_label, "Try another word");
        paint_cells();
        return;
    }

    target_found[match] = true;
    for (int i = 0; i < length; i++) {
        fixed_cells[start_row + i * dr][start_col + i * dc] = true;
    }
    paint_cells();
    update_word_labels();
    lv_label_set_text(feedback_label, "You found one!");

    for (int i = 0; i < TARGET_COUNT; i++) if (!target_found[i]) return;
    overlay = nino_congrats_create(g_content, "Great word hunting!", 3,
                                    "You found every word.", NINO_COLOR_WORD_HUNT,
                                    180, on_new_puzzle);
}

static void on_board_event(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    int row, col;

    if (code == LV_EVENT_PRESSED) {
        if (!point_to_cell(&row, &col)) return;
        selecting = true;
        start_row = end_row = row;
        start_col = end_col = col;
        show_selection();
    } else if (code == LV_EVENT_PRESSING && selecting) {
        if (!point_to_cell(&row, &col)) return;
        if (row == end_row && col == end_col) return;
        end_row = row;
        end_col = col;
        show_selection();
    } else if ((code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) && selecting) {
        selecting = false;
        finish_selection();
    }
}

static void create_game_ui(lv_obj_t *content)
{
    board_area = lv_obj_create(content);
    lv_obj_remove_style_all(board_area);
    lv_obj_set_size(board_area, BOARD_SIZE, BOARD_SIZE);
    lv_obj_set_pos(board_area, 0, 8);
    lv_obj_add_flag(board_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(board_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(board_area, on_board_event, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(board_area, on_board_event, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(board_area, on_board_event, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_event_cb(board_area, on_board_event, LV_EVENT_PRESS_LOST, nullptr);

    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            cells[r][c] = lv_obj_create(board_area);
            lv_obj_set_size(cells[r][c], CELL_SIZE, CELL_SIZE);
            lv_obj_set_pos(cells[r][c], c * CELL_PITCH, r * CELL_PITCH);
            lv_obj_set_style_radius(cells[r][c], 8, 0);
            lv_obj_set_style_border_width(cells[r][c], 1, 0);
            lv_obj_set_style_border_color(cells[r][c], lv_color_hex(0xDDDDDD), 0);
            lv_obj_set_style_pad_all(cells[r][c], 0, 0);
            lv_obj_clear_flag(cells[r][c], LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(cells[r][c], LV_OBJ_FLAG_SCROLLABLE);

            cell_labels[r][c] = lv_label_create(cells[r][c]);
            char text[2] = {letters[r][c], '\0'};
            lv_label_set_text(cell_labels[r][c], text);
            lv_obj_set_style_text_font(cell_labels[r][c], &lv_font_montserrat_20, 0);
            lv_obj_set_style_text_color(cell_labels[r][c], lv_color_hex(0x333333), 0);
            lv_obj_center(cell_labels[r][c]);
            lv_obj_clear_flag(cell_labels[r][c], LV_OBJ_FLAG_CLICKABLE);
        }
    }

    lv_obj_t *prompt = lv_label_create(content);
    lv_label_set_text(prompt, "Find these words");
    lv_obj_set_style_text_font(prompt, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(prompt, lv_color_hex(0x555555), 0);
    lv_obj_set_pos(prompt, 270, 12);

    for (int i = 0; i < TARGET_COUNT; i++) {
        word_labels[i] = lv_label_create(content);
        lv_obj_set_style_text_font(word_labels[i], &lv_font_montserrat_20, 0);
        lv_obj_set_pos(word_labels[i], 278, 48 + i * 34);
    }

    feedback_label = lv_label_create(content);
    lv_label_set_text(feedback_label, "Drag across a word");
    lv_obj_set_width(feedback_label, 188);
    lv_obj_set_style_text_align(feedback_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(feedback_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(feedback_label, lv_color_hex(0x777777), 0);
    lv_obj_set_pos(feedback_label, 266, 200);

    lv_obj_t *new_button = lv_btn_create(content);
    lv_obj_set_size(new_button, 140, 36);
    lv_obj_set_pos(new_button, 290, 224);
    lv_obj_set_style_radius(new_button, 18, 0);
    lv_obj_set_style_shadow_width(new_button, 0, 0);
    lv_obj_set_style_bg_color(new_button, NINO_COLOR_WORD_HUNT, 0);
    lv_obj_add_event_cb(new_button, on_new_puzzle, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *new_label = lv_label_create(new_button);
    lv_label_set_text(new_label, "New Puzzle");
    lv_obj_set_style_text_font(new_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(new_label, lv_color_hex(0x333333), 0);
    lv_obj_center(new_label);

    update_word_labels();
    paint_cells();
}

void app_word_hunt_create(lv_obj_t *content)
{
    g_content = content;
    overlay = nullptr;
    selecting = false;
    memset(target_found, 0, sizeof(target_found));
    memset(fixed_cells, 0, sizeof(fixed_cells));
    lv_obj_set_style_bg_color(content, lv_color_hex(0xFFF9E8), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    if (!load_words()) {
        lv_obj_t *title = lv_label_create(content);
        lv_label_set_text(title, "Word list not found");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(title, NINO_COLOR_DANGER, 0);
        lv_obj_align(title, LV_ALIGN_CENTER, 0, -24);

        lv_obj_t *help = lv_label_create(content);
        lv_label_set_text(help, "Add /word_lists/dolch_words_all.json\nto the SD card.");
        lv_obj_set_style_text_font(help, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(help, lv_color_hex(0x555555), 0);
        lv_obj_set_style_text_align(help, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(help, LV_ALIGN_CENTER, 0, 28);
        return;
    }

    build_puzzle();
    create_game_ui(content);
}
