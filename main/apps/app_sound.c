/*
 * Speaker test for the ES8311 codec and NS4150B amplifier.
 *
 * The ESP32 sends samples over I2S to the ES8311, then to the amplifier and
 * speaker. esp_codec_dev configures the codec over I2C. Notes are synthesized
 * as sine waves in real time.
 */
#include "apps.h"
#include "board.h"
#include "buttons.h"
#include "display.h"

#include <math.h>
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"

static const char *TAG = "sound";

#define SAMPLE_RATE 16000

typedef struct {
    i2s_chan_handle_t tx;
    const audio_codec_data_if_t *data_if;
    const audio_codec_ctrl_if_t *ctrl_if;
    const audio_codec_gpio_if_t *gpio_if;
    const audio_codec_if_t *codec_if;
    esp_codec_dev_handle_t dev;
} speaker_t;

static void speaker_close(speaker_t *s)
{
    if (s->dev) { esp_codec_dev_close(s->dev); esp_codec_dev_delete(s->dev); }
    if (s->codec_if) audio_codec_delete_codec_if(s->codec_if);
    if (s->ctrl_if) audio_codec_delete_ctrl_if(s->ctrl_if);
    if (s->gpio_if) audio_codec_delete_gpio_if(s->gpio_if);
    if (s->data_if) audio_codec_delete_data_if(s->data_if);
    if (s->tx) { i2s_channel_disable(s->tx); i2s_del_channel(s->tx); }
    *s = (speaker_t){0};
}

static bool speaker_open(speaker_t *s)
{
    *s = (speaker_t){0};

    // 1. Set up I2S output with the ESP32 providing clocks, including MCLK.
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    if (i2s_new_channel(&chan_cfg, &s->tx, NULL) != ESP_OK) goto fail;
    const i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = PIN_I2S_MCLK, .bclk = PIN_I2S_BCLK, .ws = PIN_I2S_WS,
            .dout = PIN_I2S_DOUT, .din = I2S_GPIO_UNUSED,
        },
    };
    if (i2s_channel_init_std_mode(s->tx, &std_cfg) != ESP_OK) goto fail;
    if (i2s_channel_enable(s->tx) != ESP_OK) goto fail;

    // 2. Set up data (I2S), control (I2C), and amplifier GPIO for esp_codec_dev.
    audio_codec_i2s_cfg_t i2s_cfg = {.port = I2S_NUM_0, .tx_handle = s->tx};
    s->data_if = audio_codec_new_i2s_data(&i2s_cfg);
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = I2C_NUM_0,
        .addr = ES8311_CODEC_DEFAULT_ADDR,   // 0x30 = 0x18 << 1
        .bus_handle = board_i2c_bus(),
    };
    s->ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    s->gpio_if = audio_codec_new_gpio();
    if (!s->data_if || !s->ctrl_if || !s->gpio_if) goto fail;

    // 3. Configure the ES8311 for DAC playback.
    es8311_codec_cfg_t es_cfg = {
        .ctrl_if = s->ctrl_if,
        .gpio_if = s->gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = PIN_AUDIO_PA,
        .use_mclk = true,
        .hw_gain = {.pa_voltage = 5.0, .codec_dac_voltage = 3.3},
    };
    s->codec_if = es8311_codec_new(&es_cfg);
    if (!s->codec_if) goto fail;

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = s->codec_if,
        .data_if = s->data_if,
    };
    s->dev = esp_codec_dev_new(&dev_cfg);
    if (!s->dev) goto fail;

    esp_codec_dev_sample_info_t fs = {.sample_rate = SAMPLE_RATE, .channel = 1, .bits_per_sample = 16};
    if (esp_codec_dev_open(s->dev, &fs) != ESP_CODEC_DEV_OK) goto fail;
    return true;

fail:
    ESP_LOGE(TAG, "Failed to start audio");
    speaker_close(s);
    return false;
}

