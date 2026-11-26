/*
 * app_imu.c — Acelerómetro y giroscopio QMI8658 por I2C.
 *
 * Muestra la aceleración (en g), el giro (en grados/s), la temperatura del
 * chip y un "nivel de burbuja" que se mueve al inclinar la placa.
 * Datasheet: busca "QMI8658" en la página de recursos de Waveshare.
 */
#include "apps.h"
#include "board.h"
#include "buttons.h"
#include "display.h"

#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "imu";

// Registros del QMI8658
#define REG_WHO_AM_I 0x00   // debe valer 0x05
#define REG_CTRL1    0x02
#define REG_CTRL2    0x03   // acelerómetro: escala y frecuencia
#define REG_CTRL3    0x04   // giroscopio: escala y frecuencia
#define REG_CTRL7    0x08   // activar sensores
#define REG_TEMP_L   0x33
#define REG_AX_L     0x35
#define REG_RESET    0x60

#define ACC_LSB_PER_G   8192.0f  // escala ±4 g
#define GYR_LSB_PER_DPS 64.0f    // escala ±512 °/s

static i2c_master_dev_handle_t s_dev;

static esp_err_t write_reg(uint8_t reg, uint8_t value)
{
    const uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
}

static esp_err_t read_regs(uint8_t reg, uint8_t *out, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, out, len, 100);
}

static bool imu_start(void)
{
    i2c_master_bus_handle_t bus = board_i2c_bus();
    uint8_t addr = I2C_ADDR_QMI8658;
    if (i2c_master_probe(bus, addr, 100) != ESP_OK) {
        addr = 0x6A;  // dirección alternativa según el pin SA0
        if (i2c_master_probe(bus, addr, 100) != ESP_OK) return false;
    }
    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    if (i2c_master_bus_add_device(bus, &dev_cfg, &s_dev) != ESP_OK) return false;

    uint8_t who = 0;
    read_regs(REG_WHO_AM_I, &who, 1);
    ESP_LOGI(TAG, "QMI8658 en 0x%02X, WHO_AM_I = 0x%02X", addr, who);
    if (who != 0x05) return false;

    write_reg(REG_RESET, 0xB0);           // reinicio por software
    vTaskDelay(pdMS_TO_TICKS(20));
    write_reg(REG_CTRL1, 0x40);           // autoincremento de dirección, little-endian
    write_reg(REG_CTRL2, 0x15);           // ±4 g, 250 Hz
    write_reg(REG_CTRL3, 0x55);           // ±512 °/s, 250 Hz
    write_reg(REG_CTRL7, 0x03);           // activar acelerómetro + giroscopio
    vTaskDelay(pdMS_TO_TICKS(50));
    return true;
}

static void imu_stop(void)
{
    if (s_dev) {
        write_reg(REG_CTRL7, 0x00);       // apagar sensores
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }
}

void app_imu(void)
{
    if (!imu_start()) {
        imu_stop();
        display_clear(COLOR_BLACK);
        display_header("Movimiento");
        display_text_center(100, "No se encuentra el QMI8658", 1, COLOR_RED);
        display_footer("BOOT volver");
        display_flush();
        button_event_t e;
        while (!(buttons_get_event(&e, portMAX_DELAY) && e.type == BTN_EVENT_CLICK && e.button == BTN_BOOT)) {
        }
        return;
    }

    float bx = 0, by = 0;  // posición suavizada de la burbuja
    for (;;) {
        button_event_t e;
        if (buttons_get_event(&e, 0) && e.type == BTN_EVENT_CLICK && e.button == BTN_BOOT) break;

        uint8_t raw[14];  // TEMP_L, TEMP_H, AX_L ... GZ_H
        if (read_regs(REG_TEMP_L, raw, sizeof(raw)) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        const float temp = (int16_t)(raw[0] | raw[1] << 8) / 256.0f;
        float a[3], g[3];
        for (int i = 0; i < 3; i++) {
            a[i] = (int16_t)(raw[2 + 2 * i] | raw[3 + 2 * i] << 8) / ACC_LSB_PER_G;
            g[i] = (int16_t)(raw[8 + 2 * i] | raw[9 + 2 * i] << 8) / GYR_LSB_PER_DPS;
        }
        const float total = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);

        display_clear(COLOR_BLACK);
        display_header("Movimiento");

        // Nivel de burbuja
        const int cx = 64, cy = 110, r = 52;
        display_circle(cx, cy, r, COLOR_GRAY, false);
        display_circle(cx, cy, 14, COLOR_DARK, false);
        display_rect(cx - r, cy, 2 * r, 1, COLOR_DARK);
        display_rect(cx, cy - r, 1, 2 * r, COLOR_DARK);
        float tx = a[0] * r, ty = a[1] * r;
        const float d = sqrtf(tx * tx + ty * ty);
        if (d > r - 8) { tx *= (r - 8) / d; ty *= (r - 8) / d; }
        bx += (tx - bx) * 0.3f;
        by += (ty - by) * 0.3f;
        const bool level = fabsf(a[0]) < 0.03f && fabsf(a[1]) < 0.03f;
        display_circle(cx + (int)bx, cy + (int)by, 7, level ? COLOR_GREEN : COLOR_YELLOW, true);

        // Valores numéricos
        static const char axis[3] = {'X', 'Y', 'Z'};
        int y = 40;
        display_text(128, y, "Acel. (g)", 1, COLOR_CYAN);
        y += 12;
        for (int i = 0; i < 3; i++, y += 11) display_printf(128, y, 1, COLOR_WHITE, "%c %+6.2f", axis[i], a[i]);
        display_printf(128, y, 1, COLOR_GRAY, "|a| %5.2f", total);
        y += 18;
        display_text(128, y, "Giro (°/s)", 1, COLOR_CYAN);
        y += 12;
        for (int i = 0; i < 3; i++, y += 11) display_printf(128, y, 1, COLOR_WHITE, "%c %+7.1f", axis[i], g[i]);
        y += 7;
        display_printf(128, y, 1, COLOR_ORANGE, "Temp %.1f °C", temp);

        display_text_center(178, level ? "¡Nivelada!" : "Inclina la placa", 2, level ? COLOR_GREEN : COLOR_WHITE);
        display_footer("BOOT volver");
        display_flush();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    imu_stop();
}
