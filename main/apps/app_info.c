/*
 * Board details: chip, memory, MAC address, and ESP-IDF version.
 */
#include "apps.h"
#include "buttons.h"
#include "display.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_mac.h"
#include "esp_psram.h"
#include "esp_timer.h"

void app_info(void)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    for (;;) {
        display_clear(COLOR_BLACK);
        display_header("Board info");
        const int x = 8, s = 1, c = COLOR_WHITE, k = COLOR_CYAN;
        int y = 34;
        const int dy = 14;
        display_text(x, y, "Chip:", s, k);
        display_printf(x + 66, y, s, c, "ESP32-S3 rev %d.%d", chip.revision / 100, chip.revision % 100); y += dy;
        display_text(x, y, "Cores:", s, k);
        display_printf(x + 66, y, s, c, "%d (240 MHz)", chip.cores); y += dy;
        display_text(x, y, "Radio:", s, k);
        display_printf(x + 66, y, s, c, "%s%s", (chip.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi " : "",
                       (chip.features & CHIP_FEATURE_BLE) ? "BLE" : ""); y += dy;
        display_text(x, y, "Flash:", s, k);
        display_printf(x + 66, y, s, c, "%lu MB", (unsigned long)(flash_size / (1024 * 1024))); y += dy;
        display_text(x, y, "PSRAM:", s, k);
        display_printf(x + 66, y, s, c, "%u MB", (unsigned)(esp_psram_get_size() / (1024 * 1024))); y += dy;
        display_text(x, y, "Free RAM:", s, k);
        display_printf(x + 66, y, s, c, "%u KB int + %u KB ext",
                       (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                       (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024)); y += dy;
        display_text(x, y, "MAC:", s, k);
        display_printf(x + 66, y, s, c, "%02X:%02X:%02X:%02X:%02X:%02X",
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); y += dy;
        display_text(x, y, "ESP-IDF:", s, k);
        display_printf(x + 66, y, s, c, "%s", esp_get_idf_version()); y += dy;
        const unsigned up = (unsigned)(esp_timer_get_time() / 1000000);
        display_text(x, y, "Uptime:", s, k);
        display_printf(x + 66, y, s, c, "%02u:%02u:%02u", up / 3600, (up / 60) % 60, up % 60); y += dy + 6;

        display_text(x, y, "ST7789 240x240 SPI display\nQMI8658 IMU, ES8311/ES7210 audio\nmicroSD, 3.7 V Li-ion battery",
                     s, COLOR_GRAY);
        display_footer("BOOT back");
        display_flush();

        button_event_t e;
        if (buttons_get_event(&e, pdMS_TO_TICKS(1000)) && e.type == BTN_EVENT_CLICK && e.button == BTN_BOOT) return;
    }
}
