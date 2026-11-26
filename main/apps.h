/*
 * An app is a function that owns the display and buttons until it returns
 * to the main menu.
 *
 * To add an app, copy apps/app_template.c and rename its function. Declare
 * it here, add the source to main/CMakeLists.txt, and add an entry to MENU
 * in main.c.
 */
#pragma once

void app_template(void);
void app_info(void);
void app_buttons(void);
void app_screen(void);
void app_help_qr(void);
void app_battery(void);
void app_imu(void);
void app_i2c_scan(void);
void app_sdcard(void);
void app_sound(void);
