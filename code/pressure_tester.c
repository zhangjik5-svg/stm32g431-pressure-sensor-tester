#include "pressure_tester.h"

#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "app_config.h"
#include "lcd.h"
#include "main.h"
#include "pressure_math.h"
#include "tim.h"

#define AUTO_POINT_COUNT (6U)

typedef struct {
    uint32_t frequency_hz;
    uint16_t duty_permille;
} AutoPoint;

static const AutoPoint auto_plan[AUTO_POINT_COUNT] = {
    {PWM_FREQUENCY_LOW_HZ, 300U},
    {PWM_FREQUENCY_LOW_HZ, 500U},
    {PWM_FREQUENCY_LOW_HZ, 700U},
    {PWM_FREQUENCY_HIGH_HZ, 300U},
    {PWM_FREQUENCY_HIGH_HZ, 500U},
    {PWM_FREQUENCY_HIGH_HZ, 700U}
};

static volatile uint32_t capture_period_ticks;
static volatile uint32_t capture_high_ticks;
static volatile uint32_t capture_timestamp_ms;
static volatile uint32_t capture_sequence;

static uint32_t consumed_capture_sequence;
static uint32_t feedback_frequency_hz;
static uint16_t feedback_duty_permille;
static bool feedback_valid;

static uint32_t command_frequency_hz = PWM_FREQUENCY_LOW_HZ;
static uint16_t command_duty_permille = 500U;
static float adc_pin_voltage;
static float sensor_voltage;
static float pressure_mpa;

static uint8_t display_page;
static PressureAutoState auto_state = PRESSURE_AUTO_IDLE;
static PressureTestResult results[AUTO_POINT_COUNT];
static uint8_t auto_point_index;
static uint8_t auto_sample_count;
static float auto_pressure_sum;
static uint32_t auto_state_started_ms;
static uint32_t last_auto_sample_ms;
static uint32_t last_adc_sample_ms;
static uint32_t last_lcd_refresh_ms;
static uint8_t passed_points;
static uint8_t failed_points;

static uint16_t read_adc(ADC_HandleTypeDef *hadc)
{
    uint16_t value = 0U;

    if (HAL_ADC_Start(hadc) != HAL_OK) {
        return value;
    }
    if (HAL_ADC_PollForConversion(hadc, 5U) == HAL_OK) {
        value = (uint16_t)HAL_ADC_GetValue(hadc);
    }
    (void)HAL_ADC_Stop(hadc);
    return value;
}

static void apply_pwm(uint32_t frequency_hz, uint16_t duty_permille)
{
    uint32_t prescaler;
    uint32_t compare;

    if (duty_permille < PWM_DUTY_MIN_PERMILLE) {
        duty_permille = PWM_DUTY_MIN_PERMILLE;
    } else if (duty_permille > PWM_DUTY_MAX_PERMILLE) {
        duty_permille = PWM_DUTY_MAX_PERMILLE;
    }

    prescaler = pressure_pwm_prescaler(PWM_TIMER_CLOCK_HZ,
                                       frequency_hz,
                                       PWM_PERIOD_COUNTS);
    compare = pressure_pwm_compare(duty_permille, PWM_PERIOD_COUNTS);

    __HAL_TIM_SET_AUTORELOAD(&htim3, PWM_PERIOD_COUNTS - 1U);
    __HAL_TIM_SET_PRESCALER(&htim3, prescaler);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, compare);
    htim3.Instance->EGR = TIM_EGR_UG;

    command_frequency_hz = frequency_hz;
    command_duty_permille = duty_permille;
}

static void sample_analog_inputs(bool update_manual_duty)
{
    uint16_t sensor_raw = read_adc(&hadc1);

    adc_pin_voltage = pressure_adc_pin_voltage(sensor_raw,
                                               ADC_REFERENCE_VOLTAGE,
                                               ADC_FULL_SCALE_COUNTS);
    sensor_voltage = pressure_sensor_voltage(adc_pin_voltage,
                                             PRESSURE_DIVIDER_GAIN);
    pressure_mpa = pressure_mpa_from_voltage(sensor_voltage,
                                             PRESSURE_SENSOR_MIN_VOLTAGE,
                                             PRESSURE_SENSOR_MAX_VOLTAGE,
                                             PRESSURE_FULL_SCALE_MPA);

    if (update_manual_duty) {
        uint16_t adjust_raw = read_adc(&hadc2);
        uint16_t span = PWM_DUTY_MAX_PERMILLE - PWM_DUTY_MIN_PERMILLE;
        uint16_t duty = PWM_DUTY_MIN_PERMILLE +
                        (uint16_t)(((uint32_t)adjust_raw * span) /
                                   ADC_FULL_SCALE_COUNTS);
        apply_pwm(command_frequency_hz, duty);
    }
}

