/*
 * app_template.c — PLANTILLA: punto de partida para tus programas.
 *
 * Muestra un contador y una pelota que rebota.
 *   "+"  -> suma 1 al contador
 *   PWR  -> cambia el color de la pelota
 *   BOOT -> volver al menú
 *
 * Patrón típico de una app:
 *   for (;;) {
 *       1. leer botones (sin bloquear mucho tiempo)
 *       2. actualizar el estado (variables)
 *       3. dibujar en el lienzo y display_flush()
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
        // 1. Botones: atender todos los eventos pendientes sin esperar
        button_event_t e;
        while (buttons_get_event(&e, 0)) {
            if (e.type != BTN_EVENT_CLICK) continue;
            if (e.button == BTN_BOOT) return;   // volver al menú
            if (e.button == BTN_PLUS) {
                counter++;
                ESP_LOGI(TAG, "contador = %d", counter);  // aparece en "idf.py monitor"
            }
            if (e.button == BTN_PWR) color = (color + 1) % 5;
        }

        // 2. Estado: mover la pelota y rebotar en los bordes
        x += dx;
        y += dy;
        if (x < r || x > DISPLAY_W - 1 - r) dx = -dx;
        if (y < 90 + r || y > DISPLAY_H - 15 - r) dy = -dy;

        // 3. Dibujar
        display_clear(COLOR_BLACK);
        display_header("Mi app");
        display_text_center(34, "Edita main/apps/app_template.c", 1, COLOR_GRAY);
        display_printf(20, 54, 3, COLOR_WHITE, "Cuenta: %d", counter);
        display_rect(0, 88, DISPLAY_W, 1, COLOR_DARK);
        display_circle(x, y, r, colors[color], true);
        display_footer("+ sumar   PWR color   BOOT volver");
        display_flush();

        vTaskDelay(pdMS_TO_TICKS(16));  // ~60 fotogramas por segundo como máximo
    }
}
