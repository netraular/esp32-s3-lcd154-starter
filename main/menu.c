#include "menu.h"
#include "buttons.h"
#include "display.h"

#define ROW_H    19
#define LIST_Y   30
#define VISIBLE  ((DISPLAY_H - LIST_Y - 16) / ROW_H)

static void draw(const char *title, const menu_item_t *items, int count, int sel, int top)
{
    display_clear(COLOR_BLACK);
    display_header(title);
    for (int i = 0; i < VISIBLE && top + i < count; i++) {
        const int idx = top + i;
        const int y = LIST_Y + i * ROW_H;
        if (idx == sel) {
            display_rect(0, y - 2, DISPLAY_W, ROW_H, COLOR_DARK);
            display_rect(0, y - 2, 4, ROW_H, COLOR_ACCENT);
        }
        display_text(10, y, items[idx].label, 2, idx == sel ? COLOR_YELLOW : COLOR_WHITE);
    }
    // Indicador de desplazamiento
    if (count > VISIBLE) {
        const int bar_h = (DISPLAY_H - LIST_Y - 16) * VISIBLE / count;
        const int bar_y = LIST_Y + (DISPLAY_H - LIST_Y - 16 - bar_h) * top / (count - VISIBLE);
        display_rect(DISPLAY_W - 3, bar_y, 3, bar_h, COLOR_GRAY);
    }
    display_footer("+ bajar   PWR entrar   BOOT subir");
    display_flush();
}

void menu_run(const char *title, const menu_item_t *items, int count)
{
    int sel = 0, top = 0;
    for (;;) {
        if (sel < top) top = sel;
        if (sel >= top + VISIBLE) top = sel - VISIBLE + 1;
        draw(title, items, count, sel, top);

        button_event_t e;
        buttons_get_event(&e, portMAX_DELAY);
        if (e.type != BTN_EVENT_CLICK) continue;

        if (e.button == BTN_PLUS) {
            sel = (sel + 1) % count;
        } else if (e.button == BTN_BOOT) {
            sel = (sel + count - 1) % count;
        } else if (e.button == BTN_PWR) {
            buttons_flush();
            items[sel].run();
            buttons_flush();
        }
    }
}
