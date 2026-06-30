/**
 * settings.h — child name persistence (NVS Preferences-backed; SD/JSON deferred).
 * The spec asks for settings.json on SD; this API abstracts storage so it can be
 * swapped later without touching UI code.
 */
#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Returns true if a name was stored (and writes it into buf, NUL-terminated).
bool nino_settings_get_name(char *buf, size_t buf_size);
// Stores `name` (truncated to buf size). Returns true on success.
bool nino_settings_set_name(const char *name);

#ifdef __cplusplus
}
#endif