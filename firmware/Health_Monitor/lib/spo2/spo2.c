#include "spo2.h"

#include <math.h>
#include <stddef.h>

/*
 * At 100 samples per second, 500 samples represents five seconds.
 */
#define SPO2_MIN_SAMPLE_COUNT       500U

/*
 * Reject measurements when the optical DC signal is too small.
 */
#define SPO2_MIN_DC_LEVEL           10000.0f

/*
 * Reject measurements with essentially no pulsatile component.
 */
#define SPO2_MIN_AC_RMS             5.0f

/*
 * Broad ratio limits used to reject unstable measurements.
 */
#define SPO2_MIN_VALID_RATIO        0.20f
#define SPO2_MAX_VALID_RATIO        1.40f

#define SPO2_MIN_PERCENT            70.0f
#define SPO2_MAX_PERCENT            100.0f

/**
 * @brief Calculate the RMS value of a floating-point signal.
 *
 * The samples passed to this function are expected to already have their
 * DC component removed.
 */
static float spo2_calculate_rms(
    const float *samples,
    size_t sample_count
)
{
    double squared_sum = 0.0;

    for(size_t i = 0; i < sample_count; i++)
    {
        double sample = (double)samples[i];
        squared_sum += sample * sample;
    }

    return (float)sqrt(
        squared_sum / (double)sample_count
    );
}

/**
 * @brief Limit a value to a specified range.
 */
static float spo2_clamp(
    float value,
    float minimum,
    float maximum
)
{
    if(value < minimum)
    {
        return minimum;
    }

    if(value > maximum)
    {
        return maximum;
    }

    return value;
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
    if(red_ac_samples == NULL ||
       ir_ac_samples == NULL ||
       result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if(sample_count < SPO2_MIN_SAMPLE_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Clear the output structure before beginning the calculation.
     */
    result->spo2_percent = 0.0f;
    result->raw_spo2_percent = 0.0f;
    result->ratio = 0.0f;

    result->red_dc = red_dc;
    result->ir_dc = ir_dc;

    result->red_ac_rms = 0.0f;
    result->ir_ac_rms = 0.0f;

    /*
     * The DC values represent the average received optical intensity.
     */
    if(red_dc < SPO2_MIN_DC_LEVEL ||
       ir_dc < SPO2_MIN_DC_LEVEL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * Calculate the pulsatile RMS amplitude of each filtered signal.
     */
    float red_ac_rms = spo2_calculate_rms(
        red_ac_samples,
        sample_count
    );

    float ir_ac_rms = spo2_calculate_rms(
        ir_ac_samples,
        sample_count
    );

    result->red_ac_rms = red_ac_rms;
    result->ir_ac_rms = ir_ac_rms;

    if(red_ac_rms < SPO2_MIN_AC_RMS ||
       ir_ac_rms < SPO2_MIN_AC_RMS)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * Calculate the normalized AC amplitudes.
     */
    float red_normalized = red_ac_rms / red_dc;
    float ir_normalized = ir_ac_rms / ir_dc;

    if(red_normalized <= 0.0f ||
       ir_normalized <= 0.0f)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * Ratio of ratios:
     *
     *       Red AC / Red DC
     * R = -------------------
     *        IR AC / IR DC
     */
    float ratio = red_normalized / ir_normalized;

    result->ratio = ratio;

    if(!isfinite(ratio))
    {
        return ESP_ERR_INVALID_STATE;
    }

    if(ratio < SPO2_MIN_VALID_RATIO ||
       ratio > SPO2_MAX_VALID_RATIO)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * Prototype polynomial conversion from ratio to SpO2.
     *
     * This is suitable for initial project testing but must eventually
     * be calibrated for the final wearable optical design.
     */
    float raw_spo2 =
        (-45.060f * ratio * ratio) +
        (30.354f * ratio) +
        94.845f;

    if(!isfinite(raw_spo2))
    {
        return ESP_ERR_INVALID_STATE;
    }

    result->raw_spo2_percent = raw_spo2;

    result->spo2_percent = spo2_clamp(
        raw_spo2,
        SPO2_MIN_PERCENT,
        SPO2_MAX_PERCENT
    );

    return ESP_OK;
}