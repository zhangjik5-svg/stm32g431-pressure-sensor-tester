#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/* Timer clocks are 80 MHz with the current CubeMX clock tree. */
#define PWM_TIMER_CLOCK_HZ            (80000000UL)
#define PWM_PERIOD_COUNTS             (1000U)
#define PWM_FREQUENCY_LOW_HZ          (4000UL)
#define PWM_FREQUENCY_HIGH_HZ         (8000UL)
#define PWM_DUTY_MIN_PERMILLE         (100U)
#define PWM_DUTY_MAX_PERMILLE         (900U)

/* TIM2 is prescaled to a 1 MHz capture clock. */
#define CAPTURE_TIMER_TICK_HZ         (1000000UL)
#define CAPTURE_TIMEOUT_MS            (250UL)
#define FREQUENCY_TOLERANCE_HZ        (160UL)
#define DUTY_TOLERANCE_PERMILLE       (30U)

/*
 * Pressure input: a common 0.5-4.5 V, 0-1.6 MPa sensor is divided by 1.5
 * before PB12/ADC1. A board potentiometer can be used instead during bench
 * testing. Never apply more than 3.3 V to an STM32 ADC pin.
 */
#define ADC_FULL_SCALE_COUNTS         (4095U)
#define ADC_REFERENCE_VOLTAGE         (3.3f)
#define PRESSURE_DIVIDER_GAIN         (1.5f)
#define PRESSURE_SENSOR_MIN_VOLTAGE   (0.5f)
#define PRESSURE_SENSOR_MAX_VOLTAGE   (4.5f)
#define PRESSURE_FULL_SCALE_MPA       (1.6f)

#define ADC_SAMPLE_PERIOD_MS          (20UL)
#define LCD_REFRESH_PERIOD_MS         (100UL)
#define AUTO_SETTLE_TIME_MS           (800UL)
#define AUTO_SAMPLE_PERIOD_MS         (20UL)
#define AUTO_SAMPLE_COUNT             (16U)

#endif
