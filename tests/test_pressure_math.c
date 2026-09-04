#include <assert.h>
#include <stdio.h>

#include "pressure_math.h"

static void assert_close(float actual, float expected, float tolerance)
{
    float difference = actual - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    assert(difference <= tolerance);
}

int main(void)
{
    uint32_t frequency_hz = 0U;
    uint16_t duty_permille = 0U;
    float adc_pin;
    float sensor_voltage;
    float pressure;

    assert(pressure_pwm_prescaler(80000000U, 4000U, 1000U) == 19U);
    assert(pressure_pwm_prescaler(80000000U, 8000U, 1000U) == 9U);
    assert(pressure_pwm_compare(500U, 1000U) == 500U);
    assert(pressure_pwm_compare(300U, 1000U) == 300U);

    adc_pin = pressure_adc_pin_voltage(2482U, 3.3f, 4095U);
    sensor_voltage = pressure_sensor_voltage(adc_pin, 1.5f);
    pressure = pressure_mpa_from_voltage(sensor_voltage,
                                         0.5f, 4.5f, 1.6f);
    assert_close(adc_pin, 2.0f, 0.01f);
    assert_close(sensor_voltage, 3.0f, 0.02f);
    assert_close(pressure, 1.0f, 0.02f);

    assert(pressure_capture_measurement(250U, 125U, 1000000U,
                                        &frequency_hz,
                                        &duty_permille));
    assert(frequency_hz == 4000U);
    assert(duty_permille == 500U);

    assert(pressure_capture_measurement(125U, 37U, 1000000U,
                                        &frequency_hz,
                                        &duty_permille));
    assert(frequency_hz == 8000U);
    assert(duty_permille == 296U);

    assert(pressure_feedback_matches(8000U, 300U, 7990U, 296U,
                                     160U, 30U));
    assert(!pressure_feedback_matches(8000U, 300U, 7600U, 296U,
                                      160U, 30U));
    assert(!pressure_feedback_matches(8000U, 300U, 7990U, 250U,
                                      160U, 30U));

    puts("pressure_math tests passed");
    return 0;
}
