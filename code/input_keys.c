#include "input_keys.h"

#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "pressure_tester.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState stable_state;
    GPIO_PinState sampled_state;
    uint32_t changed_at_ms;
} DebouncedKey;

static DebouncedKey keys[4];
static bool initialized;

static void dispatch_press(uint8_t index)
{
    switch (index) {
    case 0U:
        pressure_tester_next_page();
        break;
    case 1U:
        if (!pressure_tester_auto_running()) {
            pressure_tester_toggle_frequency();
        }
        break;
    case 2U:
        pressure_tester_start_or_abort_auto();
        break;
    case 3U:
        if (!pressure_tester_auto_running()) {
            pressure_tester_reset_results();
        }
        break;
    default:
        break;
    }
}

void input_keys_init(void)
{
    uint8_t index;

    keys[0].port = GPIOB;
    keys[0].pin = GPIO_PIN_0;
    keys[1].port = GPIOB;
    keys[1].pin = GPIO_PIN_1;
    keys[2].port = GPIOB;
    keys[2].pin = GPIO_PIN_2;
    keys[3].port = GPIOA;
    keys[3].pin = GPIO_PIN_0;

    for (index = 0U; index < 4U; ++index) {
        keys[index].stable_state = HAL_GPIO_ReadPin(keys[index].port,
                                                    keys[index].pin);
        keys[index].sampled_state = keys[index].stable_state;
        keys[index].changed_at_ms = HAL_GetTick();
    }
    initialized = true;
}

void input_keys_poll(void)
{
    uint8_t index;
    uint32_t now_ms = HAL_GetTick();

    if (!initialized) {
        input_keys_init();
    }

    for (index = 0U; index < 4U; ++index) {
        GPIO_PinState sample = HAL_GPIO_ReadPin(keys[index].port,
                                               keys[index].pin);
        if (sample != keys[index].sampled_state) {
            keys[index].sampled_state = sample;
            keys[index].changed_at_ms = now_ms;
        }

        if ((sample != keys[index].stable_state) &&
            ((now_ms - keys[index].changed_at_ms) >= 20U)) {
            keys[index].stable_state = sample;
            if (sample == GPIO_PIN_RESET) {
                dispatch_press(index);
            }
        }
    }
}
