/*
 * app_screen.c — Prueba de pantalla: colores puros (para ver píxeles muertos),
 * degradados, rejilla y ajuste de brillo.
 */
#include "apps.h"
#include "buttons.h"
#include "display.h"

typedef void (*pattern_fn)(void);

static void solid(uint16_t c, const char *name)
{
    display_clear(c);
    display_text(4, 4, name, 1, c == COLOR_WHITE ? COLOR_BLACK : COLOR_WHITE);
}
static void p_red(void)   { solid(COLOR_RED, "Rojo"); }
static void p_green(void) { solid(COLOR_GREEN, "Verde"); }
static void p_blue(void)  { solid(COLOR_BLUE, "Azul"); }
static void p_white(void) { solid(COLOR_WHITE, "Blanco"); }
static void p_black(void) { solid(COLOR_BLACK, "Negro"); }

static void p_gradient(void)
{
    // 4 franjas: rojo, verde, azul y gris, de oscuro a claro
    for (int x = 0; x < DISPLAY_W; x++) {
        const int v = x * 255 / (DISPLAY_W - 1);
        display_rect(x, 0, 1, 60, RGB565(v, 0, 0));
        display_rect(x, 60, 1, 60, RGB565(0, v, 0));
        display_rect(x, 120, 1, 60, RGB565(0, 0, v));
        display_rect(x, 180, 1, 60, RGB565(v, v, v));
    }
    display_text(4, 4, "Degradados", 1, COLOR_WHITE);
}

static void p_grid(void)
{
    display_clear(COLOR_BLACK);
    for (int i = 0; i < DISPLAY_W; i += 20) {
        display_rect(i, 0, 1, DISPLAY_H, COLOR_GRAY);
        display_rect(0, i, DISPLAY_W, 1, COLOR_GRAY);
    }
    display_frame(0, 0, DISPLAY_W, DISPLAY_H, COLOR_RED);  // el borde debe verse entero
    display_line(0, 0, DISPLAY_W - 1, DISPLAY_H - 1, COLOR_YELLOW);
    display_line(DISPLAY_W - 1, 0, 0, DISPLAY_H - 1, COLOR_YELLOW);
    display_circle(120, 120, 60, COLOR_CYAN, false);
    display_text(4, 4, "Rejilla 20 px", 1, COLOR_WHITE);
}

static void p_text(void)
{
    display_clear(COLOR_BLACK);
    display_text(4, 4, "Escala 1: ABCabc 0123 ¡¿ñáéíóú°?!", 1, COLOR_WHITE);
    display_text(4, 20, "Escala 2: Hola", 2, COLOR_YELLOW);
    display_text(4, 42, "Esc. 3", 3, COLOR_GREEN);
    display_text(4, 70, "Esc 4", 4, COLOR_CYAN);
    display_text(4, 110, " !\"#$%&'()*+,-./0123456789:;<=>?\n@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n`abcdefghijklmnopqrstuvwxyz{|}~",
                 1, COLOR_WHITE);
}

void app_screen(void)
{
    static const pattern_fn patterns[] = {p_red, p_green, p_blue, p_white, p_black, p_gradient, p_grid, p_text};
    static const int levels[] = {100, 75, 50, 25, 10};
    const int n = sizeof(patterns) / sizeof(patterns[0]);
    int idx = 0, level = 0;
    const int saved_brightness = display_get_brightness();

    for (;;) {
        patterns[idx]();
        display_footer("+ siguiente  PWR brillo  BOOT volver");
        display_printf(DISPLAY_W - 60, 4, 1, idx == 3 ? COLOR_BLACK : COLOR_WHITE, "Brillo %d%%", levels[level]);
        display_flush();

        button_event_t e;
        buttons_get_event(&e, portMAX_DELAY);
        if (e.type != BTN_EVENT_CLICK) continue;
        if (e.button == BTN_PLUS) idx = (idx + 1) % n;
        if (e.button == BTN_PWR) {
            level = (level + 1) % 5;
            display_set_brightness(levels[level]);
        }
        if (e.button == BTN_BOOT) {
            display_set_brightness(saved_brightness);
            return;
        }
    }
}
