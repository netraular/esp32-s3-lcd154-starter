# esp32-s3-lcd154-starter

Firmware de iniciación para la placa **Waveshare ESP32-S3-LCD-1.54**. Sirve para
probar todo el hardware de la placa y como punto de partida para tus programas.

Está escrito en C con **ESP-IDF**, el entorno oficial de Espressif. No usa
librerías gráficas: la pantalla se maneja con un lienzo en memoria y una fuente
5x7 muy simple. Así todo el código se puede leer y entender en poco tiempo.

## Instalar sin programar nada

Abre **https://netraular.github.io/esp32-s3-lcd154-starter/** con Chrome, Edge u
Opera en un ordenador, conecta la placa por USB-C y pulsa *Conectar e instalar*.
La página se genera automáticamente con GitHub Actions en cada cambio de `main`
(ver `.github/workflows/firmware.yml` y la carpeta `web/`).

## Controles

La placa tiene 3 botones; en la placa están en este orden: `BOOT`, `PWR`, `+`.
En todo el firmware se usan igual:

| Botón  | Uso                               |
|--------|-----------------------------------|
| `BOOT` | volver atrás (en el menú: subir)  |
| `PWR`  | aceptar / acción                  |
| `+`    | siguiente / bajar / cambiar       |

Con batería, mantén `PWR` pulsado para encender la placa.

## Menú

| Opción        | Qué hace                                                                  |
|---------------|---------------------------------------------------------------------------|
| Mi app        | Plantilla con un contador y una pelota. **Empieza a programar aquí.**     |
| Ayuda (QR)    | Códigos QR: instalador web, este código, documentación, ejemplos, vídeo y ESP-IDF |
| Botones       | Estado de cada botón en tiempo real y contador de pulsaciones cortas/largas |
| Pantalla      | Colores puros, degradados, rejilla, texto y brillo                         |
| Info placa    | Chip, memoria flash/PSRAM, RAM libre, MAC y versión de ESP-IDF            |
| Batería       | Tensión, porcentaje aproximado, estado de carga y gráfica                 |
| Movimiento    | Acelerómetro y giroscopio QMI8658 con nivel de burbuja                     |
| Sonido        | Toca una escala por el altavoz y cambia el volumen                        |
| Tarjeta SD    | Monta la microSD, enseña su tamaño y los ficheros, y prueba a escribir    |
| Escáner I2C   | Lista los chips del bus I2C (sirve también para tus propios sensores)     |
| Apagar        | Corta la batería (con el USB conectado la placa sigue encendida)          |

## Instalar ESP-IDF

Opción recomendada: **VS Code + extensión "ESP-IDF"** de Espressif.

