/*
 * Shows each button's current state, short and long press counts, and the
 * latest event. Hold BOOT for two seconds to exit.
 */
#include "apps.h"
#include "buttons.h"
#include "display.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

static const char *event_name(button_event_type_t t)
{
    switch (t) {
    case BTN_EVENT_DOWN:  return "DOWN (pressed)";
    case BTN_EVENT_UP:    return "UP (released)";
    case BTN_EVENT_CLICK: return "CLICK (short)";
    case BTN_EVENT_LONG:  return "LONG (held)";
    }
    return "?";
}

void app_buttons(void)
{
    int clicks[BTN_COUNT] = {0}, longs[BTN_COUNT] = {0};
    char last[48] = "(none)";
    int64_t boot_down_since = -1;

    for (;;) {
        button_event_t e;
        while (buttons_get_event(&e, 0)) {
            if (e.type == BTN_EVENT_CLICK) clicks[e.button]++;
            if (e.type == BTN_EVENT_LONG) longs[e.button]++;
            snprintf(last, sizeof(last), "%s: %s", buttons_name(e.button), event_name(e.type));
        }

        // Exit after holding BOOT for two seconds.
        const int64_t now = esp_timer_get_time();
        if (buttons_is_down(BTN_BOOT)) {
            if (boot_down_since < 0) boot_down_since = now;
            if (now - boot_down_since > 2000000) {
                while (buttons_is_down(BTN_BOOT)) vTaskDelay(pdMS_TO_TICKS(10));
                return;
            }
        } else {
            boot_down_since = -1;
        }

        display_clear(COLOR_BLACK);
        display_header("Buttons");
        // One box per button in physical order: BOOT, PWR, +.
        static const button_id_t order[BTN_COUNT] = {BTN_BOOT, BTN_PWR, BTN_PLUS};
        static const char *gpio_txt[BTN_COUNT] = {"GPIO0", "GPIO4", "GPIO5"};
        for (int i = 0; i < BTN_COUNT; i++) {
            const button_id_t b = order[i];
            const int x = 8 + i * 78, y = 36, w = 70, h = 70;
            const bool down = buttons_is_down(b);
            display_rect(x, y, w, h, down ? COLOR_GREEN : COLOR_DARK);
            display_frame(x, y, w, h, COLOR_GRAY);
            const char *name = buttons_name(b);
            display_text(x + (w - display_text_width(name, 2)) / 2, y + 12, name, 2, down ? COLOR_BLACK : COLOR_WHITE);
            display_text(x + (w - display_text_width(gpio_txt[b], 1)) / 2, y + 36, gpio_txt[b], 1,
                         down ? COLOR_BLACK : COLOR_GRAY);
            display_text(x + (w - display_text_width(down ? "ON" : "off", 1)) / 2, y + 52, down ? "ON" : "off", 1,
                         down ? COLOR_BLACK : COLOR_GRAY);
            display_printf(x + 4, y + h + 8, 1, COLOR_WHITE, "Short %d", clicks[b]);
            display_printf(x + 4, y + h + 20, 1, COLOR_WHITE, "Long %d", longs[b]);
        }
        display_text(8, 150, "Last event:", 1, COLOR_CYAN);
        display_text(8, 164, last, 1, COLOR_YELLOW);
        display_text(8, 186, "Pressed = low (pull-up).", 1, COLOR_GRAY);
        display_printf(8, 200, 1, COLOR_GRAY, "Long press = %d ms.", BUTTON_LONG_MS);
        if (boot_down_since >= 0) {
            const int pct = (int)((now - boot_down_since) / 20000);
            display_rect(0, DISPLAY_H - 18, DISPLAY_W * (pct > 100 ? 100 : pct) / 100, 4, COLOR_RED);
        }
        display_footer("Hold BOOT 2 s to go back");
        display_flush();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
