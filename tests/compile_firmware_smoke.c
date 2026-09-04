#include <assert.h>
#include <stdint.h>

#include "input_keys.h"
#include "main.h"
#include "pressure_tester.h"

GPIO_TypeDef test_gpioa;
GPIO_TypeDef test_gpiob;
GPIO_TypeDef test_gpioc;
GPIO_TypeDef test_gpiod;
TIM_TypeDef test_tim2;
TIM_TypeDef test_tim3;
TIM_HandleTypeDef htim2 = {&test_tim2, HAL_TIM_ACTIVE_CHANNEL_1};
TIM_HandleTypeDef htim3 = {&test_tim3, 0U};
ADC_HandleTypeDef hadc1 = {1U};
ADC_HandleTypeDef hadc2 = {2U};

static uint32_t fake_tick;
static uint32_t lcd_write_count;

uint32_t HAL_GetTick(void)
{
    return fake_tick++;
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    (void)port;
    (void)pin;
    return GPIO_PIN_SET;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    (void)port;
    (void)pin;
    (void)state;
}

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc,
                                            uint32_t timeout)
{
    (void)hadc;
    (void)timeout;
    return HAL_OK;
}

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc)
{
    return (hadc->id == 1U) ? 2482U : 2048U;
}

HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return HAL_OK;
}

uint32_t HAL_TIM_ReadCapturedValue(TIM_HandleTypeDef *htim, uint32_t channel)
{
    (void)htim;
    return (channel == TIM_CHANNEL_1) ? 250U : 125U;
}

uint32_t __get_PRIMASK(void)
{
    return 0U;
}

void __disable_irq(void)
{
}

void __enable_irq(void)
{
}

void LCD_DisplayStringLine(uint8_t line, uint8_t *text)
{
    (void)line;
    (void)text;
    ++lcd_write_count;
}

int main(void)
{
    uint32_t index;

    pressure_tester_init();
    input_keys_init();
    pressure_tester_capture_isr(250U, 125U, HAL_GetTick());

    for (index = 0U; index < 1000U; ++index) {
        input_keys_poll();
        pressure_tester_poll();
    }

    pressure_tester_toggle_frequency();
    pressure_tester_start_or_abort_auto();
    for (index = 0U; index < 10000U; ++index) {
        pressure_tester_capture_isr(125U, 62U, HAL_GetTick());
        pressure_tester_poll();
    }

    assert(lcd_write_count > 0U);
    return 0;
}
