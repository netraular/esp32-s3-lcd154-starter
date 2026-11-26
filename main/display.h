/*
 * Drawing API for a 240x240 display backed by a framebuffer.
 *
 * Draw into the framebuffer, then call display_flush() to send the full
 * frame to the screen.
 *
 * The origin is at the top left. X increases rightward and Y downward.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DISPLAY_W 240
#define DISPLAY_H 240

// RGB565 colors: 5 red bits, 6 green bits, 5 blue bits.
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

// Unscaled character size (5x7 glyph plus 1 px spacing).
#define FONT_W 6
#define FONT_H 8

/** Initialize SPI, the ST7789 controller, and the backlight. */
void display_init(void);

/** Send the framebuffer to the screen and wait for the transfer. */
void display_flush(void);

/** Turn off the backlight and put the display controller to sleep. */
void display_sleep(void);

/** Backlight brightness, from 0 to 100 percent. */
void display_set_brightness(int percent);
int  display_get_brightness(void);

// ---- Framebuffer drawing ----
void display_clear(uint16_t color);
void display_pixel(int x, int y, uint16_t color);
void display_rect(int x, int y, int w, int h, uint16_t color);         // Filled.
void display_frame(int x, int y, int w, int h, uint16_t color);        // Outline only.
void display_line(int x0, int y0, int x1, int y1, uint16_t color);
void display_circle(int cx, int cy, int r, uint16_t color, bool filled);

/**
 * Draw text and return the final x position. Supports ASCII, selected
 * Latin-1 glyphs in UTF-8, and '\n' line breaks. At scale 1, each character
 * takes 6x8 pixels (40 columns); at scale 2, it takes 12x16 (20 columns).
 */
int display_text(int x, int y, const char *text, int scale, uint16_t color);

/** Draw text centered horizontally on the screen. */
void display_text_center(int y, const char *text, int scale, uint16_t color);

/** Draw printf-formatted text into the framebuffer. */
int display_printf(int x, int y, int scale, uint16_t color, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));

/** Return the width of a single line of text in pixels. */
int display_text_width(const char *text, int scale);

/** Draw the common title bar or footer. */
void display_header(const char *title);
void display_footer(const char *hint);
