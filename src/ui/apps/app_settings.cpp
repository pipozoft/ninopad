#include "app_settings.h"
#include "storage/settings.h"
#include "storage/storage.h"
#include "utils/sd_utils.h"
#include "hal/display_driver.h"
#include "nino_colors.h"

static lv_obj_t *name_ta;
static lv_obj_t *kb;

static void on_name_ready(lv_event_t *e)
{
    (void)e;
    const char *txt = lv_textarea_get_text(name_ta);
    if (txt && txt[0]) nino_settings_set_name(txt);
    if (kb && !lv_obj_has_flag(kb, LV_OBJ_FLAG_HIDDEN))
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

static void profile_tab_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 8, 0);

    lv_obj_t *intro = lv_label_create(parent);
    lv_label_set_text(intro, "Child's name:");
    lv_obj_set_style_text_color(intro, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(intro, &lv_font_montserrat_16, 0);

    name_ta = lv_textarea_create(parent);
    lv_obj_set_width(name_ta, 360);
    lv_textarea_set_max_length(name_ta, 32);
    lv_textarea_set_one_line(name_ta, true);
    lv_textarea_set_placeholder_text(name_ta, "tap to type name");

    char buf[64];
    if (nino_settings_get_name(buf, sizeof(buf)))
        lv_textarea_set_text(name_ta, buf);

    lv_obj_t *note = lv_label_create(parent);
    lv_label_set_text(note, "Tap \xE2\x9C\x93 on keyboard when done");
    lv_obj_set_style_text_color(note, lv_color_hex(0x999999), 0);
    lv_obj_set_style_text_font(note, &lv_font_montserrat_12, 0);

    kb = lv_keyboard_create(parent);
    lv_keyboard_set_textarea(kb, name_ta);
    lv_obj_add_event_cb(kb, on_name_ready, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, on_name_ready, LV_EVENT_CANCEL, NULL);
}

static lv_obj_t *word_list_container = NULL;

static void word_toggle_cb(lv_event_t *e)
{
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
    void *udata = lv_event_get_user_data(e);
    if (!udata) return;
    const char *word = (const char *)udata;
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    Serial.printf("[settings] word '%s' = %s\n", word, on ? "on" : "off");
}

static void words_tab_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 8, 0);

    if (!nino_sd_is_mounted()) {
        lv_obj_t *lab = lv_label_create(parent);
        lv_label_set_text(lab, "No SD card detected.\nInsert SD with word_lists/");
        lv_obj_set_style_text_color(lab, lv_color_hex(0x888888), 0);
        return;
    }

    if (!nino_storage_exists("/word_lists/pre_primer.json")) {
        lv_obj_t *lab = lv_label_create(parent);
        lv_label_set_text(lab, "word_lists/pre_primer.json not found.");
        lv_obj_set_style_text_color(lab, lv_color_hex(0x888888), 0);
        return;
    }

    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "Pre-Primer Words");
    lv_obj_set_style_text_color(header, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(header, &lv_font_montserrat_14, 0);

    JsonDocument doc;
    if (!nino_storage_read("/word_lists/pre_primer.json", doc)) {
        lv_obj_t *lab = lv_label_create(parent);
        lv_label_set_text(lab, "Error reading word list.");
        return;
    }

    JsonArray arr = doc.as<JsonArray>();
    word_list_container = lv_obj_create(parent);
    lv_obj_remove_style_all(word_list_container);
    lv_obj_set_width(word_list_container, 460);
    lv_obj_set_flex_flow(word_list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(word_list_container, 0, 0);

    for (JsonVariant item : arr) {
        const char *word = item["word"];
        bool enabled = item["enabled"] | true;
        if (!word) continue;

        lv_obj_t *row = lv_obj_create(word_list_container);
        lv_obj_remove_style_all(row);
        lv_obj_set_width(row, 460);
        lv_obj_set_height(row, 32);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_all(row, 4, 0);

        lv_obj_t *lab = lv_label_create(row);
        lv_label_set_text(lab, word);
        lv_obj_set_style_text_font(lab, &lv_font_montserrat_16, 0);
        lv_obj_set_flex_grow(lab, 1);

        lv_obj_t *sw = lv_switch_create(row);
        if (enabled) lv_obj_add_state(sw, LV_STATE_CHECKED);

        char *word_copy = (char *)lv_malloc(strlen(word) + 1);
        if (word_copy) {
            strcpy(word_copy, word);
            lv_obj_add_event_cb(sw, word_toggle_cb, LV_EVENT_VALUE_CHANGED, word_copy);
        }
    }
}

static void progress_tab_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 8, 0);

    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "App Progress");
    lv_obj_set_style_text_color(header, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);

    if (!nino_sd_is_mounted()) {
        lv_obj_t *lab = lv_label_create(parent);
        lv_label_set_text(lab, "Insert SD card to view progress.");
        lv_obj_set_style_text_color(lab, lv_color_hex(0x888888), 0);
        return;
    }

    static const char *progress_files[] = {
        "apps/word_spy/progress.json",
        "apps/luz_letters/progress.json",
        "apps/ten_frame/progress.json",
        "apps/snip_snip/progress.json",
    };

    bool any = false;
    for (unsigned i = 0; i < sizeof(progress_files) / sizeof(progress_files[0]); i++) {
        if (!nino_storage_exists(progress_files[i])) continue;
        any = true;

        JsonDocument doc;
        if (!nino_storage_read(progress_files[i], doc)) continue;

        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_remove_style_all(row);
        lv_obj_set_width(row, 460);
        lv_obj_set_height(row, 28);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_all(row, 4, 0);

        lv_obj_t *lab = lv_label_create(row);
        lv_label_set_text(lab, progress_files[i]);
        lv_obj_set_style_text_font(lab, &lv_font_montserrat_12, 0);
        lv_obj_set_flex_grow(lab, 1);

        int stars = doc["stars"] | 0;
        char stars_buf[16];
        snprintf(stars_buf, sizeof(stars_buf), "%d stars", stars);
        lv_obj_t *star_lab = lv_label_create(row);
        lv_label_set_text(star_lab, stars_buf);
        lv_obj_set_style_text_font(star_lab, &lv_font_montserrat_12, 0);
    }

    if (!any) {
        lv_obj_t *lab = lv_label_create(parent);
        lv_label_set_text(lab, "No progress data yet.\nComplete app activities to see progress.");
        lv_obj_set_style_text_color(lab, lv_color_hex(0x888888), 0);
    }
}

