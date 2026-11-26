/*
 * app_i2c_scan.c — Busca dispositivos en el bus I2C (SDA=GPIO42, SCL=GPIO41).
 * Útil también para comprobar sensores que conectes tú.
 */
#include "apps.h"
#include "board.h"
#include "buttons.h"
#include "display.h"

static const char *known_name(uint8_t addr)
{
    switch (addr) {
    case 0x15: return "CST816 (táctil)";
    case 0x18: return "ES8311 (altavoz)";
    case 0x40: return "ES7210 (micrófono)";
    case 0x51: return "PCF85063 (reloj)";
    case 0x6A:
    case 0x6B: return "QMI8658 (IMU)";
    }
    return "desconocido";
}

void app_i2c_scan(void)
{
    i2c_master_bus_handle_t bus = board_i2c_bus();
    for (;;) {
        display_clear(COLOR_BLACK);
        display_header("Escáner I2C");
        display_text_center(40, "Buscando...", 2, COLOR_GRAY);
        display_flush();

        uint8_t found[32];
        int n = 0;
        for (uint8_t addr = 0x08; addr < 0x78 && n < 32; addr++) {
            if (i2c_master_probe(bus, addr, 20) == ESP_OK) found[n++] = addr;
        }

        display_clear(COLOR_BLACK);
        display_header("Escáner I2C");
        display_printf(8, 34, 1, COLOR_CYAN, "%d dispositivo(s) en SDA42/SCL41:", n);
        for (int i = 0; i < n && i < 12; i++) {
            display_printf(8, 52 + i * 14, 1, COLOR_YELLOW, "0x%02X", found[i]);
            display_text(48, 52 + i * 14, known_name(found[i]), 1, COLOR_WHITE);
        }
        if (n == 0) display_text_center(100, "Nada encontrado", 2, COLOR_RED);
        display_footer("PWR repetir   BOOT volver");
        display_flush();

        button_event_t e;
        for (;;) {
            buttons_get_event(&e, portMAX_DELAY);
            if (e.type != BTN_EVENT_CLICK) continue;
            if (e.button == BTN_BOOT) return;
            if (e.button == BTN_PWR) break;
        }
    }
}