static void update_capture(uint32_t now_ms)
{
    uint32_t sequence;
    uint32_t period;
    uint32_t high;
    uint32_t timestamp;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    sequence = capture_sequence;
    period = capture_period_ticks;
    high = capture_high_ticks;
    timestamp = capture_timestamp_ms;
    if (primask == 0U) {
        __enable_irq();
    }

    if (sequence != consumed_capture_sequence) {
        consumed_capture_sequence = sequence;
        feedback_valid = pressure_capture_measurement(period,
                                                      high,
                                                      CAPTURE_TIMER_TICK_HZ,
                                                      &feedback_frequency_hz,
                                                      &feedback_duty_permille);
    }

    if ((now_ms - timestamp) > CAPTURE_TIMEOUT_MS) {
        feedback_valid = false;
    }
}

static void configure_auto_point(uint8_t index, uint32_t now_ms)
{
    apply_pwm(auto_plan[index].frequency_hz, auto_plan[index].duty_permille);
    auto_sample_count = 0U;
    auto_pressure_sum = 0.0f;
    auto_state = PRESSURE_AUTO_SETTLING;
    auto_state_started_ms = now_ms;
}

static void finish_auto_point(uint32_t now_ms)
{
    PressureTestResult *result = &results[auto_point_index];
    bool electrical_range_ok = (sensor_voltage >= 0.40f) &&
                               (sensor_voltage <= 4.60f);
    bool feedback_ok = feedback_valid &&
        pressure_feedback_matches(command_frequency_hz,
                                  command_duty_permille,
                                  feedback_frequency_hz,
                                  feedback_duty_permille,
                                  FREQUENCY_TOLERANCE_HZ,
                                  DUTY_TOLERANCE_PERMILLE);

    result->commanded_frequency_hz = command_frequency_hz;
    result->commanded_duty_permille = command_duty_permille;
    result->measured_frequency_hz = feedback_frequency_hz;
    result->measured_duty_permille = feedback_duty_permille;
    result->pressure_mpa = auto_pressure_sum / (float)AUTO_SAMPLE_COUNT;
    result->passed = electrical_range_ok && feedback_ok;
    result->completed = true;

    if (result->passed) {
        ++passed_points;
    } else {
        ++failed_points;
    }

    ++auto_point_index;
    if (auto_point_index >= AUTO_POINT_COUNT) {
        auto_state = PRESSURE_AUTO_COMPLETE;
        auto_state_started_ms = now_ms;
        apply_pwm(PWM_FREQUENCY_LOW_HZ, 500U);
    } else {
        configure_auto_point(auto_point_index, now_ms);
    }
}

static void run_auto_state_machine(uint32_t now_ms)
{
    if (auto_state == PRESSURE_AUTO_SETTLING) {
        if ((now_ms - auto_state_started_ms) >= AUTO_SETTLE_TIME_MS) {
            auto_state = PRESSURE_AUTO_SAMPLING;
            auto_sample_count = 0U;
            auto_pressure_sum = 0.0f;
            last_auto_sample_ms = now_ms;
        }
    } else if (auto_state == PRESSURE_AUTO_SAMPLING) {
        if ((now_ms - last_auto_sample_ms) >= AUTO_SAMPLE_PERIOD_MS) {
            last_auto_sample_ms = now_ms;
            sample_analog_inputs(false);
            auto_pressure_sum += pressure_mpa;
            ++auto_sample_count;
            if (auto_sample_count >= AUTO_SAMPLE_COUNT) {
                finish_auto_point(now_ms);
            }
        }
    }
}