static void on_brightness_change(lv_event_t *e)
{
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    nino_backlight_set_pct((uint8_t)val);
}

static lv_obj_t *reset_mbox = NULL;

static void on_reset_do(lv_event_t *e)
{
    (void)e;
    Serial.println("[settings] Reset all progress (TODO)");
    if (reset_mbox) {
        lv_msgbox_close(reset_mbox);
        reset_mbox = NULL;
    }
}

static void on_reset_close(lv_event_t *e)
{
    (void)e;
    if (reset_mbox) {
        lv_msgbox_close(reset_mbox);
        reset_mbox = NULL;
    }
}

static void on_reset_tap(lv_event_t *e)
{
    (void)e;
    reset_mbox = lv_msgbox_create(lv_scr_act());
    lv_msgbox_add_title(reset_mbox, "Reset All Progress?");
    lv_msgbox_add_text(reset_mbox, "This will delete all app\nprogress data. Are you sure?");
    lv_obj_t *cancel = lv_msgbox_add_footer_button(reset_mbox, "Cancel");
    lv_obj_t *reset  = lv_msgbox_add_footer_button(reset_mbox, "Reset");
    lv_obj_add_event_cb(cancel, on_reset_close, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(reset,  on_reset_do,   LV_EVENT_CLICKED, NULL);
    lv_obj_center(reset_mbox);
}

static void system_tab_create(lv_obj_t *parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 8, 0);

    // Brightness
    lv_obj_t *bright_lab = lv_label_create(parent);
    lv_label_set_text(bright_lab, "Brightness");
    lv_obj_set_style_text_color(bright_lab, lv_color_hex(0x444444), 0);

    lv_obj_t *bright_sl = lv_slider_create(parent);
    lv_obj_set_width(bright_sl, 400);
    lv_slider_set_range(bright_sl, 10, 100);
    lv_slider_set_value(bright_sl, 100, LV_ANIM_OFF);
    lv_obj_add_event_cb(bright_sl, on_brightness_change, LV_EVENT_VALUE_CHANGED, NULL);

    // Language
    lv_obj_t *lang_lab = lv_label_create(parent);
    lv_obj_set_style_text_color(lang_lab, lv_color_hex(0x444444), 0);
    lv_obj_set_style_pad_top(lang_lab, 12, 0);
    lv_label_set_text(lang_lab, "Language");

    lv_obj_t *lang_dd = lv_dropdown_create(parent);
    lv_dropdown_set_options(lang_dd, "English\nEspa\xC3\xB1ol");
    lv_dropdown_set_selected(lang_dd, 0);
    lv_obj_set_width(lang_dd, 200);

    // Reset
    lv_obj_t *reset_lab = lv_label_create(parent);
    lv_obj_set_style_text_color(reset_lab, lv_color_hex(0x444444), 0);
    lv_obj_set_style_pad_top(reset_lab, 12, 0);
    lv_label_set_text(reset_lab, "Reset");

    lv_obj_t *reset_btn = lv_btn_create(parent);
    lv_obj_set_style_bg_color(reset_btn, lv_color_hex(0xCC4444), 0);
    lv_obj_set_width(reset_btn, 200);
    lv_obj_add_event_cb(reset_btn, on_reset_tap, LV_EVENT_CLICKED, NULL);

    lv_obj_t *reset_lab2 = lv_label_create(reset_btn);
    lv_label_set_text(reset_lab2, "Reset All Progress");
    lv_obj_center(reset_lab2);
}

void app_settings_create(lv_obj_t *content)
{
    lv_obj_set_style_bg_color(content, lv_color_hex(0xF5F5F5), 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *tv = lv_tabview_create(content);
    lv_obj_set_size(tv, 480, 276);
    lv_tabview_set_tab_bar_position(tv, LV_DIR_TOP);

    lv_obj_t *t_profile  = lv_tabview_add_tab(tv, "Profile");
    lv_obj_t *t_words    = lv_tabview_add_tab(tv, "Words");
    lv_obj_t *t_progress = lv_tabview_add_tab(tv, "Progress");
    lv_obj_t *t_system   = lv_tabview_add_tab(tv, "System");

    profile_tab_create(t_profile);
    words_tab_create(t_words);
    progress_tab_create(t_progress);
    system_tab_create(t_system);
}
