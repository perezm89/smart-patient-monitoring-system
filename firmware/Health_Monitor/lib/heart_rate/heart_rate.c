#include "heart_rate.h"

#include "signal_processing.h"

#include <stddef.h>
#include <stdint.h>

size_t heart_rate_minimum_peak_distance(float sample_rate_hz)
{
    if(sample_rate_hz <= 0.0f)
    {
        return 0U;
    }

    /*
     * Round upward without calling ceilf(), avoiding an unnecessary math
     * library operation.
     */
    const float distance =
        (60.0f * sample_rate_hz) / HEART_RATE_BPM_MAX;

    const size_t whole_distance = (size_t)distance;

    return ((float)whole_distance < distance) ?
           (whole_distance + 1U) :
           whole_distance;
}

float heart_rate_calculate_threshold(
    const float *samples,
    size_t sample_count
)
{
    if((samples == NULL) || (sample_count == 0U))
    {
        return 0.0f;
    }

    return signal_mean(samples, sample_count) +
           (HEART_RATE_THRESHOLD_FACTOR *
            signal_peak_to_peak(samples, sample_count));
}

size_t heart_rate_find_peaks(
    const float *samples,
    size_t sample_count,
    float threshold,
    size_t minimum_peak_distance,
    size_t *peak_indices,
    size_t max_peaks
)
{
    if((samples == NULL) ||
       (peak_indices == NULL) ||
       (sample_count < 3U) ||
       (max_peaks == 0U))
    {
        return 0U;
    }

    size_t peak_count = 0U;

    for(size_t i = 1U; i < (sample_count - 1U); i++)
    {
        if((samples[i] <= threshold) ||
           (samples[i] <= samples[i - 1U]) ||
           (samples[i] <= samples[i + 1U]))
        {
            continue;
        }

        if(peak_count == 0U)
        {
            peak_indices[peak_count++] = i;
            continue;
        }

        const size_t previous_peak =
            peak_indices[peak_count - 1U];

        if((i - previous_peak) >= minimum_peak_distance)
        {
            if(peak_count >= max_peaks)
            {
                break;
            }

            peak_indices[peak_count++] = i;
        }
        else if(samples[i] > samples[previous_peak])
        {
            peak_indices[peak_count - 1U] = i;
        }
    }

    return peak_count;
}

float heart_rate_calculate(
    const size_t *peak_indices,
    size_t peak_count,
    float sample_rate_hz
)
{
    if((peak_indices == NULL) ||
       (peak_count < 2U) ||
       (sample_rate_hz <= 0.0f))
    {
        return 0.0f;
    }

    /*
     * Interval limits are stored as integer sample counts because every
     * measured interval is already an integer number of samples.
     */
    const size_t minimum_interval =
        heart_rate_minimum_peak_distance(sample_rate_hz);

    const size_t maximum_interval =
        (size_t)((60.0f * sample_rate_hz) /
                 HEART_RATE_BPM_MIN);

    size_t valid_interval_count = 0U;
    size_t interval_sum = 0U;

    for(size_t i = 1U; i < peak_count; i++)
    {
        if(peak_indices[i] <= peak_indices[i - 1U])
        {
            return 0.0f;
        }

        const size_t interval =
            peak_indices[i] - peak_indices[i - 1U];

        if((interval < minimum_interval) ||
           (interval > maximum_interval))
        {
            continue;
        }

        interval_sum += interval;
        valid_interval_count++;
    }

    if(valid_interval_count == 0U)
    {
        return 0.0f;
    }

    /*
     * Equivalent to calculating the average interval first, but requires one
     * floating-point division instead of two.
     */
    return (60.0f * sample_rate_hz *
            (float)valid_interval_count) /
           (float)interval_sum;
}

float heart_rate_process(
    const float *samples,
    size_t sample_count,
    float sample_rate_hz
)
{
    if((samples == NULL) ||
       (sample_count < 3U) ||
       (sample_rate_hz <= 0.0f))
    {
        return 0.0f;
    }

    size_t peak_indices[HEART_RATE_MAX_PEAKS];

    const size_t minimum_peak_distance =
        heart_rate_minimum_peak_distance(sample_rate_hz);

    const float threshold =
        heart_rate_calculate_threshold(samples, sample_count);

    const size_t peak_count = heart_rate_find_peaks(
        samples,
        sample_count,
        threshold,
        minimum_peak_distance,
        peak_indices,
        HEART_RATE_MAX_PEAKS
    );

    if(peak_count < 2U)
    {
        return 0.0f;
    }

    return heart_rate_calculate(
        peak_indices,
        peak_count,
        sample_rate_hz
    );
}