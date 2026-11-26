/*
 * app_sdcard.c — Monta la microSD (FAT32), muestra su tamaño y los ficheros
 * de la raíz. Con PWR escribe y vuelve a leer un fichero de prueba.
 *
 * Tras montarla, se usa con las funciones normales de C: fopen("/sdcard/...").
 */
#include "apps.h"
#include "board.h"
#include "buttons.h"
#include "display.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include "driver/sdmmc_host.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT "/sdcard"
#define TEST_FILE   MOUNT_POINT "/starter_test.txt"

static void wait_back(void)
{
    display_footer("BOOT volver");
    display_flush();
    button_event_t e;
    while (!(buttons_get_event(&e, portMAX_DELAY) && e.type == BTN_EVENT_CLICK && e.button == BTN_BOOT)) {
    }
}

void app_sdcard(void)
{
    display_clear(COLOR_BLACK);
    display_header("Tarjeta SD");
    display_text_center(100, "Montando...", 2, COLOR_GRAY);
    display_flush();

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = PIN_SD_CLK;
    slot.cmd = PIN_SD_CMD;
    slot.d0 = PIN_SD_D0;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    const esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,   // nunca borrar la tarjeta del usuario
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };
    sdmmc_card_t *card = NULL;
    const esp_err_t err = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot, &mount_cfg, &card);
    if (err != ESP_OK) {
        display_clear(COLOR_BLACK);
        display_header("Tarjeta SD");
        display_text_center(80, "No se pudo montar", 2, COLOR_RED);
        display_text_center(110, esp_err_to_name(err), 1, COLOR_GRAY);
        display_text_center(130, "¿Está insertada y en FAT32?", 1, COLOR_WHITE);
        wait_back();
        return;
    }

    char status[48] = "PWR: probar escritura";
    for (;;) {
        uint64_t total = 0, free_bytes = 0;
        esp_vfs_fat_info(MOUNT_POINT, &total, &free_bytes);

        display_clear(COLOR_BLACK);
        display_header("Tarjeta SD");
        display_printf(8, 32, 1, COLOR_CYAN, "%s  %s", card->cid.name,
                       (card->ocr & (1 << 30)) ? "SDHC/SDXC" : "SDSC");
        display_printf(8, 44, 1, COLOR_WHITE, "Capacidad: %llu MB",
                       (unsigned long long)card->csd.capacity * card->csd.sector_size / (1024 * 1024));
        display_printf(8, 56, 1, COLOR_WHITE, "Libre: %llu / %llu MB",
                       (unsigned long long)(free_bytes / (1024 * 1024)), (unsigned long long)(total / (1024 * 1024)));
        display_printf(8, 68, 1, COLOR_GRAY, "Bus 1 bit, %d kHz", (int)card->max_freq_khz);

        display_text(8, 86, "Raíz de la tarjeta:", 1, COLOR_CYAN);
        int y = 100, shown = 0;
        DIR *dir = opendir(MOUNT_POINT);
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL && shown < 9) {
                char line[40];
                snprintf(line, sizeof(line), "%s %.35s", ent->d_type == DT_DIR ? "[D]" : "   ", ent->d_name);
                display_text(8, y, line, 1, ent->d_type == DT_DIR ? COLOR_YELLOW : COLOR_WHITE);
                y += 11;
                shown++;
            }
            closedir(dir);
        }
        if (shown == 0) display_text(8, y, "(vacía)", 1, COLOR_GRAY);
        display_text(8, 204, status, 1, COLOR_ORANGE);
        display_footer("PWR escribir   BOOT volver");
        display_flush();

        button_event_t e;
        buttons_get_event(&e, portMAX_DELAY);
        if (e.type != BTN_EVENT_CLICK) continue;
        if (e.button == BTN_BOOT) break;
        if (e.button == BTN_PWR) {
            // Escribir un fichero de texto y leerlo de nuevo para comprobarlo
            char written[48], readback[48] = "";
            snprintf(written, sizeof(written), "Hola desde ESP32, t=%lld ms", esp_timer_get_time() / 1000);
            FILE *f = fopen(TEST_FILE, "w");
            if (f) {
                fputs(written, f);
                fclose(f);
                f = fopen(TEST_FILE, "r");
                if (f) {
                    if (fgets(readback, sizeof(readback), f) == NULL) readback[0] = '\0';
                    fclose(f);
                }
            }
            snprintf(status, sizeof(status), "%s", strcmp(written, readback) == 0 ?
                     "Escritura OK: starter_test.txt" : "Error de escritura/lectura");
        }
    }
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
}
