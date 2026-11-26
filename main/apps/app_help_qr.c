/*
 * app_help_qr.c — Enlaces de ayuda como códigos QR: escanéalos con el móvil.
 * Usa el componente espressif/qrcode para generar el QR.
 */
#include "apps.h"
#include "buttons.h"
#include "display.h"

#include <string.h>
#include "qrcode.h"

typedef struct {
    const char *title;
    const char *description;
    const char *url;
} help_link_t;

static const help_link_t LINKS[] = {
    {"Producto",  "Página de la placa en Waveshare",
     "https://www.waveshare.com/esp32-s3-lcd-1.54.htm"},
    {"Wiki",      "Documentación y tutoriales",
     "https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54"},
    {"Esquemas",  "Esquema eléctrico y datasheets",
     "https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54/Resources-And-Documents"},
    {"Vídeo",     "Vídeo de presentación",
     "https://www.youtube.com/watch?v=McWEw64yzAA"},
    {"Ejemplos",  "Código de ejemplo oficial (GitHub)",
     "https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54"},
    {"ESP-IDF",   "Instalar ESP-IDF y primeros pasos",
     "https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/index.html"},
    {"API",       "Referencia de la API de ESP-IDF",
     "https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/index.html"},
};
#define LINK_COUNT ((int)(sizeof(LINKS) / sizeof(LINKS[0])))

#define QR_AREA 150   // tamaño máximo del QR en píxeles
#define QR_TOP  30

// esp_qrcode_generate() llama a esta función con el QR ya calculado.
static void draw_qr(esp_qrcode_handle_t qr)
{
    const int size = esp_qrcode_get_size(qr);
    const int quiet = 2;                               // margen blanco (en módulos)
    int scale = QR_AREA / (size + 2 * quiet);
    if (scale < 1) scale = 1;
    const int px = (size + 2 * quiet) * scale;
    const int x0 = (DISPLAY_W - px) / 2, y0 = QR_TOP;

    display_rect(x0, y0, px, px, COLOR_WHITE);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (esp_qrcode_get_module(qr, x, y)) {
                display_rect(x0 + (x + quiet) * scale, y0 + (y + quiet) * scale, scale, scale, COLOR_BLACK);
            }
        }
    }
}

/** Escribe la URL partida en líneas de como mucho 39 caracteres. */
static void draw_wrapped(int y, const char *text, uint16_t color)
{
    const int max_chars = DISPLAY_W / FONT_W - 1;
    char line[48];
    const size_t len = strlen(text);
    for (size_t pos = 0; pos < len; pos += max_chars, y += FONT_H + 2) {
        snprintf(line, sizeof(line), "%.*s", max_chars, text + pos);
        display_text_center(y, line, 1, color);
    }
}

void app_help_qr(void)
{
    int idx = 0;
    for (;;) {
        const help_link_t *link = &LINKS[idx];
        char title[32];
        snprintf(title, sizeof(title), "%s %d/%d", link->title, idx + 1, LINK_COUNT);

        display_clear(COLOR_BLACK);
        display_header(title);

        esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
        cfg.display_func = draw_qr;
        cfg.max_qrcode_version = 10;
        cfg.qrcode_ecc_level = ESP_QRCODE_ECC_MED;
        if (esp_qrcode_generate(&cfg, link->url) != ESP_OK) {
            display_text_center(90, "Error generando QR", 2, COLOR_RED);
        }

        display_text_center(QR_TOP + QR_AREA + 4, link->description, 1, COLOR_YELLOW);
        draw_wrapped(QR_TOP + QR_AREA + 18, link->url, COLOR_GRAY);
        display_footer("+ siguiente  PWR anterior  BOOT volver");
        display_flush();

        button_event_t e;
        buttons_get_event(&e, portMAX_DELAY);
        if (e.type != BTN_EVENT_CLICK) continue;
        if (e.button == BTN_PLUS) idx = (idx + 1) % LINK_COUNT;
        if (e.button == BTN_PWR) idx = (idx + LINK_COUNT - 1) % LINK_COUNT;
        if (e.button == BTN_BOOT) return;
    }
}
