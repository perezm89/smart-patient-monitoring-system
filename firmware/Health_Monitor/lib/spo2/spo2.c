#include "spo2.h"

#include <math.h>
#include <stddef.h>

#define SPO2_MIN_SAMPLE_COUNT    500U
#define SPO2_MIN_DC_LEVEL        10000.0f
#define SPO2_MIN_AC_RMS          5.0f
#define SPO2_MIN_VALID_RATIO     0.20f
#define SPO2_MAX_VALID_RATIO     1.40f
#define SPO2_MIN_PERCENT         70.0f
#define SPO2_MAX_PERCENT         100.0f

static float calculate_rms(const float *samples, size_t sample_count)
{
    float squared_sum = 0.0f;

    for(size_t i = 0U; i < sample_count; i++)
    {
        squared_sum += samples[i] * samples[i];
    }

    return sqrtf(squared_sum / (float)sample_count);
}

esp_err_t spo2_calculate(
    const float *red_ac_samples,
    const float *ir_ac_samples,
    size_t sample_count,
    float red_dc,
    float ir_dc,
    spo2_result_t *result
)
{
    if((red_ac_samples == NULL) ||
       (ir_ac_samples == NULL) ||
       (result == NULL) ||
       (sample_count < SPO2_MIN_SAMPLE_COUNT))
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Clear the result in one operation. This replaces seven separate
     * assignments while preserving the existing result structure.
     */
    *result = (spo2_result_t){0};

    if((red_dc < SPO2_MIN_DC_LEVEL) ||
       (ir_dc < SPO2_MIN_DC_LEVEL))
    {
        return ESP_ERR_INVALID_STATE;
    }

    const float red_ac_rms =
        calculate_rms(red_ac_samples, sample_count);

    const float ir_ac_rms =
        calculate_rms(ir_ac_samples, sample_count);

    if((red_ac_rms < SPO2_MIN_AC_RMS) ||
       (ir_ac_rms < SPO2_MIN_AC_RMS))
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * Algebraically equivalent to:
     *
     * (red_ac_rms / red_dc) / (ir_ac_rms / ir_dc)
     *
     * This form uses fewer floating-point divisions.
     */
    const float ratio =
        (red_ac_rms * ir_dc) / (ir_ac_rms * red_dc);

    if(!isfinite(ratio) ||
       (ratio < SPO2_MIN_VALID_RATIO) ||
       (ratio > SPO2_MAX_VALID_RATIO))
    {
        return ESP_ERR_INVALID_STATE;
    }

    const float raw_spo2 =
        ((-45.060f * ratio + 30.354f) * ratio) + 94.845f;

    if(!isfinite(raw_spo2))
    {
        return ESP_ERR_INVALID_STATE;
    }

    float spo2_percent = raw_spo2;

    if(spo2_percent < SPO2_MIN_PERCENT)
    {
        spo2_percent = SPO2_MIN_PERCENT;
    }
    else if(spo2_percent > SPO2_MAX_PERCENT)
    {
        spo2_percent = SPO2_MAX_PERCENT;
    }

    /*
     * Keep all existing result fields populated in case other project files
     * use them for diagnostics or calibration.
     */
    result->spo2_percent = spo2_percent;
    result->raw_spo2_percent = raw_spo2;
    result->ratio = ratio;
    result->red_dc = red_dc;
    result->ir_dc = ir_dc;
    result->red_ac_rms = red_ac_rms;
    result->ir_ac_rms = ir_ac_rms;

    return ESP_OK;
}