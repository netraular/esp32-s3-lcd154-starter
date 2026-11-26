/*
 * battery.h — Estado de la batería, medido en segundo plano.
 *
 * Una tarea lee la tensión cada segundo desde el arranque, la suaviza y decide
 * el estado. Cualquier parte del programa puede consultarlo con battery_get().
 *
 * Limitaciones de la placa: el porcentaje se estima a partir de la tensión
 * (no hay medidor de carga), y no existe ninguna señal de "batería llena".
 * "Carga completa" se deduce así:
 *   - CHG = 1 (no carga) y conectada a un ordenador (el USB recibe tramas), o
 *   - CHG pasa de 0 a 1 mientras la tensión es alta (el cargador terminó).
 */
#pragma once

#include <stdbool.h>

typedef enum {
    BATTERY_DISCHARGING,   // funcionando con batería (o no se puede saber otra cosa)
    BATTERY_CHARGING,      // el cargador está cargando (CHG = 0)
    BATTERY_FULL,          // carga terminada y todavía conectada
} battery_state_t;

typedef struct {
    battery_state_t state;
    int  mv;          // tensión de la batería suavizada (mV)
    int  percent;     // 0..100; -1 mientras carga (la tensión no es fiable)
    int  adc_mv;      // última lectura del ADC en GPIO1 (mV, antes de multiplicar por 3)
    bool calibrated;  // el ADC usa la calibración de fábrica
    bool usb_host;    // hay un ordenador al otro lado del USB
    bool valid;       // ya hay al menos una lectura
} battery_status_t;

#define BATTERY_HISTORY_LEN      200
#define BATTERY_HISTORY_PERIOD_S 6     // una muestra cada 6 s -> 20 minutos

void battery_init(void);

/** Copia el estado actual. */
void battery_get(battery_status_t *out);

/** Copia el historial de tensiones (mV, de la más antigua a la más reciente). Devuelve cuántas hay. */
int battery_history(int *out_mv, int max);