1. Instala [VS Code](https://code.visualstudio.com/).
2. Instala la extensión **ESP-IDF** (de Espressif Systems).
3. Abre la paleta (`Ctrl+Shift+P`), ejecuta **"ESP-IDF: Configure ESP-IDF Extension"**
   y elige la instalación *Express* con la versión **v5.5** (o cualquier 5.3 o posterior).
4. Abre esta carpeta en VS Code.

En la barra inferior aparecen botones para elegir el puerto, el *target*
(`esp32s3`), compilar, grabar y abrir el monitor. Más información en la
[guía oficial](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/index.html).

También puedes usar la línea de comandos: instala ESP-IDF según esa guía y
activa el entorno con `. $HOME/esp/esp-idf/export.sh` (Linux/macOS) o con el
acceso directo "ESP-IDF PowerShell" (Windows).

## Compilar, grabar y ver el monitor

Conecta la placa por USB-C y ejecuta:

```bash
idf.py set-target esp32s3   # solo la primera vez
idf.py build                # compilar
idf.py -p /dev/ttyACM0 flash monitor   # grabar y abrir el monitor serie
```

- En Windows el puerto es `COM3`, `COM4`... (míralo en el Administrador de dispositivos).
- Para salir del monitor: `Ctrl+]`.
- Los mensajes `ESP_LOGI(...)` y `printf(...)` aparecen en el monitor.
- La primera compilación tarda unos minutos y descarga dos componentes
  (`qrcode` y `esp_codec_dev`) a `managed_components/`.

**Si no graba** (no encuentra el puerto o da error de conexión): desconecta el USB,
mantén pulsado `BOOT`, vuelve a conectar el USB, suelta `BOOT` y repite `flash`.
Así la placa entra en modo descarga. Después de grabar, desconecta y conecta el
USB para arrancar el programa nuevo.

Sin instalar nada, también se puede compilar con Docker:

```bash
docker run --rm -v $PWD:/project -w /project espressif/idf:v5.5.1 idf.py build
```

## Estructura del proyecto

```
CMakeLists.txt          proyecto ESP-IDF
.github/workflows/      compila y publica el instalador web en GitHub Pages
web/                    página del instalador (ESP Web Tools) y su manifest
sdkconfig.defaults      configuración de la placa (flash 16 MB, PSRAM 8 MB, 240 MHz)
main/
  main.c                arranque y tabla del menú
  board.h / board.c     pines de la placa, encendido/apagado y bus I2C
  display.h / .c        pantalla: lienzo, rectángulos, líneas, círculos y texto
  buttons.h / .c        botones con antirrebote y pulsación larga
  menu.h / .c           menú desplazable
  apps.h                lista de apps
  apps/app_*.c          una app por fichero
```

## Cómo crear tu propia app

1. Copia `main/apps/app_template.c` como `main/apps/app_juego.c` y cambia el
   nombre de la función a `app_juego`.
2. Declárala en `main/apps.h`: `void app_juego(void);`
3. Añade `"apps/app_juego.c"` en `main/CMakeLists.txt`.
4. Añade `{"Mi juego", app_juego},` en la tabla `MENU[]` de `main/main.c`.

Una app es una función con un bucle. Cuando la función termina (`return`), se
vuelve al menú:

```c
void app_juego(void)
{
    for (;;) {
        button_event_t e;
        while (buttons_get_event(&e, 0)) {              // 1. leer botones
            if (e.type == BTN_EVENT_CLICK && e.button == BTN_BOOT) return;
        }
        // 2. actualizar variables...
        display_clear(COLOR_BLACK);                     // 3. dibujar
        display_text(10, 10, "¡Hola!", 3, COLOR_YELLOW);
        display_flush();                                // enviar a la pantalla
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
```

### Resumen de funciones

| Función | Qué hace |
|---------|----------|
| `display_clear(color)` | Pinta todo el lienzo de un color |
| `display_pixel(x, y, color)` | Un píxel |
| `display_rect(x, y, w, h, color)` / `display_frame(...)` | Rectángulo relleno / solo borde |
| `display_line(x0, y0, x1, y1, color)` | Línea |
| `display_circle(cx, cy, r, color, relleno)` | Círculo |
| `display_text(x, y, "texto", escala, color)` | Texto (escala 1 = 6x8 px por letra) |
| `display_printf(x, y, escala, color, "%d", n)` | Texto con formato, como `printf` |
| `display_flush()` | Envía el lienzo a la pantalla. Sin esto no se ve nada |
| `display_set_brightness(0..100)` | Brillo |
| `buttons_get_event(&e, espera)` | Recoge un evento: `BTN_EVENT_DOWN`, `UP`, `CLICK` o `LONG` |
| `buttons_is_down(BTN_PLUS)` | ¿Está pulsado ahora mismo? |
| `RGB565(r, g, b)` | Crea un color a partir de rojo, verde y azul (0..255) |

La pantalla mide 240x240. El punto (0,0) está arriba a la izquierda.

## Pines de la placa

| Elemento | GPIO |
|----------|------|
| Pantalla ST7789 (SPI) | SCLK 38, MOSI 39, CS 21, DC 45, RST 40, retroiluminación 46 |
| Botones (activos a nivel bajo; orden físico BOOT, PWR, `+`) | BOOT 0, PWR 5, `+` 4 |
| Batería | mantener encendido 2, ADC (tensión/3) 1, cargando 3 (0 = sí) |
| I2C (IMU QMI8658 0x6B, ES8311 0x18, ES7210 0x40) | SDA 42, SCL 41 |
| Audio I2S | MCLK 8, BCLK 9, WS 10, DIN 11, DOUT 12, amplificador 7 |
| microSD (1 bit) | CLK 16, CMD 15, D0 17 |

Todo está en `main/board.h`. El esquema completo está en la página de recursos
de Waveshare (enlace abajo y en el menú *Ayuda (QR)*).

## Enlaces

- [Página del producto](https://www.waveshare.com/esp32-s3-lcd-1.54.htm)
- [Documentación de Waveshare](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54)
- [Esquemas y datasheets](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.54/Resources-And-Documents)
- [Ejemplos oficiales en GitHub](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.54)
- [Vídeo de la placa](https://www.youtube.com/watch?v=McWEw64yzAA)
- [ESP-IDF: primeros pasos](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/index.html)
- [ESP-IDF: referencia de la API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/index.html)
- [Ejemplos de ESP-IDF](https://github.com/espressif/esp-idf/tree/master/examples) (WiFi, Bluetooth, HTTP, etc.)

## Ideas para seguir

- Un juego sencillo con los 3 botones (Snake, Pong...) partiendo de *Mi app*.
- Mover algo por la pantalla inclinando la placa (ver `app_imu.c`).
- Conectarse a la WiFi y mostrar la hora o el tiempo (ejemplos `wifi/getting_started` y `protocols/sntp`).
- Guardar datos en la microSD (ver `app_sdcard.c`).
