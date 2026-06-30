/**
 * anim_utils.h — reusable LVGL v9 animations: pulse, bounce, screen fade/slide.
 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pulse scale on an obj (256 = 100%). Repeats count times (0 = infinite).
void nino_anim_pulse(lv_obj_t *obj, uint32_t ms_period, uint32_t scale_max_q8, int32_t repeat_count);

// Bounce-scale used for app-button taps: scale up to scale_max then back to 256.
// Calls done_cb (may be NULL) when finished so callee can advance screens.
void nino_anim_bounce(lv_obj_t *obj, uint32_t ms_duration, uint32_t scale_max_q8,
                      lv_anim_completed_cb_t done_cb, void *user_data);

// Fade a screen in (0 -> 255 opacity). Used when loading a new screen.
void nino_anim_fade_in(lv_obj_t *scr, uint32_t ms);
void nino_anim_fade_out(lv_obj_t *scr, uint32_t ms, lv_anim_completed_cb_t done_cb, void *user_data);

#ifdef __cplusplus
}
#endif