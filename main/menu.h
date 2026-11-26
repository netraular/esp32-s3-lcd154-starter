#pragma once

typedef struct {
    const char *label;
    void (*run)(void);
} menu_item_t;

/**
 * Run the scrollable menu. + moves down, PWR opens an item, and BOOT
 * moves up. This function does not return.
 */
void menu_run(const char *title, const menu_item_t *items, int count);
