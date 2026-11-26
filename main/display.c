/*
 * display.c — Driver mínimo: ST7789 por SPI + lienzo RGB565 de 240x240.
 *
 * Se usa el driver esp_lcd que viene con ESP-IDF. Todo el dibujo se hace en
 * un búfer en RAM (115 KB) y display_flush() lo envía entero por DMA.
 */
#include "display.h"
#include "font5x7.h"
#include "board.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "display";

#define LCD_SPI_HOST   SPI2_HOST
#define LCD_SPI_HZ     (40 * 1000 * 1000)
#define LCD_CHUNK_ROWS 40

static esp_lcd_panel_handle_t s_panel;
static uint16_t *s_fb;               // lienzo (píxeles ya en el orden de bytes del ST7789)
static SemaphoreHandle_t s_flush_done;
static int s_brightness = 100;

// El ST7789 espera cada píxel con el byte alto primero; el ESP32 es little-endian.
static inline uint16_t swap16(uint16_t c) { return (uint16_t)((c >> 8) | (c << 8)); }

static bool on_flush_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *e, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_flush_done, &woken);
    return woken == pdTRUE;
}

static void backlight_init(void)
{
    const ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
    const ledc_channel_config_t ch = {
        .gpio_num = PIN_LCD_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));
}

void display_set_brightness(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    s_brightness = percent;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (1023 * percent) / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

int display_get_brightness(void) { return s_brightness; }

void display_sleep(void)
{
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);   // retroiluminación a 0
    esp_lcd_panel_disp_on_off(s_panel, false);
    esp_lcd_panel_disp_sleep(s_panel, true);
}

void display_init(void)
{
    s_flush_done = xSemaphoreCreateBinary();

    const spi_bus_config_t bus = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_W * LCD_CHUNK_ROWS * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io;
    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = LCD_SPI_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 3,
        .trans_queue_depth = 10,
        .on_color_trans_done = on_flush_done,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_cfg, &io));

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &panel_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true)); // este panel IPS va invertido
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    // El lienzo debe estar en RAM interna con DMA para que el SPI lo lea directamente.
    s_fb = heap_caps_malloc(DISPLAY_W * DISPLAY_H * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (s_fb == NULL) {
        ESP_LOGE(TAG, "Sin memoria para el lienzo");
        abort();
    }
    display_clear(COLOR_BLACK);
    display_flush();

    backlight_init();
    display_set_brightness(s_brightness);
    ESP_LOGI(TAG, "Pantalla lista (%dx%d)", DISPLAY_W, DISPLAY_H);
}

void display_flush(void)
{
    esp_lcd_panel_draw_bitmap(s_panel, 0, 0, DISPLAY_W, DISPLAY_H, s_fb);
    // Esperar a que el DMA termine antes de que alguien vuelva a dibujar en el lienzo.
    xSemaphoreTake(s_flush_done, pdMS_TO_TICKS(1000));
}

// ---------------------------------------------------------------------------
// Primitivas de dibujo
// ---------------------------------------------------------------------------

void display_clear(uint16_t color)
{
    const uint16_t c = swap16(color);
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++) s_fb[i] = c;
}

void display_pixel(int x, int y, uint16_t color)
{
    if ((unsigned)x < DISPLAY_W && (unsigned)y < DISPLAY_H) s_fb[y * DISPLAY_W + x] = swap16(color);
}

void display_rect(int x, int y, int w, int h, uint16_t color)
{
    // Recortar a los bordes de la pantalla
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > DISPLAY_W) w = DISPLAY_W - x;
    if (y + h > DISPLAY_H) h = DISPLAY_H - y;
    if (w <= 0 || h <= 0) return;
    const uint16_t c = swap16(color);
    for (int j = y; j < y + h; j++) {
        uint16_t *row = &s_fb[j * DISPLAY_W + x];
        for (int i = 0; i < w; i++) row[i] = c;
    }
}

void display_frame(int x, int y, int w, int h, uint16_t color)
{
    display_rect(x, y, w, 1, color);
    display_rect(x, y + h - 1, w, 1, color);
    display_rect(x, y, 1, h, color);
    display_rect(x + w - 1, y, 1, h, color);
}

