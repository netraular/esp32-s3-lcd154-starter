/*
 * Battery status, measured in the background.
 *
 * A task samples and smooths the voltage once a second. Use battery_get()
 * to read the latest status.
 *
 * The board has no fuel gauge or full-charge signal. Percentage is estimated
 * from voltage. Full charge is inferred when CHG is high with a USB host
 * present, or when CHG goes high while battery voltage is high.
 */
#pragma once

#include <stdbool.h>

typedef enum {
    BATTERY_DISCHARGING,   // On battery power, or the charger state is unknown.
    BATTERY_CHARGING,      // Charger active (CHG = 0).
    BATTERY_FULL,          // Charging finished and charger still connected.
} battery_state_t;

typedef struct {
    battery_state_t state;
    int  mv;          // Smoothed battery voltage (mV).
    int  percent;     // 0..100; -1 if charging makes voltage unreliable.
    int  adc_mv;      // Latest GPIO1 ADC reading (mV), before divider correction.
    bool calibrated;  // Factory ADC calibration is in use.
    bool usb_host;    // A USB host is connected.
    bool valid;       // At least one reading is available.
} battery_status_t;

#define BATTERY_HISTORY_LEN      200
#define BATTERY_HISTORY_PERIOD_S 6     // One sample every 6 s (20 minutes total).

void battery_init(void);

/** Copy the current status. */
void battery_get(battery_status_t *out);

/** Copy voltage samples (mV), oldest first, and return the number copied. */
int battery_history(int *out_mv, int max);
