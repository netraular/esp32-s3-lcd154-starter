/*
 * buttons.h — Los 3 botones físicos con antirrebote y pulsación larga.
 *
 * Una tarea lee los botones cada 10 ms y deja "eventos" en una cola.
 * Tu código los recoge con buttons_get_event().
 *
 * Convenio usado en todo el firmware:
 *   "+"    (PLUS)  -> siguiente / bajar / cambiar
 *   "PWR"          -> aceptar / acción
 *   "BOOT"         -> volver atrás
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
    BTN_EVENT_DOWN,     // se acaba de pulsar
    BTN_EVENT_UP,       // se acaba de soltar
    BTN_EVENT_CLICK,    // pulsación corta (se emite al soltar)
    BTN_EVENT_LONG,     // sigue pulsado tras BUTTON_LONG_MS (se emite una vez)
} button_event_type_t;

typedef struct {
    button_id_t button;
    button_event_type_t type;
} button_event_t;

#define BUTTON_LONG_MS 700

void buttons_init(void);

/**
 * Espera un evento hasta "timeout" ticks (0 = no esperar, portMAX_DELAY = siempre).
 * Devuelve true si llegó alguno.
 */
bool buttons_get_event(button_event_t *event, TickType_t timeout);

/** Estado actual (ya sin rebotes) de un botón. */
bool buttons_is_down(button_id_t button);

/** Descarta los eventos pendientes. */
void buttons_flush(void);

/** Nombre legible del botón ("BOOT", "+", "PWR"). */
const char *buttons_name(button_id_t button);
