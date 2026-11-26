/*
 * app_battery.c — Muestra el estado de la batería que mide battery.c.
 *
 * La batería llega a GPIO1 a través de un divisor 200k/100k (el ADC ve 1/3 de
 * la tensión). CHG (GPIO3) vale 0 mientras carga. Ver battery.h para saber
 * cómo se deduce "carga completa" y por qué no se muestra % al cargar.
 */
#include "apps.h"
#include "battery.h"
#include "buttons.h"
#include "display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void draw_battery_icon(int fill_percent, uint16_t color)
{
    display_frame(20, 40, 120, 50, COLOR_WHITE);
    display_rect(140, 55, 8, 20, COLOR_WHITE);
    display_rect(24, 44, 112 * fill_percent / 100, 42, color);
}

void app_battery(void)
{
    static int history[BATTERY_HISTORY_LEN];
    int anim = 0;

    for (;;) {
        battery_status_t b;
        battery_get(&b);

        display_clear(COLOR_BLACK);
        display_header("Batería");

        if (!b.valid) {
            display_text_center(100, "Midiendo...", 2, COLOR_GRAY);
        } else {
            const char *state_text;
            uint16_t state_color;
            switch (b.state) {
            case BATTERY_CHARGING:
                // Animación de carga: el icono se va llenando
                anim = (anim + 25) % 125;
                draw_battery_icon(anim > 100 ? 100 : anim, COLOR_YELLOW);
                display_text(160, 50, "Carga", 2, COLOR_YELLOW);
                state_text = "Cargando: el % se ve al desconectar";
                state_color = COLOR_YELLOW;
                break;
            case BATTERY_FULL:
                draw_battery_icon(100, COLOR_GREEN);
                display_text(160, 50, "100%", 2, COLOR_GREEN);
                state_text = "Carga completa";
                state_color = COLOR_GREEN;
                break;
            default: {
                const uint16_t fill = b.percent > 50 ? COLOR_GREEN : b.percent > 20 ? COLOR_YELLOW : COLOR_RED;
                draw_battery_icon(b.percent, fill);
                display_printf(160, 50, 2, COLOR_WHITE, "%d%%", b.percent);
                state_text = "Con batería";
                state_color = COLOR_WHITE;
                break;
            }
            }
            display_printf(160, 72, 1, COLOR_WHITE, "%d.%02d V", b.mv / 1000, (b.mv % 1000) / 10);
            display_text(20, 100, state_text, 1, state_color);
            display_printf(20, 114, 1, COLOR_GRAY, "GPIO1 %d mV x3%s  USB-PC: %s", b.adc_mv,
                           b.calibrated ? "" : " (sin cal.)", b.usb_host ? "sí" : "no");

            // Gráfica de tensión (3.0 V .. 4.4 V)
            const int n = battery_history(history, BATTERY_HISTORY_LEN);
            const int gx = 20, gy = 132, gw = BATTERY_HISTORY_LEN, gh = 70;
            display_frame(gx - 1, gy - 1, gw + 2, gh + 2, COLOR_DARK);
            for (int i = 0; i < n; i++) {
                int v = (history[i] - 3000) * gh / 1400;
                if (v < 0) v = 0;
                if (v > gh - 1) v = gh - 1;
                display_pixel(gx + i, gy + gh - 1 - v, COLOR_CYAN);
            }
            display_text(gx, gy + gh + 4, "Últimos 20 min, 3.0-4.4 V", 1, COLOR_GRAY);
        }
        display_footer("BOOT volver");
        display_flush();

        button_event_t e;
        if (buttons_get_event(&e, pdMS_TO_TICKS(500)) && e.type == BTN_EVENT_CLICK && e.button == BTN_BOOT) return;
    }
}
