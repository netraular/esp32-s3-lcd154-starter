/*
 * display.h — Pantalla 240x240 con un "lienzo" en memoria.
 *
 * Forma de uso:
 *   1. Dibuja en el lienzo (display_clear, display_rect, display_text...).
 *      Esto solo cambia la memoria, la pantalla no cambia todavía.
 *   2. Llama a display_flush() para enviar el lienzo completo a la pantalla.
 *
 * Coordenadas: (0,0) arriba a la izquierda, x hacia la derecha, y hacia abajo.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DISPLAY_W 240
#define DISPLAY_H 240

// Colores RGB565 (5 bits rojo, 6 verde, 5 azul)
#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
#define COLOR_BLACK   RGB565(0, 0, 0)
#define COLOR_WHITE   RGB565(255, 255, 255)
#define COLOR_RED     RGB565(255, 0, 0)
#define COLOR_GREEN   RGB565(0, 255, 0)
#define COLOR_BLUE    RGB565(0, 0, 255)
#define COLOR_YELLOW  RGB565(255, 220, 0)
#define COLOR_CYAN    RGB565(0, 220, 255)
#define COLOR_MAGENTA RGB565(255, 0, 255)
#define COLOR_ORANGE  RGB565(255, 140, 0)
#define COLOR_GRAY    RGB565(120, 120, 120)
#define COLOR_DARK    RGB565(30, 30, 40)
#define COLOR_ACCENT  RGB565(0, 150, 255)

// Tamaño de un carácter con escala 1 (fuente 5x7 + 1 px de separación)
#define FONT_W 6
#define FONT_H 8

/** Inicializa SPI, el controlador ST7789 y la retroiluminación. */
void display_init(void);

/** Envía el lienzo a la pantalla (espera a que termine). */
void display_flush(void);

/** Brillo de la retroiluminación, 0..100 %. */
void display_set_brightness(int percent);
int  display_get_brightness(void);

// ---- Dibujo sobre el lienzo ----
void display_clear(uint16_t color);
void display_pixel(int x, int y, uint16_t color);
void display_rect(int x, int y, int w, int h, uint16_t color);         // relleno
void display_frame(int x, int y, int w, int h, uint16_t color);        // solo borde
void display_line(int x0, int y0, int x1, int y1, uint16_t color);
void display_circle(int cx, int cy, int r, uint16_t color, bool filled);

/**
 * Escribe texto (UTF-8; admite á é í ó ú ñ ¿ ¡ °). '\n' salta de línea.
 * scale = 1 -> 6x8 px por carácter (40 columnas), 2 -> 12x16 (20 columnas)...
 * Devuelve la x final.
 */
int display_text(int x, int y, const char *text, int scale, uint16_t color);

/** Como display_text pero centrado horizontalmente en la pantalla. */
void display_text_center(int y, const char *text, int scale, uint16_t color);

/** printf sobre el lienzo. */
int display_printf(int x, int y, int scale, uint16_t color, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));

/** Ancho en píxeles que ocupará un texto (una sola línea). */
int display_text_width(const char *text, int scale);

/** Barra de título y barra de ayuda inferior con estilo común. */
void display_header(const char *title);
void display_footer(const char *hint);