static void led_mask_write(uint8_t on_mask)
{
    uint16_t all_leds = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
                        GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    uint8_t index;

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, all_leds, GPIO_PIN_SET);
    for (index = 0U; index < 8U; ++index) {
        if ((on_mask & (1U << index)) != 0U) {
            HAL_GPIO_WritePin(GPIOC, (uint16_t)(GPIO_PIN_8 << index),
                              GPIO_PIN_RESET);
        }
    }
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
}

static void refresh_leds(void)
{
    uint8_t mask = 0x01U;

    if (pressure_tester_auto_running()) {
        mask |= 0x02U;
    }
    if (!feedback_valid || (failed_points > 0U)) {
        mask |= 0x04U;
    }
    if ((auto_state == PRESSURE_AUTO_COMPLETE) && (failed_points == 0U)) {
        mask |= 0x08U;
    }
    led_mask_write(mask);
}

static void lcd_line(uint8_t line, const char *text)
{
    char padded[21];
    size_t length = strlen(text);

    if (length > 20U) {
        length = 20U;
    }
    memset(padded, ' ', 20U);
    memcpy(padded, text, length);
    padded[20] = '\0';
    LCD_DisplayStringLine(line, (uint8_t *)padded);
}

static void show_live_page(void)
{
    char text[48];
    uint16_t pressure_milli = (uint16_t)(pressure_mpa * 1000.0f + 0.5f);
    uint16_t voltage_centi = (uint16_t)(sensor_voltage * 100.0f + 0.5f);

    lcd_line(Line0, " PRESSURE TESTER");
    snprintf(text, sizeof(text), "MODE:MAN  F:%lu", (unsigned long)command_frequency_hz);
    lcd_line(Line2, text);
    snprintf(text, sizeof(text), "PWM:%u.%u%%",
            command_duty_permille / 10U,
            command_duty_permille % 10U);
    lcd_line(Line3, text);
    snprintf(text, sizeof(text), "P:%u.%03u MPa",
            pressure_milli / 1000U,
            pressure_milli % 1000U);
    lcd_line(Line4, text);
    snprintf(text, sizeof(text), "SENSOR:%u.%02u V",
            voltage_centi / 100U,
            voltage_centi % 100U);
    lcd_line(Line5, text);
    if (feedback_valid) {
        snprintf(text, sizeof(text), "FB:%luHz %u.%u%%",
                (unsigned long)feedback_frequency_hz,
                feedback_duty_permille / 10U,
                feedback_duty_permille % 10U);
    } else {
        snprintf(text, sizeof(text), "FB:NO SIGNAL");
    }
    lcd_line(Line6, text);
    lcd_line(Line8, "B2:FREQ B3:AUTO");
    lcd_line(Line9, "B1:PAGE B4:RESET");
}

static const char *auto_state_name(void)
{
    switch (auto_state) {
    case PRESSURE_AUTO_SETTLING:
        return "SETTLING";
    case PRESSURE_AUTO_SAMPLING:
        return "SAMPLING";
    case PRESSURE_AUTO_COMPLETE:
        return "COMPLETE";
    case PRESSURE_AUTO_ABORTED:
        return "ABORTED";
    default:
        return "IDLE";
    }
}

static void show_test_page(void)
{
    char text[48];
    uint8_t shown_point = auto_point_index;

    if (shown_point >= AUTO_POINT_COUNT) {
        shown_point = AUTO_POINT_COUNT;
    } else {
        ++shown_point;
    }

    lcd_line(Line0, "  AUTO TEST");
    snprintf(text, sizeof(text), "STATE:%s", auto_state_name());
    lcd_line(Line2, text);
    snprintf(text, sizeof(text), "POINT:%u/%u", shown_point, AUTO_POINT_COUNT);
    lcd_line(Line3, text);
    snprintf(text, sizeof(text), "CMD:%luHz %u.%u%%",
            (unsigned long)command_frequency_hz,
            command_duty_permille / 10U,
            command_duty_permille % 10U);
    lcd_line(Line4, text);
    snprintf(text, sizeof(text), "PASS:%u FAIL:%u", passed_points, failed_points);
    lcd_line(Line5, text);
    if ((auto_state == PRESSURE_AUTO_COMPLETE) && (failed_points == 0U)) {
        lcd_line(Line7, "RESULT: PASS");
    } else if (auto_state == PRESSURE_AUTO_COMPLETE) {
        lcd_line(Line7, "RESULT: CHECK FAIL");
    } else {
        lcd_line(Line7, "B3:START / ABORT");
    }
    lcd_line(Line9, "PA7->PA15 LOOPBACK");
}

