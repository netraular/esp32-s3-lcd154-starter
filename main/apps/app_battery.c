/*
 * app_battery.c — Tensión de la batería (ADC) y estado de carga.
 *
 * La batería llega a GPIO1 a través de un divisor 200k/100k, así que el ADC
 * ve 1/3 de la tensión real. Sin batería conectada la lectura no es fiable.
 */
#include "apps.h"
#include "board.h"
#include "buttons.h"
#include "display.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#define HISTORY 200

/** Porcentaje aproximado a partir de la tensión (curva típica Li-ion en reposo). */
static int battery_percent(int mv)
{
    static const int table[][2] = {
        {4200, 100}, {4100, 90}, {4000, 78}, {3900, 64}, {3800, 48},
        {3700, 30},  {3600, 15}, {3500, 5},  {3300, 0},
    };
    if (mv >= table[0][0]) return 100;
    for (int i = 1; i < (int)(sizeof(table) / sizeof(table[0])); i++) {
        if (mv >= table[i][0]) {
            return table[i][1] + (mv - table[i][0]) * (table[i - 1][1] - table[i][1]) / (table[i - 1][0] - table[i][0]);
        }
    }
    return 0;
}

void app_battery(void)
{
    adc_oneshot_unit_handle_t adc;
    const adc_oneshot_unit_init_cfg_t unit_cfg = {.unit_id = ADC_UNIT_1};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc));
    const adc_oneshot_chan_cfg_t chan_cfg = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, ADC_CHANNEL_0, &chan_cfg));   // GPIO1

    // Calibración de fábrica del ADC (convierte lectura cruda en milivoltios)
    adc_cali_handle_t cali = NULL;
    const adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1, .chan = ADC_CHANNEL_0, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    const bool calibrated = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali) == ESP_OK;

    gpio_config_t chg = {.pin_bit_mask = 1ULL << PIN_CHARGING, .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE};
    gpio_config(&chg);

    static int history[HISTORY];
    int count = 0;

    for (;;) {
        // Media de 32 lecturas para reducir ruido
        int raw_sum = 0, mv = 0;
        for (int i = 0; i < 32; i++) {
            int raw = 0;
            adc_oneshot_read(adc, ADC_CHANNEL_0, &raw);
            raw_sum += raw;
        }
        const int raw = raw_sum / 32;
        if (calibrated) adc_cali_raw_to_voltage(cali, raw, &mv);
        else mv = raw * 3100 / 4095;
        const int bat_mv = mv * 3;
        const bool charging = gpio_get_level(PIN_CHARGING) == 0;

        if (count < HISTORY) history[count++] = bat_mv;
        else {
            for (int i = 1; i < HISTORY; i++) history[i - 1] = history[i];
            history[HISTORY - 1] = bat_mv;
        }

        display_clear(COLOR_BLACK);
        display_header("Batería");
        const int pct = battery_percent(bat_mv);
        // Icono de batería
        display_frame(20, 40, 120, 50, COLOR_WHITE);
        display_rect(140, 55, 8, 20, COLOR_WHITE);
        const uint16_t fill = pct > 50 ? COLOR_GREEN : pct > 20 ? COLOR_YELLOW : COLOR_RED;
        display_rect(24, 44, 112 * pct / 100, 42, fill);
        display_printf(160, 44, 2, COLOR_WHITE, "%d%%", pct);
        display_printf(160, 68, 2, COLOR_WHITE, "%d.%02dV", bat_mv / 1000, (bat_mv % 1000) / 10);

        display_text(20, 100, charging ? "Cargando (CHG = 0)" : "No carga (CHG = 1)", 1,
                     charging ? COLOR_GREEN : COLOR_GRAY);
        display_printf(20, 114, 1, COLOR_GRAY, "ADC GPIO1: crudo %d, %d mV x3%s", raw, mv, calibrated ? "" : " (sin cal.)");

        // Gráfica de las últimas lecturas (3.0 V .. 4.4 V)
        const int gx = 20, gy = 132, gw = HISTORY, gh = 70;
        display_frame(gx - 1, gy - 1, gw + 2, gh + 2, COLOR_DARK);
        for (int i = 0; i < count; i++) {
            int v = (history[i] - 3000) * gh / 1400;
            if (v < 0) v = 0;
            if (v > gh - 1) v = gh - 1;
            display_pixel(gx + i, gy + gh - 1 - v, COLOR_CYAN);
        }
        display_text(gx, gy + gh + 4, "4.4V..3.0V, 1 lectura/s. Valor aproximado.", 1, COLOR_GRAY);
        display_footer("BOOT volver");
        display_flush();

        button_event_t e;
        if (buttons_get_event(&e, pdMS_TO_TICKS(1000)) && e.type == BTN_EVENT_CLICK && e.button == BTN_BOOT) break;
    }

    if (calibrated) adc_cali_delete_scheme_curve_fitting(cali);
    adc_oneshot_del_unit(adc);
}
