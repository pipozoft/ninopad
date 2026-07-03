#include "storage.h"
#include "utils/sd_utils.h"
#include <SD.h>
#include <Arduino.h>

bool nino_storage_read(const char *path, JsonDocument &doc)
{
    if (!nino_sd_is_mounted()) return false;
    if (!SD.exists(path)) return false;
    File f = SD.open(path, FILE_READ);
    if (!f) return false;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.printf("[storage] read error '%s': %s\n", path, err.c_str());
        return false;
    }
    return true;
}

bool nino_storage_write(const char *path, const JsonDocument &doc)
{
    if (!nino_sd_is_mounted()) return false;

    String dir = String(path);
    int slash = dir.lastIndexOf('/');
    if (slash > 0) {
        String parent = dir.substring(0, slash);
        SD.mkdir(parent);
    }

    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    size_t written = serializeJson(doc, f);
    f.close();
    if (written == 0) {
        Serial.printf("[storage] write error '%s'\n", path);
        return false;
    }
    return true;
}

bool nino_storage_exists(const char *path)
{
    if (!nino_sd_is_mounted()) return false;
    return SD.exists(path);
}

bool nino_storage_delete(const char *path)
{
    if (!nino_sd_is_mounted()) return false;
    return SD.remove(path);
}
