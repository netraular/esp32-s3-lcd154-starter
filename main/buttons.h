/*
 * The three physical buttons, with debouncing and long-press detection.
 *
 * A task samples the buttons every 10 ms and queues events for
 * buttons_get_event().
 *
 * Typical controls: + moves down or changes a value, PWR selects an item
 * or runs an action, and BOOT moves up in the menu or goes back in an app.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"

typedef enum {
    BTN_BOOT = 0,
    BTN_PLUS,
    BTN_PWR,
    BTN_COUNT
} button_id_t;

typedef enum {
    BTN_EVENT_DOWN,     // Button pressed.
    BTN_EVENT_UP,       // Button released.
    BTN_EVENT_CLICK,    // Short press, emitted on release.
    BTN_EVENT_LONG,     // Emitted once after BUTTON_LONG_MS.
} button_event_type_t;

typedef struct {
    button_id_t button;
    button_event_type_t type;
} button_event_t;

#define BUTTON_LONG_MS 700

void buttons_init(void);

/**
 * Wait up to timeout ticks for an event (0 is non-blocking, portMAX_DELAY
 * waits indefinitely). Return true if an event was received.
 */
bool buttons_get_event(button_event_t *event, TickType_t timeout);

/** Return the current debounced state of a button. */
bool buttons_is_down(button_id_t button);

/** Discard pending events. */
void buttons_flush(void);

/** Return the button label ("BOOT", "+", "PWR"). */
const char *buttons_name(button_id_t button);
