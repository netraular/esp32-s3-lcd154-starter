#include "board.h"
#include "esp_log.h"

static const char *TAG = "board";
static i2c_master_bus_handle_t s_i2c_bus;

void board_power_hold(void)
{
    // Con batería, al pulsar PWR la placa arranca; el firmware debe poner
    // BAT_EN a 1 enseguida o se apagará al soltar el botón.
    gpio_set_level(PIN_BAT_EN, 1);
    gpio_set_direction(PIN_BAT_EN, GPIO_MODE_OUTPUT);
}

void board_power_off(void)
{
    ESP_LOGI(TAG, "Apagando (BAT_EN = 0)");
    gpio_set_level(PIN_BAT_EN, 0);
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
