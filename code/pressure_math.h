#ifndef PRESSURE_MATH_H
#define PRESSURE_MATH_H

#include <stdbool.h>
#include <stdint.h>

uint32_t pressure_pwm_prescaler(uint32_t timer_clock_hz,
                                uint32_t target_frequency_hz,
                                uint32_t period_counts);
uint32_t pressure_pwm_compare(uint16_t duty_permille,
                              uint32_t period_counts);

float pressure_adc_pin_voltage(uint16_t adc_raw,
                               float reference_voltage,
                               uint16_t adc_full_scale);
float pressure_sensor_voltage(float adc_pin_voltage, float divider_gain);
float pressure_mpa_from_voltage(float sensor_voltage,
                                float sensor_min_voltage,
                                float sensor_max_voltage,
                                float full_scale_mpa);

bool pressure_capture_measurement(uint32_t period_ticks,
                                  uint32_t high_ticks,
                                  uint32_t capture_tick_hz,
                                  uint32_t *frequency_hz,
                                  uint16_t *duty_permille);

bool pressure_feedback_matches(uint32_t commanded_frequency_hz,
                               uint16_t commanded_duty_permille,
                               uint32_t measured_frequency_hz,
                               uint16_t measured_duty_permille,
                               uint32_t frequency_tolerance_hz,
                               uint16_t duty_tolerance_permille);

#endif
