/**
 * display_driver.cpp — ST7796 init, LVGL v9 flush_cb, PWM backlight.
 *
 * Hardware:
 *   - MCU: ESP32 (ESP-WROOM-32) or ESP32-S3, no PSRAM on base target.
 *   - Display: ST7796 4" 480×320 RGB565 on SPI3 (VSPI).
 *   - Backlight: PWM via LEDC ch0, 5kHz, 8-bit on GPIO27.
 *
 * SPI3 note: Arduino_ESP32SPI(..., spi_num=3) — the GFX library's internal
 * #define VSPI 1 is wrong; ESP32 Arduino core uses VSPI = 3, not 1.
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

#ifndef NINO_TFT_SPI_HZ
#define NINO_TFT_SPI_HZ 20000000
#endif

#ifndef NINO_LVGL_BUF_ROWS
#define NINO_LVGL_BUF_ROWS 40
#endif

// Single partial buffer. More rows means fewer address-window resets per frame.
static uint8_t *buf1 = nullptr;

static Arduino_DataBus *bus = nullptr;
static Arduino_GFX     *gfx  = nullptr;

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
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    lv_color_format_t cf = lv_display_get_color_format(disp);
    uint32_t px_size = lv_color_format_get_size(cf);
    uint32_t row_bytes = w * px_size;
    uint32_t stride = lv_draw_buf_width_to_stride(w, cf);

    Arduino_ST7796 *tft = static_cast<Arduino_ST7796 *>(gfx);
    tft->startWrite();
    tft->writeAddrWindow(area->x1, area->y1, w, h);
    if (stride == row_bytes) {
        tft->writePixels((uint16_t *)px_map, w * h);
    } else {
        for (int32_t y = 0; y < h; y++) {
            tft->writePixels((uint16_t *)px_map, w);
            px_map += stride;
        }
    }
    tft->endWrite();

    lv_display_flush_ready(disp);
}

extern "C" void nino_display_init(void)
{
    /* Force spi_num=3 so we use SPI3 (VSPI), not SPI1 (flash controller).
       The GFX library's internal #define VSPI 1 is wrong — Arduino core uses VSPI 3. */
    bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO, 3 /* spi_num=3: VSPI, not SPI1 */);
    gfx = new Arduino_ST7796(bus, TFT_RST, 1 /* rotation */, false /* IPS=false → INVOFF (0x20); toggle to true if colors invert */);

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);  // full bright until PWM takes over
    nino_backlight_set_pct(100);

    gfx->begin(NINO_TFT_SPI_HZ);
    Serial.printf("[disp] gfx begin done (SPI %d Hz)\n", NINO_TFT_SPI_HZ);
    gfx->fillScreen(0x0000);
    Serial.println("[disp] fillScreen done");

    uint32_t buf_rows = NINO_LVGL_BUF_ROWS;
    uint32_t buf_size = NINO_HOR_RES * buf_rows * 2;
    while (!buf1 && buf_rows >= 20) {
        buf_size = NINO_HOR_RES * buf_rows * 2;
        buf1 = (uint8_t *)malloc(buf_size);
        if (!buf1) {
            Serial.printf("[disp] buf1 malloc FAILED at %d rows\n", buf_rows);
            buf_rows /= 2;
        }
    }
    if (!buf1) {
        Serial.println("[disp] buf1 malloc FAILED");
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
