#include "wifi_utils.h"
#include "sd_utils.h"
#include <SD.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <time.h>

#define WIFI_CONFIG_PATH "/wifi_networks.json"
#define WIFI_TIMEOUT_MS  15000
#define NTP_SERVER       "time.google.com"

bool nino_wifi_connect_from_sd(void)
{
    if (!nino_sd_is_mounted())
    {
        Serial.println("[WiFi] SD not mounted, can't read config");
        return false;
    }

    File f = SD.open(WIFI_CONFIG_PATH, FILE_READ);
    if (!f)
    {
        Serial.printf("[WiFi] Could not open %s\n", WIFI_CONFIG_PATH);
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err)
    {
        Serial.printf("[WiFi] JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    if (arr.size() == 0)
    {
        Serial.println("[WiFi] No networks in config");
        return false;
    }

    for (JsonVariant item : arr)
    {
        const char *ssid = item["ssid"];
        const char *password = item["password"];

        if (!ssid || !password) continue;

        Serial.printf("[WiFi] Connecting to %s...\n", ssid);
        WiFi.begin(ssid, password);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED)
        {
            if (millis() - start > WIFI_TIMEOUT_MS) break;
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("[WiFi] Connected to %s, IP: %s\n",
                          ssid, WiFi.localIP().toString().c_str());
            return true;
        }

        Serial.printf("[WiFi] Failed to connect to %s\n", ssid);
        WiFi.disconnect(true);
    }

    Serial.println("[WiFi] All networks failed");
    return false;
}

bool nino_wifi_is_connected(void)
{
    return WiFi.status() == WL_CONNECTED;
}

void nino_wifi_disconnect(void)
{
    WiFi.disconnect(true);
}

bool nino_ntp_sync(int timezone_offset)
{
    (void)timezone_offset;
    if (!nino_wifi_is_connected())
    {
        Serial.println("[NTP] No WiFi, can't sync");
        return false;
    }

    configTime(0, 0, NTP_SERVER);
    setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
    tzset();
    Serial.println("[NTP] Syncing...");

    for (int i = 0; i < 20; i++)
    {
        time_t now = time(nullptr);
        if (now > 100000)
        {
            struct tm *ti = localtime(&now);
            Serial.printf("[NTP] Synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                          ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
                          ti->tm_hour, ti->tm_min, ti->tm_sec);
            return true;
        }
        delay(500);
    }

    Serial.println("[NTP] Sync timeout");
    return false;
}

bool nino_ntp_has_time(void)
{
    time_t now = time(nullptr);
    return now > 100000;
}
