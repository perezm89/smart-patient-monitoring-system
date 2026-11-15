#include "ppg_processing.h"
#include "algorithm.h" /* Maxim/DFRobot 25-Hz algorithm; see setup_maxim.py */
#include <stddef.h>

#define WINDOW_SIZE 100U
#define INPUTS_PER_OUTPUT 4U
#define OUTPUTS_PER_REPORT 25U
#define MAX_RAW_ADC 0x3FFFFUL

static uint32_t red_window[WINDOW_SIZE];
static uint32_t ir_window[WINDOW_SIZE];
static uint32_t red_ordered[WINDOW_SIZE];
static uint32_t ir_ordered[WINDOW_SIZE];

static uint32_t red_sum;
static uint32_t ir_sum;
static uint8_t input_count;
static uint32_t output_count;
static uint32_t stored_count;
static uint32_t next_index;

void ppg_processing_init(void)
{
    red_sum = 0;
    ir_sum = 0;
    input_count = 0;
    output_count = 0;
    stored_count = 0;
    next_index = 0;
}

bool ppg_processing_add_sample(uint32_t red, uint32_t ir, ppg_result_t *result)
{
    if (result == NULL || red > MAX_RAW_ADC || ir > MAX_RAW_ADC) {
        return false;
    }

    /* 100 Hz -> 25 Hz, including an elementary anti-aliasing average. */
    red_sum += red;
    ir_sum += ir;
    input_count++;
    if (input_count != INPUTS_PER_OUTPUT) {
        return false;
    }

    red_window[next_index] = red_sum / INPUTS_PER_OUTPUT;
    ir_window[next_index] = ir_sum / INPUTS_PER_OUTPUT;
    red_sum = 0;
    ir_sum = 0;
    input_count = 0;
    next_index = (next_index + 1U) % WINDOW_SIZE;
    output_count++;
    if (stored_count < WINDOW_SIZE) {
        stored_count++;
    }

    if (stored_count < WINDOW_SIZE ||
        ((output_count - WINDOW_SIZE) % OUTPUTS_PER_REPORT) != 0) {
        return false;
    }

    /* Order the circular buffer oldest -> newest for Maxim's algorithm. */
    uint32_t ir_min = UINT32_MAX;
    uint32_t ir_max = 0;
    for (uint32_t i = 0; i < WINDOW_SIZE; i++) {
        uint32_t src = (next_index + i) % WINDOW_SIZE;
        red_ordered[i] = red_window[src];
        ir_ordered[i] = ir_window[src];
        if (ir_ordered[i] < ir_min) ir_min = ir_ordered[i];
        if (ir_ordered[i] > ir_max) ir_max = ir_ordered[i];
    }

    int8_t hr_valid = 0;
    int8_t spo2_valid = 0;
    int32_t bpm = -999;
    int32_t spo2 = -999;
    maxim_heart_rate_and_oxygen_saturation(
        ir_ordered, (int32_t)WINDOW_SIZE, red_ordered,
        &spo2, &spo2_valid, &bpm, &hr_valid);

    result->heart_rate_bpm = bpm;
    result->spo2_percent = spo2;
    result->ir_peak_to_peak = ir_max - ir_min;
    /* The original algorithm's flags are permissive; these are basic
     * plausibility limits, not clinical signal-quality certification. */
    result->heart_rate_valid = (hr_valid == 1 && bpm >= 40 && bpm <= 180);
    result->spo2_valid = (spo2_valid == 1 && spo2 >= 85 && spo2 <= 100);
    return true;
}
