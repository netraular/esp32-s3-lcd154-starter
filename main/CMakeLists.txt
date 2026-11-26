idf_component_register(
    SRCS
        "main.c"
        "board.c"
        "display.c"
        "buttons.c"
        "menu.c"
        "battery.c"
        "apps/app_template.c"
        "apps/app_info.c"
        "apps/app_buttons.c"
        "apps/app_screen.c"
        "apps/app_help_qr.c"
        "apps/app_battery.c"
        "apps/app_imu.c"
        "apps/app_i2c_scan.c"
        "apps/app_sdcard.c"
        "apps/app_sound.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_gpio esp_driver_spi esp_driver_i2c esp_driver_i2s
        esp_driver_ledc esp_driver_sdmmc esp_lcd esp_adc fatfs
        esp_timer esp_psram spi_flash esp_hw_support esp_system
        esp_driver_usb_serial_jtag
)
