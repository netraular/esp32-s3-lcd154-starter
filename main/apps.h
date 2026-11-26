/*
 * apps.h — Cada "app" es una función que toma el control de la pantalla y
 * los botones, y vuelve (return) cuando el usuario quiere regresar al menú.
 *
 * Para añadir tu propia app:
 *   1. Copia apps/app_template.c a apps/app_loquesea.c y renombra la función.
 *   2. Declárala aquí abajo.
 *   3. Añade el .c a main/CMakeLists.txt.
 *   4. Añade una entrada en la tabla MENU[] de main.c.
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
