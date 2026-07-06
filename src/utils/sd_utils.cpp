#include "sd_utils.h"
#include <SD.h>
#include <SPI.h>
#include <Arduino.h>

static SPIClass sdSPI(2);
static bool mounted = false;

bool nino_sd_mount(void)
{
    if (mounted) return true;

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);

    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    for (int attempt = 0; attempt < 3; attempt++)
    {
        delay(200 + attempt * 300);
        if (SD.begin(SD_CS, sdSPI, 10000000))
        {
            mounted = true;
            Serial.printf("[SD] Mounted OK (attempt %d)\n", attempt + 1);
            return true;
        }
        Serial.printf("[SD] Attempt %d failed\n", attempt + 1);
    }

    Serial.println("[SD] Failed after 3 attempts");
    return false;
}

bool nino_sd_unmount(void)
{
    if (!mounted) return true;
    SD.end();
    mounted = false;
    Serial.println("[SD] Unmounted");
    return true;
}

bool nino_sd_is_mounted(void)
{
    return mounted;
}
