#ifndef PRESSURE_TESTER_H
#define PRESSURE_TESTER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PRESSURE_AUTO_IDLE = 0,
    PRESSURE_AUTO_SETTLING,
    PRESSURE_AUTO_SAMPLING,
    PRESSURE_AUTO_COMPLETE,
    PRESSURE_AUTO_ABORTED
} PressureAutoState;

typedef struct {
    uint32_t commanded_frequency_hz;
    uint16_t commanded_duty_permille;
    uint32_t measured_frequency_hz;
    uint16_t measured_duty_permille;
    float pressure_mpa;
    bool passed;
    bool completed;
} PressureTestResult;

void pressure_tester_init(void);
void pressure_tester_poll(void);
void pressure_tester_next_page(void);
void pressure_tester_toggle_frequency(void);
void pressure_tester_start_or_abort_auto(void);
void pressure_tester_reset_results(void);
bool pressure_tester_auto_running(void);

/* Called by the TIM2 input-capture ISR. */
void pressure_tester_capture_isr(uint32_t period_ticks,
                                 uint32_t high_ticks,
                                 uint32_t tick_ms);

#endif
