/*
 * Mounts the microSD card (FAT32) and shows its capacity and root directory.
 * PWR writes and reads back a temporary test file.
 *
 * Once mounted, the card is accessible via standard C file functions.
 */
#include "apps.h"
#include "board.h"
#include "buttons.h"
#include "display.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "driver/sdmmc_host.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT "/sdcard"
#define TEST_FILE   MOUNT_POINT "/starter_test.txt"

static void wait_back(void)
{
    display_footer("BOOT back");
    display_flush();
    button_event_t e;
    while (!(buttons_get_event(&e, portMAX_DELAY) && e.type == BTN_EVENT_CLICK && e.button == BTN_BOOT)) {
    }
}

static const char *test_write(void)
{
    int fd = open(TEST_FILE, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd < 0) {
        return errno == EEXIST ? "Test file already exists" : "Cannot create test file";
    }

    char written[48], readback[48];
    snprintf(written, sizeof(written), "Hello from ESP32, t=%lld ms", esp_timer_get_time() / 1000);
    FILE *file = fdopen(fd, "w+");
    bool ok = false;
    if (file) {
        ok = fputs(written, file) != EOF && fflush(file) == 0 && fseek(file, 0, SEEK_SET) == 0 &&
             fgets(readback, sizeof(readback), file) != NULL && strcmp(written, readback) == 0;
        if (fclose(file) != 0) ok = false;
    } else {
        close(fd);
    }

    if (unlink(TEST_FILE) != 0) return "Cannot remove starter_test.txt";
    return ok ? "Write and read OK" : "Write/read error";
}

void app_sdcard(void)
{
    display_clear(COLOR_BLACK);
    display_header("microSD");
    display_text_center(100, "Mounting...", 2, COLOR_GRAY);
    display_flush();

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = PIN_SD_CLK;
    slot.cmd = PIN_SD_CMD;
    slot.d0 = PIN_SD_D0;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    const esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,   // Never erase the user's card.
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };
    sdmmc_card_t *card = NULL;
    const esp_err_t err = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot, &mount_cfg, &card);
    if (err != ESP_OK) {
        display_clear(COLOR_BLACK);
        display_header("microSD");
        display_text_center(80, "Mount failed", 2, COLOR_RED);
        display_text_center(110, esp_err_to_name(err), 1, COLOR_GRAY);
        display_text_center(130, "Insert a FAT32 card", 1, COLOR_WHITE);
        wait_back();
        return;
    }

    const char *status = "PWR: test write";
    for (;;) {
        uint64_t total = 0, free_bytes = 0;
        esp_vfs_fat_info(MOUNT_POINT, &total, &free_bytes);

        display_clear(COLOR_BLACK);
        display_header("microSD");
        display_printf(8, 32, 1, COLOR_CYAN, "%s  %s", card->cid.name,
                       (card->ocr & (1 << 30)) ? "SDHC/SDXC" : "SDSC");
        display_printf(8, 44, 1, COLOR_WHITE, "Capacity: %llu MiB",
                       (unsigned long long)card->csd.capacity * card->csd.sector_size / (1024 * 1024));
        display_printf(8, 56, 1, COLOR_WHITE, "Free: %llu / %llu MiB",
                       (unsigned long long)(free_bytes / (1024 * 1024)), (unsigned long long)(total / (1024 * 1024)));
        display_printf(8, 68, 1, COLOR_GRAY, "1-bit bus, %d kHz", (int)card->max_freq_khz);

        display_text(8, 86, "Root directory:", 1, COLOR_CYAN);
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
        if (shown == 0) display_text(8, y, "(empty)", 1, COLOR_GRAY);
        display_text(8, 204, status, 1, COLOR_ORANGE);
        display_footer("PWR test write   BOOT back");
        display_flush();

        button_event_t e;
        buttons_get_event(&e, portMAX_DELAY);
        if (e.type != BTN_EVENT_CLICK) continue;
        if (e.button == BTN_BOOT) break;
        if (e.button == BTN_PWR) status = test_write();
    }
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
}