/** Play a note (0 Hz is silence). Return false if BOOT is pressed. */
static bool play_tone(speaker_t *s, float freq, int ms)
{
    static int16_t buf[256];
    static float phase = 0;
    const int total = SAMPLE_RATE * ms / 1000;
    for (int done = 0; done < total;) {
        const int n = total - done < 256 ? total - done : 256;
        for (int i = 0; i < n; i++, done++) {
            // Fade each note in and out to avoid clicks.
            const int edge = SAMPLE_RATE / 200;
            float env = 1.0f;
            if (done < edge) env = (float)done / edge;
            if (total - done < edge) env = (float)(total - done) / edge;
            buf[i] = freq > 0 ? (int16_t)(sinf(phase) * 12000 * env) : 0;
            phase += 2.0f * (float)M_PI * freq / SAMPLE_RATE;
            if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
        }
        esp_codec_dev_write(s->dev, buf, n * sizeof(int16_t));
        if (buttons_is_down(BTN_BOOT)) return false;
    }
    return true;
}

static void draw(int volume, const char *note, const char *status)
{
    display_clear(COLOR_BLACK);
    display_header("Sound");
    display_text_center(40, "Speaker (ES8311)", 1, COLOR_GRAY);
    display_text_center(70, note, 4, COLOR_YELLOW);
    display_text(20, 130, "Volume", 1, COLOR_CYAN);
    display_frame(20, 142, 200, 16, COLOR_GRAY);
    display_rect(22, 144, 196 * volume / 100, 12, COLOR_GREEN);
    display_printf(20, 164, 1, COLOR_WHITE, "%d %%", volume);
    display_text_center(190, status, 1, COLOR_ORANGE);
    display_footer("PWR play   + volume   BOOT back");
    display_flush();
}

void app_sound(void)
{
    speaker_t spk;
    if (!speaker_open(&spk)) {
        draw(0, ":(", "Could not start audio");
        button_event_t e;
        while (!(buttons_get_event(&e, portMAX_DELAY) && e.type == BTN_EVENT_CLICK && e.button == BTN_BOOT)) {
        }
        return;
    }

    static const int volumes[] = {40, 60, 75, 90};
    int vol_idx = 1;
    esp_codec_dev_set_out_vol(spk.dev, volumes[vol_idx]);

    // C major scale and closing notes.
    static const struct { const char *name; float freq; int ms; } melody[] = {
        {"C", 261.63f, 250}, {"D", 293.66f, 250}, {"E", 329.63f, 250}, {"F", 349.23f, 250},
        {"G", 392.00f, 250}, {"A", 440.00f, 250}, {"B", 493.88f, 250}, {"C", 523.25f, 400},
        {"G", 392.00f, 180}, {"E", 329.63f, 180}, {"C", 261.63f, 500},
    };

    draw(volumes[vol_idx], "-", "Press PWR to play a scale");
    for (;;) {
        button_event_t e;
        buttons_get_event(&e, portMAX_DELAY);
        if (e.type != BTN_EVENT_CLICK) continue;
        if (e.button == BTN_BOOT) break;
        if (e.button == BTN_PLUS) {
            vol_idx = (vol_idx + 1) % 4;
            esp_codec_dev_set_out_vol(spk.dev, volumes[vol_idx]);
            draw(volumes[vol_idx], "-", "Volume changed");
            play_tone(&spk, 880.0f, 120);   // Confirmation beep.
            play_tone(&spk, 0, 50);
        }
        if (e.button == BTN_PWR) {
            bool aborted = false;
            for (int i = 0; i < (int)(sizeof(melody) / sizeof(melody[0])) && !aborted; i++) {
                draw(volumes[vol_idx], melody[i].name, "Playing... (BOOT to stop)");
                aborted = !play_tone(&spk, melody[i].freq, melody[i].ms) || !play_tone(&spk, 0, 30);
            }
            play_tone(&spk, 0, 100);  // Flush with silence.
            // After a BOOT abort, the release posts a CLICK and exits the loop.
            draw(volumes[vol_idx], "-", aborted ? "Stopped" : "Playback complete");
        }
    }
    speaker_close(&spk);
}
