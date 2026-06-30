/*
 * ninopad.ino — Arduino IDE entry stub.
 *
 * When opened in Arduino IDE, the sketch must be in the folder
 * matching the .ino name. Add all src/*.cpp files as tabs and
 * ensure LVGL + Arduino_GFX + XPT2046_Touchscreen + ArduinoJson
 * libs are installed via Library Manager.
 *
 * For PlatformIO builds this file is ignored; src/main.cpp is used.
 */
#include <Arduino.h>

void setup()  { lv_init(); /* minimal stub — use PlatformIO */ }
void loop()   { delay(5); }