#include "board.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "board";
static i2c_master_bus_handle_t s_i2c_bus;

/** Set an output level and hold it through deep sleep. */
static void hold_output(gpio_num_t pin, int level)
{
    gpio_hold_dis(pin);
    gpio_set_level(pin, level);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_hold_en(pin);
}

void board_power_hold(void)
{
    // On battery power, assert BAT_EN before the user releases PWR.
    gpio_set_level(PIN_BAT_EN, 1);
    gpio_set_direction(PIN_BAT_EN, GPIO_MODE_OUTPUT);

    // Release GPIO holds left in place by board_power_off().
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis(PIN_BAT_EN);
    gpio_hold_dis(PIN_LCD_BL);
    gpio_hold_dis(PIN_AUDIO_PA);
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED) {
        rtc_gpio_deinit(PIN_BTN_PWR);   // Return PWR to normal GPIO mode.
        ESP_LOGI(TAG, "Woke from PWR");
    }
}

void board_power_off(void)
{
    // 1. Try to cut battery power.
    ESP_LOGI(TAG, "Powering off: BAT_EN = 0");
    hold_output(PIN_BAT_EN, 0);
    vTaskDelay(pdMS_TO_TICKS(500));

    // 2. If still powered, use deep sleep with PWR as the wake source.
    ESP_LOGI(TAG, "Still powered: entering deep sleep until PWR is pressed");
    while (gpio_get_level(PIN_BTN_PWR) == 0) vTaskDelay(pdMS_TO_TICKS(10));  // Wait for PWR release.
    vTaskDelay(pdMS_TO_TICKS(50));

    hold_output(PIN_LCD_BL, 0);     // Backlight off.
    hold_output(PIN_AUDIO_PA, 0);   // Amplifier off.
    gpio_deep_sleep_hold_en();

    rtc_gpio_pullup_en(PIN_BTN_PWR);
    rtc_gpio_pulldown_dis(PIN_BTN_PWR);
    esp_sleep_enable_ext1_wakeup_io(1ULL << PIN_BTN_PWR, ESP_EXT1_WAKEUP_ANY_LOW);
    esp_deep_sleep_start();
}

i2c_master_bus_handle_t board_i2c_bus(void)
{
    if (s_i2c_bus == NULL) {
        const i2c_master_bus_config_t cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = PIN_I2C_SDA,
            .scl_io_num = PIN_I2C_SCL,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &s_i2c_bus));
    }
    return s_i2c_bus;
}
