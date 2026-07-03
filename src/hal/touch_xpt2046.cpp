/**
 * touch_xpt2046.cpp — XPT2046 resistive touch via GPIO bit-bang.
 *
 * Shares the same SPI3 pins as the display (SCK=14, MOSI=13, MISO=12).
 * Takes them over as plain GPIOs, bit-bangs 24 SCLK cycles (8 cmd + 16
 * read) continuously, then reattaches them to SPI3 via GPIO matrix.
 *
 * CS=33, IRQ=36 — NOT on SPI3 CS (that's GPIO15 for the display).
 *
 * Raw ADC values (480×320 panel):
 *   X: 315 (right) to 3910 (left)  — inverted
 *   Y: 278 (bottom) to 3746 (top)  — inverted
 */
#include "touch_xpt2046.h"
#ifdef NINO_TOUCH_XPT2046

#include <Arduino.h>
#include "hal/display_driver.h"
#include <soc/gpio_struct.h>
#include <soc/gpio_sig_map.h>
#include <esp32-hal-matrix.h>

#define TOUCH_SCK   14
#define TOUCH_MOSI  13
#define TOUCH_MISO  12
#define TOUCH_CS    33
#define TOUCH_IRQ   36

#define XPT2046_X_CMD  0x90
#define XPT2046_Y_CMD  0xD0

/* Edge raw values (inverted: X dec right, Y dec down) */
#define X_RAW_RIGHT   315
#define X_RAW_LEFT   3910
#define Y_RAW_TOP    3746
#define Y_RAW_BOTTOM  278

static void take_pins(void)
{
    /* Detach SPI3 signals from GPIO matrix (out by pin, in by signal) */
    pinMatrixOutDetach(TOUCH_SCK,  false, false);
    pinMatrixOutDetach(TOUCH_MOSI, false, false);
    pinMatrixInDetach(VSPIQ_IN_IDX, false, false);

    /* Configure as plain GPIOs */
    pinMode(TOUCH_SCK,  OUTPUT);
    pinMode(TOUCH_MOSI, OUTPUT);
    pinMode(TOUCH_MISO, INPUT);

    digitalWrite(TOUCH_SCK, LOW);
}

static void release_pins(void)
{
    /* Reattach SPI3 signals via GPIO matrix (same as spiAttach* in core) */
    pinMatrixOutAttach(TOUCH_SCK,  VSPICLK_OUT_IDX, false, false);
    pinMatrixOutAttach(TOUCH_MOSI, VSPID_OUT_IDX,   false, false);
    pinMatrixInAttach(TOUCH_MISO,  VSPIQ_IN_IDX,    false);
}

/* Fast bit-bang via direct GPIO registers */

#define GPIO_SET(p)   do { GPIO.out_w1ts = ((uint32_t)1 << (p)); } while (0)
#define GPIO_CLR(p)   do { GPIO.out_w1tc = ((uint32_t)1 << (p)); } while (0)
#define GPIO_READ(p)  ((GPIO.in >> (p)) & 1)

/*
 * 24 SCLK total — 8 command bits + 16 read bits — continuous.
 * SCK ~500 kHz via delayMicroseconds(2) half-period.
 */
static uint16_t xpt2046_transfer(uint8_t cmd)
{
    uint16_t result = 0;

    for (int i = 7; i >= 0; i--)
    {
        if (cmd & (1 << i))
            GPIO_SET(TOUCH_MOSI);
        else
            GPIO_CLR(TOUCH_MOSI);

        GPIO_SET(TOUCH_SCK);
        delayMicroseconds(2);
        GPIO_CLR(TOUCH_SCK);
        delayMicroseconds(2);
    }

    for (int i = 0; i < 16; i++)
    {
        GPIO_SET(TOUCH_SCK);
        delayMicroseconds(2);
        result = (result << 1) | GPIO_READ(TOUCH_MISO);
        GPIO_CLR(TOUCH_SCK);
        delayMicroseconds(2);
    }

    return result;
}

static uint16_t xpt2046_read_adc(uint8_t cmd)
{
    take_pins();

    digitalWrite(TOUCH_CS, LOW);
    delayMicroseconds(5);
    uint16_t raw16 = xpt2046_transfer(cmd);
    digitalWrite(TOUCH_CS, HIGH);

    release_pins();

    return raw16 >> 3;
}

extern "C" void nino_touch_init(void)
{
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    pinMode(TOUCH_IRQ, INPUT_PULLUP);
}

extern "C" void nino_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (digitalRead(TOUCH_IRQ) != 0)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint16_t x_raw = xpt2046_read_adc(XPT2046_X_CMD);
    uint16_t y_raw = xpt2046_read_adc(XPT2046_Y_CMD);
    Serial.printf("T:%u,%u ", x_raw, y_raw);

    if (x_raw < X_RAW_RIGHT || x_raw > X_RAW_LEFT ||
        y_raw < Y_RAW_BOTTOM || y_raw > Y_RAW_TOP)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    data->state = LV_INDEV_STATE_PRESSED;

    int32_t h = (int32_t)nino_display_hor_res();   /* 480 */
    int32_t v = (int32_t)nino_display_ver_res();   /* 320 */

    /* Both axes inverted */
    int32_t mx = map(x_raw, X_RAW_RIGHT, X_RAW_LEFT, h - 1, 0);
    int32_t my = map(y_raw, Y_RAW_BOTTOM, Y_RAW_TOP, v - 1, 0);

    data->point.x = (mx < 0) ? 0 : (mx >= h) ? (h - 1) : mx;
    data->point.y = (my < 0) ? 0 : (my >= v) ? (v - 1) : my;
}

#endif /* NINO_TOUCH_XPT2046 */
