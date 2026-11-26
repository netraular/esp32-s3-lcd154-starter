/*
 * esp32-s3-lcd154-starter — Firmware de iniciación para Waveshare ESP32-S3-LCD-1.54
 *
 * Punto de entrada: app_main() lo llama ESP-IDF tras arrancar FreeRTOS.
 */
#include "apps.h"
#include "board.h"
#include "buttons.h"
#include "display.h"
#include "menu.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

static void app_power_off(void)
{
    display_clear(COLOR_BLACK);
    display_header("Apagar");
    display_text_center(80, "¡Hasta luego!", 2, COLOR_WHITE);
    display_text_center(130, "Para encender de nuevo:", 1, COLOR_GRAY);
    display_text_center(146, "pulsa PWR", 2, COLOR_YELLOW);
    display_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));
    display_sleep();
    board_power_off();   // no vuelve
}

// Opciones del menú principal. ¡Añade aquí las tuyas!
static const menu_item_t MENU[] = {
    {"Mi app",       app_template},
    {"Ayuda (QR)",   app_help_qr},
    {"Botones",      app_buttons},
    {"Pantalla",     app_screen},
    {"Info placa",   app_info},
    {"Batería",      app_battery},
    {"Movimiento",   app_imu},
    {"Sonido",       app_sound},
    {"Tarjeta SD",   app_sdcard},
    {"Escáner I2C",  app_i2c_scan},
    {"Apagar",       app_power_off},
};

static void splash(void)
{
    display_clear(COLOR_BLACK);
    display_circle(120, 80, 40, COLOR_ACCENT, true);
    display_text_center(66, "ESP32", 3, COLOR_WHITE);
    display_text_center(140, "Starter kit", 2, COLOR_YELLOW);
    display_text_center(166, "Waveshare ESP32-S3-LCD-1.54", 1, COLOR_GRAY);
    display_text_center(200, "¡Bienvenido! Pulsa un botón", 1, COLOR_WHITE);
    display_flush();

    // Espera 3 s o hasta que se pulse algo
    button_event_t e;
    const TickType_t end = xTaskGetTickCount() + pdMS_TO_TICKS(3000);
    while (xTaskGetTickCount() < end) {
        if (buttons_get_event(&e, pdMS_TO_TICKS(50)) && e.type == BTN_EVENT_CLICK) break;
    }
}

void app_main(void)
{
    board_power_hold();   // ¡lo primero! si no, con batería se apaga al soltar PWR
    ESP_LOGI(TAG, "Arrancando esp32-s3-lcd154-starter");

    display_init();
    buttons_init();

    splash();
    menu_run("Starter kit", MENU, sizeof(MENU) / sizeof(MENU[0]));
}
