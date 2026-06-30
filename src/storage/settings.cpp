/**
 * settings.cpp — NVS-backed implementation using Arduino Preferences.
 * TODO(spec): replace with SD + settings.json (ArduinoJson) — keep same API.
 */
#include "settings.h"
#include <Preferences.h>

#define NVS_NAMESPACE "ninopad"
#define KEY_NAME      "child_name"

static Preferences prefs;

bool nino_settings_get_name(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return false;
    buf[0] = 0;
    if (!prefs.begin(NVS_NAMESPACE, true)) return false;
    bool found = prefs.isKey(KEY_NAME);
    if (found) {
        prefs.getString(KEY_NAME, buf, buf_size);
        // Safety: ensure termination even if full.
        buf[buf_size - 1] = 0;
    }
    prefs.end();
    return found && buf[0] != 0;
}

bool nino_settings_set_name(const char *name)
{
    if (!name) return false;
    if (!prefs.begin(NVS_NAMESPACE, false)) return false;
    bool ok = prefs.putString(KEY_NAME, name) > 0;
    prefs.end();
    return ok;
}