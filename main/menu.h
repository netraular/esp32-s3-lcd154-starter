#pragma once

typedef struct {
    const char *label;
    void (*run)(void);
} menu_item_t;

/**
 * Muestra una lista desplazable y ejecuta la opción elegida.
 * "+" baja, PWR entra, BOOT sube. No vuelve nunca.
 */
void menu_run(const char *title, const menu_item_t *items, int count);
