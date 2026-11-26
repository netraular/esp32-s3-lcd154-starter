# esp32-s3-lcd154-starter

Starter firmware in C for the **Waveshare ESP32-S3-LCD-1.54**. It uses ESP-IDF
and a simple framebuffer, with no GUI library. The menu includes board info,
hardware demos, QR help, and a small example app.

## Install from a browser

Open the [web installer](https://netraular.github.io/esp32-s3-lcd154-starter/)
in desktop Chrome or Edge. Connect the board with a USB-C data cable, choose
**Connect**, select its serial port, and choose **Install**. No SDK is needed.

If the board won't connect, close other serial tools. To enter download mode,
unplug USB, hold `BOOT`, plug USB back in, then release `BOOT`. After flashing
in download mode, reconnect USB without holding `BOOT` to start the firmware.

## Use the board

In the main menu, `BOOT` moves up, `+` moves down, and `PWR` opens the selected
item. Inside an app, `BOOT` goes back (hold it for two seconds in **Buttons**).
On battery power, hold `PWR` to turn the board on.

## Build and flash from source

Install [ESP-IDF 5.5.1](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/get-started/index.html),
the version used by GitHub Actions. You can also set it up through Espressif's
ESP-IDF extension for VS Code. In an ESP-IDF terminal, run:

```bash
idf.py set-target esp32s3  # First setup only
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Replace `/dev/ttyACM0` with your board's port (for example, `COM4` on Windows).
To exit the serial monitor, press `Ctrl+]`.

## Make it yours

Start with [the example app](main/apps/app_template.c). To add a menu item,
register the new app in [apps.h](main/apps.h),
[the component build](main/CMakeLists.txt), and `MENU` in [main.c](main/main.c).
Returning from an app takes you back to the menu.

See [display.h](main/display.h) for drawing on the 240 x 240 screen,
[buttons.h](main/buttons.h) for input, and [board.h](main/board.h) for pin
assignments. For the schematic and vendor examples, see the
[Waveshare documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54).
