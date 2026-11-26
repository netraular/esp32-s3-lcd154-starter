#include "battery.h"
#include "board.h"

#include <string.h>
#include "driver/usb_serial_jtag.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "battery";

#define SAMPLES_PER_READ   64
#define FULL_MIN_MV        4100   // tensión mínima para dar la carga por terminada
#define UNPLUG_DROP_MV     40     // caída que indica que se desconectó el cargador

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static battery_status_t s_status;
static int s_history[BATTERY_HISTORY_LEN];
static int s_history_count;

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;
static bool s_calibrated;

/** Porcentaje aproximado a partir de la tensión (curva típica Li-ion). */
static int percent_from_mv(int mv)
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

/** Media de muchas lecturas del ADC, en mV en el pin. */
static int read_adc_mv(void)
{
    int sum = 0;
    for (int i = 0; i < SAMPLES_PER_READ; i++) {
        int raw = 0;
        adc_oneshot_read(s_adc, ADC_CHANNEL_0, &raw);
        sum += raw;
    }
    const int raw = sum / SAMPLES_PER_READ;
    int mv = 0;
    if (s_calibrated) adc_cali_raw_to_voltage(s_cali, raw, &mv);
    else mv = raw * 3100 / 4095;
    return mv;
}

static void battery_task(void *arg)
{
    float filtered = 0;          // tensión suavizada
    bool have_filtered = false;
    int shown_percent = -1;      // con batería solo puede bajar
    bool was_charging = false;
    bool full = false;
    int full_mv = 0;
    battery_state_t last_state = BATTERY_DISCHARGING;
    int tick = 0;

    for (;;) {
        const int adc_mv = read_adc_mv();
        const int bat_mv = adc_mv * 3;               // divisor 200k/100k
        const bool charging = gpio_get_level(PIN_CHARGING) == 0;
        const bool usb_host = usb_serial_jtag_is_connected();

        // --- ¿Carga completa? ---
        if (charging) {
            full = false;
        } else if (!full) {
            const int v = have_filtered ? (int)filtered : bat_mv;
            if (usb_host || (was_charging && v >= FULL_MIN_MV)) {
                full = true;
                full_mv = v;
            }
        } else if (!usb_host) {
            // Con cargador de pared: si la tensión cae, es que lo han desconectado.
            if ((int)filtered > full_mv) full_mv = (int)filtered;
            if ((int)filtered < full_mv - UNPLUG_DROP_MV) full = false;
        }
        was_charging = charging;

        const battery_state_t state = charging ? BATTERY_CHARGING : full ? BATTERY_FULL : BATTERY_DISCHARGING;

        // --- Suavizado: al cambiar de estado se empieza de nuevo ---
        if (!have_filtered || state != last_state) {
            filtered = bat_mv;
            have_filtered = true;
            shown_percent = -1;
        } else {
            filtered += (bat_mv - filtered) * 0.1f;  // media móvil exponencial (~10 s)
        }
        last_state = state;

        // --- Porcentaje ---
        int percent;
        if (state == BATTERY_CHARGING) {
            percent = -1;
        } else if (state == BATTERY_FULL) {
            percent = 100;
        } else {
            const int p = percent_from_mv((int)filtered);
            // Sin cargador la batería no gana carga: si "sube" es ruido, se ignora.
            if (shown_percent < 0 || p < shown_percent) shown_percent = p;
            percent = shown_percent;
        }

        portENTER_CRITICAL(&s_lock);
        s_status = (battery_status_t){
            .state = state, .mv = (int)filtered, .percent = percent, .adc_mv = adc_mv,
            .calibrated = s_calibrated, .usb_host = usb_host, .valid = true,
        };
        if (tick % BATTERY_HISTORY_PERIOD_S == 0) {
            if (s_history_count < BATTERY_HISTORY_LEN) {
                s_history[s_history_count++] = (int)filtered;
            } else {
                memmove(s_history, s_history + 1, (BATTERY_HISTORY_LEN - 1) * sizeof(int));
                s_history[BATTERY_HISTORY_LEN - 1] = (int)filtered;
            }
        }
        portEXIT_CRITICAL(&s_lock);

        tick++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void battery_init(void)
{
    const adc_oneshot_unit_init_cfg_t unit_cfg = {.unit_id = ADC_UNIT_1};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));
    const adc_oneshot_chan_cfg_t chan_cfg = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, ADC_CHANNEL_0, &chan_cfg));   // GPIO1

    // Calibración de fábrica del ADC (convierte la lectura cruda en milivoltios)
    const adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1, .chan = ADC_CHANNEL_0, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    s_calibrated = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali) == ESP_OK;
    if (!s_calibrated) ESP_LOGW(TAG, "ADC sin calibración de fábrica");

    const gpio_config_t chg = {
        .pin_bit_mask = 1ULL << PIN_CHARGING, .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&chg);

    xTaskCreate(battery_task, "battery", 3072, NULL, 3, NULL);
}

void battery_get(battery_status_t *out)
{
    portENTER_CRITICAL(&s_lock);
    *out = s_status;
    portEXIT_CRITICAL(&s_lock);
}

int battery_history(int *out_mv, int max)
{
    portENTER_CRITICAL(&s_lock);
    const int n = s_history_count < max ? s_history_count : max;
    memcpy(out_mv, s_history + (s_history_count - n), n * sizeof(int));
    portEXIT_CRITICAL(&s_lock);
    return n;
}
