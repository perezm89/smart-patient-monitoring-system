#ifndef PPG_PROCESSING_H
#define PPG_PROCESSING_H

#include <stdbool.h>
#include <stdint.h>

/* MAX30102 configured for 100 Hz, FIFO sample averaging = 1.
 * Four input pairs are averaged into one 25 Hz pair.
 * A result is produced once every 25 output pairs (1 second)
 * after the first 100 output pairs (4 seconds).
 */
typedef struct {
    int32_t heart_rate_bpm;
    int32_t spo2_percent;
    bool heart_rate_valid;
    bool spo2_valid;
    uint32_t ir_peak_to_peak; /* raw IR range over the four-second window */
} ppg_result_t;

void ppg_processing_init(void);

/* Returns true only when a new result has been calculated.
 * Validity flags should be checked independently.
 * Call only from one task.
 */
bool ppg_processing_add_sample(uint32_t red, uint32_t ir, ppg_result_t *result);

#endif
