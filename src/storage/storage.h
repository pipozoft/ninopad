#pragma once
#include <ArduinoJson.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Read a JSON file from the SD card into a JsonDocument.
// Path is relative to the SD root (e.g. "apps/word_spy/progress.json").
// Returns true on success. doc is left empty on failure.
bool nino_storage_read(const char *path, JsonDocument &doc);

// Write a JsonDocument to a JSON file on the SD card.
// Creates parent directories as needed.
// Returns true on success.
bool nino_storage_write(const char *path, const JsonDocument &doc);

// Returns true if the file exists on the SD card.
bool nino_storage_exists(const char *path);

// Delete a file. Returns true on success.
bool nino_storage_delete(const char *path);

#ifdef __cplusplus
}
#endif
