#ifndef SPO2_H
#define SPO2_H

#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float spo2_percent;
    float raw_spo2_percent;
    float ratio;

    float red_dc;
    float ir_dc;

    float red_ac_rms;
    float ir_ac_rms;
} spo2_result_t;

/**
 * @brief Estimate SpO2 from filtered red and IR PPG signals.
 *
 * The red and IR AC arrays must already have:
 *
 * 1. Their DC components removed.
 * 2. The same filtering applied to both channels.
 *
 * The red_dc and ir_dc values must be calculated from the corresponding
 * unfiltered raw signals.
 *
 * @param red_ac_samples Filtered red-channel AC samples.
 * @param ir_ac_samples Filtered IR-channel AC samples.
 * @param sample_count Number of samples in each array.
 * @param red_dc Mean of the unfiltered red signal.
 * @param ir_dc Mean of the unfiltered IR signal.
 * @param result Output structure containing SpO2 and diagnostic values.
 *
 * @return
 *      ESP_OK on success.
 *      ESP_ERR_INVALID_ARG for invalid arguments.
 *      ESP_ERR_INVALID_STATE when signal quality is insufficient.
 */
esp_err_t spo2_calculate(
    const float *red_ac_samples,
    const float *ir_ac_samples,
    size_t sample_count,
    float red_dc,
    float ir_dc,
    spo2_result_t *result
);

#ifdef __cplusplus
}
#endif

#endif