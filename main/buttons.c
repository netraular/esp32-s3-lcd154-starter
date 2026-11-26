#include "buttons.h"
#include "board.h"

#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define POLL_MS      10
#define DEBOUNCE_CNT 3      // Accept a change after three matching samples (30 ms).

typedef struct {
    gpio_num_t gpio;
    bool down;              // Debounced state.
    uint8_t same_count;     // Consecutive samples that differ from the stable state.
    int64_t down_since_us;
    bool long_sent;
    bool ignore_until_up;   // Already held at boot (for example, PWR).
} button_state_t;

static button_state_t s_btn[BTN_COUNT] = {
    [BTN_BOOT] = {.gpio = PIN_BTN_BOOT},
    [BTN_PLUS] = {.gpio = PIN_BTN_PLUS},
    [BTN_PWR]  = {.gpio = PIN_BTN_PWR},
};
static QueueHandle_t s_queue;

static void emit(button_id_t b, button_event_type_t t)
{
    const button_event_t e = {.button = b, .type = t};
    xQueueSend(s_queue, &e, 0); // Drop the event if the queue is full.
}

static void buttons_task(void *arg)
{
    for (;;) {
        const int64_t now = esp_timer_get_time();
        for (int i = 0; i < BTN_COUNT; i++) {
            button_state_t *b = &s_btn[i];
            const bool raw_down = gpio_get_level(b->gpio) == 0; // Active low.

            if (raw_down != b->down) {
                if (++b->same_count >= DEBOUNCE_CNT) {
                    b->same_count = 0;
                    b->down = raw_down;
                    if (b->ignore_until_up) {
                        if (!raw_down) b->ignore_until_up = false;
                        continue;
                    }
                    if (raw_down) {
                        b->down_since_us = now;
                        b->long_sent = false;
                        emit(i, BTN_EVENT_DOWN);
                    } else {
                        emit(i, BTN_EVENT_UP);
                        if (!b->long_sent) emit(i, BTN_EVENT_CLICK);
                    }
                }
            } else {
                b->same_count = 0;
            }

            if (b->down && !b->ignore_until_up && !b->long_sent &&
                now - b->down_since_us >= BUTTON_LONG_MS * 1000LL) {
                b->long_sent = true;
                emit(i, BTN_EVENT_LONG);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}

void buttons_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_BTN_BOOT) | (1ULL << PIN_BTN_PLUS) | (1ULL << PIN_BTN_PWR),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));

    // Ignore buttons already held at boot until they are released.
    for (int i = 0; i < BTN_COUNT; i++) {
        s_btn[i].down = gpio_get_level(s_btn[i].gpio) == 0;
        s_btn[i].ignore_until_up = s_btn[i].down;
    }

    s_queue = xQueueCreate(16, sizeof(button_event_t));
    xTaskCreate(buttons_task, "buttons", 2048, NULL, 5, NULL);
}

bool buttons_get_event(button_event_t *event, TickType_t timeout)
{
    return xQueueReceive(s_queue, event, timeout) == pdTRUE;
}

bool buttons_is_down(button_id_t button)
{
    return s_btn[button].down;
}

void buttons_flush(void)
{
    xQueueReset(s_queue);
}

const char *buttons_name(button_id_t button)
{
    static const char *names[BTN_COUNT] = {"BOOT", "+", "PWR"};
    return button < BTN_COUNT ? names[button] : "?";
}
