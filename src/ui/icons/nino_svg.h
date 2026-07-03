#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Draw a Heroicons-style SVG path into parent (50x50 target).
// Parses M/m/L/l/C/c/Q/q/A/a/Z/z, tessellates curves to segments.
// svg_path: the contents of the d="..." attribute.
// viewbox: the viewBox size (e.g. 24.0f for 24x24, 20.0f for 20x20).
void nino_svg_draw(lv_obj_t *parent, const char *svg_path, lv_color_t ink, int width, float viewbox);

#ifdef __cplusplus
}
#endif
