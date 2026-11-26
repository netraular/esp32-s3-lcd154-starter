/*
 * A starting point for your own app.
 *
 * Shows a counter and a bouncing ball.
 *   "+"  -> increment the counter
 *   PWR  -> change the ball's color
 *   BOOT -> return to the menu
 *
 * Typical app loop:
 *   for (;;) {
 *       1. Read buttons without blocking.
 *       2. Update the app state.
 *       3. Draw to the framebuffer and call display_flush().
 *   }
 */
#include "apps.h"
#include "buttons.h"
#include "display.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mi_app";

void app_template(void)
{
    static const uint16_t colors[] = {COLOR_YELLOW, COLOR_GREEN, COLOR_MAGENTA, COLOR_CYAN, COLOR_ORANGE};
    int counter = 0;
    int color = 0;
    int x = 120, y = 150, dx = 3, dy = 2;
    const int r = 10;

    for (;;) {
        // 1. Consume pending button events without waiting.
        button_event_t e;
        while (buttons_get_event(&e, 0)) {
            if (e.type != BTN_EVENT_CLICK) continue;
            if (e.button == BTN_BOOT) return;   // Return to the menu.
            if (e.button == BTN_PLUS) {
                counter++;
                ESP_LOGI(TAG, "counter = %d", counter);  // Shows up in idf.py monitor.
            }
            if (e.button == BTN_PWR) color = (color + 1) % 5;
        }

        // 2. Move the ball and bounce at the edges.
        x += dx;
        y += dy;
        if (x < r || x > DISPLAY_W - 1 - r) dx = -dx;
        if (y < 90 + r || y > DISPLAY_H - 15 - r) dy = -dy;

        // 3. Draw the frame.
        display_clear(COLOR_BLACK);
        display_header("My app");
        display_text_center(34, "Edit main/apps/app_template.c", 1, COLOR_GRAY);
        display_printf(20, 54, 3, COLOR_WHITE, "Count: %d", counter);
        display_rect(0, 88, DISPLAY_W, 1, COLOR_DARK);
        display_circle(x, y, r, colors[color], true);
        display_footer("+ add   PWR color   BOOT back");
        display_flush();

        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