void display_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    // Algoritmo de Bresenham
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        display_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void display_circle(int cx, int cy, int r, uint16_t color, bool filled)
{
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            int d = x * x + y * y;
            if (filled ? d <= r * r : (d <= r * r && d > (r - 1) * (r - 1))) display_pixel(cx + x, cy + y, color);
        }
    }
}

// ---------------------------------------------------------------------------
// Texto
// ---------------------------------------------------------------------------

/** Lee un carácter UTF-8 de *p, avanza el puntero y devuelve su dibujo 5x7. */
static const uint8_t *next_glyph(const char **p)
{
    const unsigned char c = (unsigned char)**p;
    (*p)++;
    if (c >= 32 && c <= 126) return FONT5X7[c - 32];

    const unsigned char c2 = (unsigned char)**p;
    if ((c == 0xC3 || c == 0xC2) && c2 != 0) {
        (*p)++;
        if (c == 0xC2) {
            switch (c2) {
            case 0xBF: return FONT5X7_EXTRA[GLYPH_INV_QUESTION];
            case 0xA1: return FONT5X7_EXTRA[GLYPH_INV_EXCLAM];
            case 0xB0: return FONT5X7_EXTRA[GLYPH_DEGREE];
            }
        } else {
            switch (c2) {
            case 0xA1: return FONT5X7_EXTRA[GLYPH_A_ACUTE];
            case 0xA9: return FONT5X7_EXTRA[GLYPH_E_ACUTE];
            case 0xAD: return FONT5X7_EXTRA[GLYPH_I_ACUTE];
            case 0xB3: return FONT5X7_EXTRA[GLYPH_O_ACUTE];
            case 0xBA: case 0xBC: return FONT5X7_EXTRA[GLYPH_U_ACUTE]; // ú, ü
            case 0xB1: return FONT5X7_EXTRA[GLYPH_N_TILDE];
            // Mayúsculas acentuadas: se dibujan sin tilde
            case 0x81: return FONT5X7['A' - 32];
            case 0x89: return FONT5X7['E' - 32];
            case 0x8D: return FONT5X7['I' - 32];
            case 0x93: return FONT5X7['O' - 32];
            case 0x9A: case 0x9C: return FONT5X7['U' - 32];
            case 0x91: return FONT5X7['N' - 32];
            }
        }
        return FONT5X7['?' - 32];
    }
    // Otros caracteres multibyte: saltar los bytes de continuación
    while ((((unsigned char)**p) & 0xC0) == 0x80) (*p)++;
    return FONT5X7['?' - 32];
}

int display_text(int x, int y, const char *text, int scale, uint16_t color)
{
    const int x_start = x;
    const char *p = text;
    while (*p) {
        if (*p == '\n') {
            p++;
            x = x_start;
            y += FONT_H * scale;
            continue;
        }
        const uint8_t *g = next_glyph(&p);
        for (int col = 0; col < 5; col++) {
            for (int row = 0; row < 7; row++) {
                if (g[col] & (1 << row)) display_rect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
        x += FONT_W * scale;
    }
    return x;
}

int display_text_width(const char *text, int scale)
{
    int n = 0;
    for (const char *p = text; *p;) {
        if (*p == '\n') break;
        next_glyph(&p);
        n++;
    }
    return n > 0 ? n * FONT_W * scale - scale : 0; // sin el espacio tras el último carácter
}

void display_text_center(int y, const char *text, int scale, uint16_t color)
{
    display_text((DISPLAY_W - display_text_width(text, scale)) / 2, y, text, scale, color);
}

int display_printf(int x, int y, int scale, uint16_t color, const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return display_text(x, y, buf, scale, color);
}

void display_header(const char *title)
{
    display_rect(0, 0, DISPLAY_W, 26, COLOR_ACCENT);
    display_text_center(5, title, 2, COLOR_WHITE);
}

void display_footer(const char *hint)
{
    display_rect(0, DISPLAY_H - 14, DISPLAY_W, 14, COLOR_DARK);
    display_text_center(DISPLAY_H - 11, hint, 1, COLOR_GRAY);
}
