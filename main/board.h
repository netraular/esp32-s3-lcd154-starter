/*
 * board.h — Pines y utilidades de la placa Waveshare ESP32-S3-LCD-1.54
 *
 * Fuente: esquema oficial de Waveshare
 * https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54/Resources-And-Documents
 */
#pragma once

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

// ---- Pantalla ST7789 240x240 (SPI) ----
#define PIN_LCD_SCLK     GPIO_NUM_38
#define PIN_LCD_MOSI     GPIO_NUM_39
#define PIN_LCD_CS       GPIO_NUM_21
#define PIN_LCD_DC       GPIO_NUM_45
#define PIN_LCD_RST      GPIO_NUM_40
#define PIN_LCD_BL       GPIO_NUM_46   // retroiluminación (PWM)

// ---- Botones (activos a nivel BAJO: pulsado = 0) ----
// Orden físico en la placa: BOOT, PWR, +
#define PIN_BTN_BOOT     GPIO_NUM_0    // botón "BOOT"
#define PIN_BTN_PWR      GPIO_NUM_5    // botón "PWR"
#define PIN_BTN_PLUS     GPIO_NUM_4    // botón "+"

// ---- Alimentación / batería ----
#define PIN_BAT_EN       GPIO_NUM_2    // 1 = mantener encendida con batería
#define PIN_BAT_ADC      GPIO_NUM_1    // tensión batería / 3 (ADC1 canal 0)
#define PIN_CHARGING     GPIO_NUM_3    // 0 = cargando

// ---- Bus I2C (IMU, códecs de audio) ----
#define PIN_I2C_SCL      GPIO_NUM_41
#define PIN_I2C_SDA      GPIO_NUM_42
#define PIN_IMU_INT      GPIO_NUM_6

// ---- Audio: códec ES8311 (altavoz) y ES7210 (micrófonos) por I2S ----
#define PIN_AUDIO_PA     GPIO_NUM_7    // habilita el amplificador NS4150B
#define PIN_I2S_MCLK     GPIO_NUM_8
#define PIN_I2S_BCLK     GPIO_NUM_9
#define PIN_I2S_WS       GPIO_NUM_10
#define PIN_I2S_DIN      GPIO_NUM_11   // micrófonos -> ESP32
#define PIN_I2S_DOUT     GPIO_NUM_12   // ESP32 -> altavoz

// ---- microSD (SDMMC 1 bit) ----
#define PIN_SD_CLK       GPIO_NUM_16
#define PIN_SD_CMD       GPIO_NUM_15
#define PIN_SD_D0        GPIO_NUM_17

// Direcciones I2C (7 bits) de los chips de la placa
#define I2C_ADDR_ES8311  0x18
#define I2C_ADDR_ES7210  0x40
#define I2C_ADDR_QMI8658 0x6B

/** Mantiene la placa encendida cuando funciona con batería. Llamar lo primero. */
void board_power_hold(void);

/** Corta la alimentación de batería (con USB conectado la placa sigue encendida). */
void board_power_off(void);

/** Bus I2C compartido. Se crea la primera vez que se pide. */
i2c_master_bus_handle_t board_i2c_bus(void);