static void show_wiring_page(void)
{
    lcd_line(Line0, "  WIRING / CAL");
    lcd_line(Line2, "PA7 : PWM OUTPUT");
    lcd_line(Line3, "PA15: PWM FEEDBACK");
    lcd_line(Line4, "PB12: PRESSURE ADC");
    lcd_line(Line5, "PB15: DUTY KNOB");
    lcd_line(Line7, "SENSOR:0.5-4.5V");
    lcd_line(Line8, "RATIO:1.5  FS:1.6");
    lcd_line(Line9, "ADC PIN MAX:3.3V");
}

static void refresh_lcd(void)
{
    if (display_page == 0U) {
        show_live_page();
    } else if (display_page == 1U) {
        show_test_page();
    } else {
        show_wiring_page();
    }
}

void pressure_tester_init(void)
{
    uint32_t now_ms = HAL_GetTick();

    memset(results, 0, sizeof(results));
    apply_pwm(PWM_FREQUENCY_LOW_HZ, 500U);
    sample_analog_inputs(false);
    auto_state_started_ms = now_ms;
    last_adc_sample_ms = now_ms;
    last_lcd_refresh_ms = now_ms - LCD_REFRESH_PERIOD_MS;
}

void pressure_tester_poll(void)
{
    uint32_t now_ms = HAL_GetTick();

    update_capture(now_ms);
    if (!pressure_tester_auto_running() &&
        ((now_ms - last_adc_sample_ms) >= ADC_SAMPLE_PERIOD_MS)) {
        last_adc_sample_ms = now_ms;
        sample_analog_inputs(true);
    }

    run_auto_state_machine(now_ms);
    refresh_leds();

    if ((now_ms - last_lcd_refresh_ms) >= LCD_REFRESH_PERIOD_MS) {
        last_lcd_refresh_ms = now_ms;
        refresh_lcd();
    }
}

void pressure_tester_next_page(void)
{
    display_page = (uint8_t)((display_page + 1U) % 3U);
    last_lcd_refresh_ms = 0U;
}

void pressure_tester_toggle_frequency(void)
{
    uint32_t next = (command_frequency_hz == PWM_FREQUENCY_LOW_HZ) ?
                    PWM_FREQUENCY_HIGH_HZ : PWM_FREQUENCY_LOW_HZ;
    apply_pwm(next, command_duty_permille);
}

void pressure_tester_start_or_abort_auto(void)
{
    uint32_t now_ms = HAL_GetTick();

    if (pressure_tester_auto_running()) {
        auto_state = PRESSURE_AUTO_ABORTED;
        apply_pwm(PWM_FREQUENCY_LOW_HZ, 500U);
        return;
    }

    pressure_tester_reset_results();
    auto_point_index = 0U;
    display_page = 1U;
    configure_auto_point(auto_point_index, now_ms);
}

void pressure_tester_reset_results(void)
{
    memset(results, 0, sizeof(results));
    passed_points = 0U;
    failed_points = 0U;
    auto_point_index = 0U;
    auto_state = PRESSURE_AUTO_IDLE;
}

bool pressure_tester_auto_running(void)
{
    return (auto_state == PRESSURE_AUTO_SETTLING) ||
           (auto_state == PRESSURE_AUTO_SAMPLING);
}

void pressure_tester_capture_isr(uint32_t period_ticks,
                                 uint32_t high_ticks,
                                 uint32_t tick_ms)
{
    capture_period_ticks = period_ticks;
    capture_high_ticks = high_ticks;
    capture_timestamp_ms = tick_ms;
    ++capture_sequence;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if ((htim->Instance == TIM2) &&
        (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)) {
        uint32_t period = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        uint32_t high = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        pressure_tester_capture_isr(period, high, HAL_GetTick());
    }
}
