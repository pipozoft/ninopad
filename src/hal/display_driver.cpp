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

// Two partial buffers in PSRAM give smoother 60fps refresh than one large buffer.
static uint8_t *buf1 = nullptr;
static uint8_t *buf2 = nullptr;

static Arduino_DataBus *bus = nullptr;
static Arduino_GFX     *gfx  = nullptr;

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

static void nino_disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
    lv_display_flush_ready(disp);
}

extern "C" void nino_display_init(void)
{
    bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
    gfx = new Arduino_ST7796(bus, TFT_RST, 1 /* rotation */, true /* IPS */);

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);  // full bright until PWM takes over
    nino_backlight_set_pct(100);

    gfx->begin();
    gfx->fillScreen(0x0000);

    uint32_t buf_size = NINO_HOR_RES * 40 * 2; // 2 bytes/pixel RGB565
    buf1 = (uint8_t *)malloc(buf_size);
    buf2 = (uint8_t *)malloc(buf_size);

    lv_display_t *disp = lv_display_create(NINO_HOR_RES, NINO_VER_RES);
    lv_display_set_flush_cb(disp, nino_disp_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
}