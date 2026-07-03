/**
 * anim_utils.cpp — pulse, bounce, fade animations backed by lv_anim.
 * Scale uses LVGL v9 style transform_scale (256 = 100%).
 */
#include "anim_utils.h"
#include <Arduino.h>

// LVGL v9 helper: set transform_scale via style. The exec_cb receives int32_t.
static void anim_exec_scale(void *obj, int32_t v)
{
    static int anim_cnt = 0;
    anim_cnt++;
    if ((anim_cnt % 10) == 0) Serial.print("S");
    lv_obj_set_style_transform_scale((lv_obj_t *)obj, v, 0);
}

static void anim_exec_opa(void *obj, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, v, 0);
}

void nino_anim_pulse(lv_obj_t *obj, uint32_t ms_period, uint32_t scale_max_q8, int32_t repeat_count)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 256, (int32_t)scale_max_q8);
    lv_anim_set_time(&a, ms_period);
    lv_anim_set_playback_time(&a, ms_period);
    lv_anim_set_repeat_count(&a, repeat_count);
    lv_anim_set_exec_cb(&a, anim_exec_scale);
    lv_anim_start(&a);
}

void nino_anim_bounce(lv_obj_t *obj, uint32_t ms_duration, uint32_t scale_max_q8,
                      lv_anim_completed_cb_t done_cb, void *user_data)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 256, (int32_t)scale_max_q8);
    lv_anim_set_time(&a, ms_duration / 2);
    lv_anim_set_playback_time(&a, ms_duration / 2);
    // v9: playback_time > 0 automatically mirrors forward then back.
    lv_anim_set_exec_cb(&a, anim_exec_scale);
    if (done_cb) {
        lv_anim_set_completed_cb(&a, done_cb);
        lv_anim_set_user_data(&a, user_data);
    }
    lv_anim_start(&a);
}

void nino_anim_fade_in(lv_obj_t *scr, uint32_t ms)
{
    lv_obj_set_style_opa(scr, 0, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, scr);
    lv_anim_set_values(&a, 0, LV_OPA_COVER);
    lv_anim_set_time(&a, ms);
    lv_anim_set_exec_cb(&a, anim_exec_opa);
    lv_anim_start(&a);
}

void nino_anim_fade_out(lv_obj_t *scr, uint32_t ms, lv_anim_completed_cb_t done_cb, void *user_data)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, scr);
    lv_anim_set_values(&a, LV_OPA_COVER, 0);
    lv_anim_set_time(&a, ms);
    lv_anim_set_exec_cb(&a, anim_exec_opa);
    if (done_cb) {
        lv_anim_set_completed_cb(&a, done_cb);
        lv_anim_set_user_data(&a, user_data);
    }
    lv_anim_start(&a);
}