/*
 * Pin assignments and helpers for the Waveshare ESP32-S3-LCD-1.54.
 *
 * Source: the official Waveshare schematic.
 * https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54/Resources-And-Documents
 */
#pragma once

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

// ---- ST7789 240x240 display (SPI) ----
#define PIN_LCD_SCLK     GPIO_NUM_38
#define PIN_LCD_MOSI     GPIO_NUM_39
#define PIN_LCD_CS       GPIO_NUM_21
#define PIN_LCD_DC       GPIO_NUM_45
#define PIN_LCD_RST      GPIO_NUM_40
#define PIN_LCD_BL       GPIO_NUM_46   // Backlight (PWM).

// ---- Buttons (active low), in physical order: BOOT, PWR, + ----
#define PIN_BTN_BOOT     GPIO_NUM_0
#define PIN_BTN_PWR      GPIO_NUM_5
#define PIN_BTN_PLUS     GPIO_NUM_4

// ---- Power and battery ----
#define PIN_BAT_EN       GPIO_NUM_2    // 1 keeps the board on under battery power.
#define PIN_BAT_ADC      GPIO_NUM_1    // Battery voltage / 3 (ADC1 channel 0).
#define PIN_CHARGING     GPIO_NUM_3    // 0 while charging.

// ---- I2C bus (IMU and audio codecs) ----
#define PIN_I2C_SCL      GPIO_NUM_41
#define PIN_I2C_SDA      GPIO_NUM_42
#define PIN_IMU_INT      GPIO_NUM_6

// ---- I2S audio: ES8311 (speaker) and ES7210 (microphones) ----
#define PIN_AUDIO_PA     GPIO_NUM_7    // NS4150B amplifier enable.
#define PIN_I2S_MCLK     GPIO_NUM_8
#define PIN_I2S_BCLK     GPIO_NUM_9
#define PIN_I2S_WS       GPIO_NUM_10
#define PIN_I2S_DIN      GPIO_NUM_11   // Microphones -> ESP32.
#define PIN_I2S_DOUT     GPIO_NUM_12   // ESP32 -> speaker.

// ---- microSD (1-bit SDMMC) ----
#define PIN_SD_CLK       GPIO_NUM_16
#define PIN_SD_CMD       GPIO_NUM_15
#define PIN_SD_D0        GPIO_NUM_17

// 7-bit I2C addresses of onboard chips.
#define I2C_ADDR_ES8311  0x18
#define I2C_ADDR_ES7210  0x40
#define I2C_ADDR_QMI8658 0x6B

/** Latch battery power. Call this first at startup. */
void board_power_hold(void);

/**
 * Turn off the board without returning. If power remains on (for example,
 * from USB), enter deep sleep and restart when PWR is pressed.
 * Call display_sleep() first.
 */
void board_power_off(void) __attribute__((noreturn));

/** Return the shared I2C bus, initializing it on first use. */
i2c_master_bus_handle_t board_i2c_bus(void);
