/*
 * Starter firmware for the Waveshare ESP32-S3-LCD-1.54.
 *
 * ESP-IDF calls app_main() after FreeRTOS starts.
 */
#include "apps.h"
#include "battery.h"
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
    display_header("Power off");
    display_text_center(80, "Goodbye!", 2, COLOR_WHITE);
    display_text_center(130, "To turn the board back on:", 1, COLOR_GRAY);
    display_text_center(146, "Hold PWR", 2, COLOR_YELLOW);
    display_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));
    display_sleep();
    board_power_off();   // Does not return.
}

// Add your own apps to the main menu here.
static const menu_item_t MENU[] = {
    {"My app",      app_template},
    {"Help (QR)",   app_help_qr},
    {"Buttons",     app_buttons},
    {"Display",     app_screen},
    {"Board info",  app_info},
    {"Battery",     app_battery},
    {"Motion",      app_imu},
    {"Sound",       app_sound},
    {"microSD",     app_sdcard},
    {"I2C scanner", app_i2c_scan},
    {"Power off",   app_power_off},
};

static void splash(void)
{
    display_clear(COLOR_BLACK);
    display_circle(120, 80, 40, COLOR_ACCENT, true);
    display_text_center(66, "ESP32", 3, COLOR_WHITE);
    display_text_center(140, "Starter kit", 2, COLOR_YELLOW);
    display_text_center(166, "Waveshare ESP32-S3-LCD-1.54", 1, COLOR_GRAY);
    display_text_center(200, "Press any button", 1, COLOR_WHITE);
    display_flush();

    // Wait up to three seconds for a button press.
    button_event_t e;
    const TickType_t end = xTaskGetTickCount() + pdMS_TO_TICKS(3000);
    while (xTaskGetTickCount() < end) {
        if (buttons_get_event(&e, pdMS_TO_TICKS(50)) && e.type == BTN_EVENT_CLICK) break;
    }
}

void app_main(void)
{
    board_power_hold();   // Latch battery power before PWR is released.
    ESP_LOGI(TAG, "Starting esp32-s3-lcd154-starter");

    display_init();
    buttons_init();
    battery_init();       // Sample the battery in the background.

    splash();
    menu_run("Starter kit", MENU, sizeof(MENU) / sizeof(MENU[0]));
}
