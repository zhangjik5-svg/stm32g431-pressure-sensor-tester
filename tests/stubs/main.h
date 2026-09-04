#ifndef TEST_STUB_MAIN_H
#define TEST_STUB_MAIN_H

#include <stdint.h>

typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1
} HAL_StatusTypeDef;

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET = 1
} GPIO_PinState;

typedef struct {
    uint32_t unused;
} GPIO_TypeDef;

typedef struct {
    uint32_t EGR;
} TIM_TypeDef;

typedef struct {
    TIM_TypeDef *Instance;
    uint32_t Channel;
} TIM_HandleTypeDef;

typedef struct {
    uint32_t id;
} ADC_HandleTypeDef;

extern GPIO_TypeDef test_gpioa;
extern GPIO_TypeDef test_gpiob;
extern GPIO_TypeDef test_gpioc;
extern GPIO_TypeDef test_gpiod;
extern TIM_TypeDef test_tim2;
extern TIM_TypeDef test_tim3;

#define GPIOA (&test_gpioa)
#define GPIOB (&test_gpiob)
#define GPIOC (&test_gpioc)
#define GPIOD (&test_gpiod)
#define TIM2  (&test_tim2)
#define TIM3  (&test_tim3)

#define GPIO_PIN_0  ((uint16_t)0x0001U)
#define GPIO_PIN_1  ((uint16_t)0x0002U)
#define GPIO_PIN_2  ((uint16_t)0x0004U)
#define GPIO_PIN_8  ((uint16_t)0x0100U)
#define GPIO_PIN_9  ((uint16_t)0x0200U)
#define GPIO_PIN_10 ((uint16_t)0x0400U)
#define GPIO_PIN_11 ((uint16_t)0x0800U)
#define GPIO_PIN_12 ((uint16_t)0x1000U)
#define GPIO_PIN_13 ((uint16_t)0x2000U)
#define GPIO_PIN_14 ((uint16_t)0x4000U)
#define GPIO_PIN_15 ((uint16_t)0x8000U)

#define TIM_CHANNEL_1            (1U)
#define TIM_CHANNEL_2            (2U)
#define HAL_TIM_ACTIVE_CHANNEL_1 (1U)
#define TIM_EGR_UG                (1U)

#define __HAL_TIM_SET_AUTORELOAD(handle, value) ((void)(handle), (void)(value))
#define __HAL_TIM_SET_PRESCALER(handle, value)  ((void)(handle), (void)(value))
#define __HAL_TIM_SET_COMPARE(handle, channel, value) \
    ((void)(handle), (void)(channel), (void)(value))

uint32_t HAL_GetTick(void);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc,
                                            uint32_t timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc);
uint32_t HAL_TIM_ReadCapturedValue(TIM_HandleTypeDef *htim, uint32_t channel);

uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __enable_irq(void);

#endif
