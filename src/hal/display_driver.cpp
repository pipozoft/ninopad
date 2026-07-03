/**
 * display_driver.cpp — ST7796 init, LVGL v9 flush_cb, PWM backlight.
 * Pinout matches the original ninopad.ino starter.
 */
#include "display_driver.h"
#include <Arduino_GFX_Library.h>
#include <esp32-hal-ledc.h>

#define NINO_HOR_RES  480
#define NINO_VER_RES  320

#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_SCK   14
#define TFT_MOSI  13
#define TFT_MISO  12
#define LCD_BL    27

// Single partial buffer — no PSRAM; 160 rows × 480px × 2 bytes = 153 KB
static uint8_t *buf1 = nullptr;

static Arduino_DataBus *bus = nullptr;
static Arduino_GFX     *gfx  = nullptr;

volatile uint32_t nino_frame_count = 0;

// ST7796 command codes (ILI9341-compatible)
#define ST77XX_CASET  0x2A
#define ST77XX_RASET  0x2B
#define ST77XX_RAMWR  0x2C

extern "C" void nino_backlight_set_pct(uint8_t pct)
{
    // ESP32-S3 LEDC PWM on GPIO27 — arduino-esp32 v2.0.x API.
    static bool started = false;
    if (!started) {
        ledcSetup(0, 5000, 8);   // channel 0, 5 kHz, 8-bit resolution
        ledcAttachPin(LCD_BL, 0);
        started = true;
    }
    if (pct > 100) pct = 100;
    ledcWrite(0, (uint32_t)255 * pct / 100);
}

extern "C" uint32_t nino_display_hor_res(void) { return NINO_HOR_RES; }

extern "C" uint32_t nino_display_ver_res(void) { return NINO_VER_RES; }

extern "C" void *nino_display_gfx(void) { return (void *)gfx; }

void nino_disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
    lv_display_flush_ready(disp);

    if (area->y1 == 0) nino_frame_count++;
}

extern "C" void nino_display_init(void)
{
    /* Force spi_num=3 so we use SPI3 (VSPI), not SPI1 (flash controller).
       The GFX library's internal #define VSPI 1 is wrong — Arduino core uses VSPI 3. */
    bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO, 3);
    gfx = new Arduino_ST7796(bus, TFT_RST, 1 /* rotation */, false /* IPS — true sends INVON (0x21) but some panels need INVOFF (0x20) */);

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);  // full bright until PWM takes over
    nino_backlight_set_pct(100);

    gfx->begin();
    Serial.println("[disp] gfx begin done");
    gfx->fillScreen(0x0000);
    Serial.println("[disp] fillScreen done");

    uint32_t buf_rows = 160;
    uint32_t buf_size = NINO_HOR_RES * buf_rows * 2;
    buf1 = (uint8_t *)malloc(buf_size);
    if (!buf1) {
        Serial.println("[disp] buf1 malloc FAILED — retrying with 60 rows");
        buf_rows = 60;
        buf_size = NINO_HOR_RES * buf_rows * 2;
        buf1 = (uint8_t *)malloc(buf_size);
    }
    if (!buf1) {
        Serial.println("[disp] buf1 malloc FAILED again");
        return;
    }
    Serial.printf("[disp] buffer = %d x %d px (%d bytes)\n", NINO_HOR_RES, buf_rows, buf_size);

    lv_display_t *disp = lv_display_create(NINO_HOR_RES, NINO_VER_RES);
    if (!disp) { Serial.println("[disp] lv_display_create FAILED"); return; }
    Serial.println("[disp] lv_display_create OK");
    lv_display_set_flush_cb(disp, nino_disp_flush_cb);
    Serial.println("[disp] flush_cb set");
    lv_display_set_buffers(disp, buf1, NULL, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial.printf("[disp] buffers set (PARTIAL, single %d-row buffer)\n", buf_rows);
    // Verify: lv_display_get_next(NULL) == disp?
    if (lv_display_get_next(NULL) == disp)
        Serial.println("[disp] get_next matches");
    else
        Serial.println("[disp] get_next MISMATCH");
}