#include "pressure_math.h"

static uint32_t difference_u32(uint32_t left, uint32_t right)
{
    return (left >= right) ? (left - right) : (right - left);
}

uint32_t pressure_pwm_prescaler(uint32_t timer_clock_hz,
                                uint32_t target_frequency_hz,
                                uint32_t period_counts)
{
    uint32_t divider;

    if ((target_frequency_hz == 0U) || (period_counts == 0U)) {
        return 0U;
    }

    divider = timer_clock_hz / (target_frequency_hz * period_counts);
    return (divider > 0U) ? (divider - 1U) : 0U;
}

uint32_t pressure_pwm_compare(uint16_t duty_permille,
                              uint32_t period_counts)
{
    if (duty_permille > 1000U) {
        duty_permille = 1000U;
    }
    return (period_counts * (uint32_t)duty_permille) / 1000U;
}

float pressure_adc_pin_voltage(uint16_t adc_raw,
                               float reference_voltage,
                               uint16_t adc_full_scale)
{
    if (adc_full_scale == 0U) {
        return 0.0f;
    }
    return reference_voltage * (float)adc_raw / (float)adc_full_scale;
}

float pressure_sensor_voltage(float adc_pin_voltage, float divider_gain)
{
    return adc_pin_voltage * divider_gain;
}

float pressure_mpa_from_voltage(float sensor_voltage,
                                float sensor_min_voltage,
                                float sensor_max_voltage,
                                float full_scale_mpa)
{
    float normalized;

    if (sensor_max_voltage <= sensor_min_voltage) {
        return 0.0f;
    }

    normalized = (sensor_voltage - sensor_min_voltage) /
                 (sensor_max_voltage - sensor_min_voltage);
    if (normalized < 0.0f) {
        normalized = 0.0f;
    } else if (normalized > 1.0f) {
        normalized = 1.0f;
    }
    return normalized * full_scale_mpa;
}

bool pressure_capture_measurement(uint32_t period_ticks,
                                  uint32_t high_ticks,
                                  uint32_t capture_tick_hz,
                                  uint32_t *frequency_hz,
                                  uint16_t *duty_permille)
{
    if ((period_ticks == 0U) || (high_ticks > period_ticks) ||
        (capture_tick_hz == 0U) || (frequency_hz == 0) ||
        (duty_permille == 0)) {
        return false;
    }

    *frequency_hz = capture_tick_hz / period_ticks;
    *duty_permille = (uint16_t)((high_ticks * 1000U) / period_ticks);
    return true;
}

bool pressure_feedback_matches(uint32_t commanded_frequency_hz,
                               uint16_t commanded_duty_permille,
                               uint32_t measured_frequency_hz,
                               uint16_t measured_duty_permille,
                               uint32_t frequency_tolerance_hz,
                               uint16_t duty_tolerance_permille)
{
    return (difference_u32(commanded_frequency_hz, measured_frequency_hz) <=
            frequency_tolerance_hz) &&
           (difference_u32((uint32_t)commanded_duty_permille,
                           (uint32_t)measured_duty_permille) <=
            (uint32_t)duty_tolerance_permille);
}
