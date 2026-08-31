/******************************************************************************
 * File: heart_rate.c
 * Author: Daniel Delgado
 *
 * Description:
 * Heart rate calculation functions for processed PPG signals. Provides
 * adaptive threshold calculation, pulse peak detection, inter-beat interval
 * validation, and heart rate calculation in beats per minute.
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "heart_rate.h"
#include "signal_processing.h"
#include <math.h>
#include <stdio.h>

/******************************************************************************
 * Public Functions
 ******************************************************************************/

/**
 * @brief Calculates the minimum allowed distance between pulse peaks.
 *
 * Uses the configured maximum heart rate and the signal sampling frequency
 * to determine the minimum number of samples that may occur between two
 * consecutive heartbeat peaks. This helps prevent noise or secondary waveform
 * features from being counted as separate heartbeats.
 *
 * @param[in] sample_rate_hz Sampling frequency in hertz.
 *
 * @return Minimum allowed distance between peaks, measured in samples.
 * @return 0 if the sampling frequency is invalid.
 */
size_t heart_rate_minimum_peak_distance(float sample_rate_hz){
    if(sample_rate_hz <= 0.0f){
        return 0;
    }

    float minimum_distance = (60.0f * sample_rate_hz) / HEART_RATE_BPM_MAX;

    return (size_t)ceilf(minimum_distance);
}

/**
 * @brief Calculates an adaptive threshold for pulse peak detection.
 *
 * Computes the threshold using the signal mean and a configurable fraction
 * of the signal's peak-to-peak amplitude. Samples must exceed this threshold
 * to be considered possible heartbeat peaks.
 *
 * @param[in] samples Pointer to the preprocessed PPG sample buffer.
 * @param[in] sample_count Number of samples in the buffer.
 *
 * @return Calculated peak-detection threshold.
 * @return 0.0f if the input buffer is NULL or empty.
 */
float heart_rate_calculate_threshold(const float *samples, size_t sample_count){
    if(samples == NULL || sample_count == 0){
        return 0.0f;
    }

    float mean = signal_mean(samples, sample_count);
    float peak_to_peak = signal_peak_to_peak(samples, sample_count);

    return mean + (HEART_RATE_THRESHOLD_FACTOR * peak_to_peak);
}

/**
 * @brief Finds heartbeat peaks in a preprocessed PPG signal.
 *
 * Searches for local maxima that exceed the supplied detection threshold.
 * Detected peaks must also satisfy the minimum peak-distance requirement.
 * When two candidate peaks occur within the minimum distance, only the peak
 * with the greater amplitude is retained.
 *
 * @param[in] samples Pointer to the preprocessed PPG sample buffer.
 * @param[in] sample_count Number of samples in the input buffer.
 * @param[in] threshold Minimum sample amplitude required for peak detection.
 * @param[in] minimum_peak_distance Minimum allowed distance between peaks,
 *                                  measured in samples.
 * @param[out] peak_indices Buffer where detected peak indices are stored.
 * @param[in] max_peaks Maximum number of peak indices that may be stored.
 *
 * @return Number of peaks stored in peak_indices.
 * @return 0 if any required pointer is NULL, fewer than three samples are
 *         provided, or max_peaks is zero.
 */
size_t heart_rate_find_peaks(const float *samples, size_t sample_count, float threshold, size_t minimum_peak_distance, size_t *peak_indices, size_t max_peaks){
    if(samples == NULL || peak_indices == NULL || sample_count < 3 || max_peaks == 0){
        return 0;
    }

    size_t peak_count = 0;

    for(size_t i = 1; i < sample_count - 1; i++){
        bool is_local_peak = samples[i] > samples[i+1] && samples[i] > samples[i-1];

        bool above_threshold = samples[i] > threshold;

        if(is_local_peak && above_threshold){
            if(peak_count == 0){
                peak_indices[peak_count] = i;
                peak_count++;
            }
            else{
                size_t previous_peak = peak_indices[peak_count - 1];

                size_t distance = i - previous_peak;

                if(distance >= minimum_peak_distance){
                    if(peak_count >= max_peaks){
                        break;
                    }
                    peak_indices[peak_count] = i;
                    peak_count++;
                }
                else if(samples[i] > samples[previous_peak]){
                    peak_indices[peak_count - 1] = i;
                }
            }
        }
    }

    return peak_count;
}

/**
 * @brief Calculates heart rate from detected pulse peak indices.
 *
 * Calculates the intervals between consecutive detected peaks and rejects
 * intervals that correspond to heart rates outside the configured valid
 * range. The average valid interval is then converted to beats per minute.
 *
 * @param[in] peak_indices Pointer to the detected peak-index buffer.
 * @param[in] peak_count Number of detected peaks in the buffer.
 * @param[in] sample_rate_hz Sampling frequency in hertz.
 *
 * @return Calculated heart rate in beats per minute.
 * @return 0.0f if the input is invalid, the peak indices are not in ascending
 *         order, or no valid peak intervals remain.
 */
float heart_rate_calculate(const size_t *peak_indices, size_t peak_count, float sample_rate_hz){
    if(peak_indices == NULL || peak_count < 2 || sample_rate_hz <= 0.0f){
        return 0.0f;
    }

    float minimum_interval = (60.0f * sample_rate_hz) / HEART_RATE_BPM_MAX;
    float maximum_interval = (60.0f * sample_rate_hz) / HEART_RATE_BPM_MIN;

    size_t interval_count = 0;
    uint64_t interval_sum = 0;

    for(size_t i = 1; i < peak_count; i++){
        if(peak_indices[i] <= peak_indices[i - 1]){
            return 0.0f;
        }
        size_t interval = peak_indices[i] - peak_indices[i - 1];

        if((float)interval < minimum_interval || (float)interval > maximum_interval){
            continue;
        }
        interval_sum += interval;
        interval_count++;
    }

    if(interval_count == 0){
        return 0.0f;
    }

    float average_interval = (float)interval_sum / (float)interval_count;

    return (60.0f * sample_rate_hz) / average_interval;
}

/**
 * @brief Calculates heart rate from a preprocessed PPG signal.
 *
 * Calculates an adaptive peak threshold, identifies valid pulse peaks, and
 * determines heart rate using the average interval between consecutive peaks.
 *
 * @note The input should contain filtered, DC-removed IR PPG samples.
 * @note The sampling frequency must match the sample rate configured in the
 *       MAX30102 driver.
 *
 * @param[in] samples Pointer to the preprocessed IR PPG sample buffer.
 * @param[in] sample_count Number of samples in the buffer.
 * @param[in] sample_rate_hz Sampling frequency in hertz.
 *
 * @return Calculated heart rate in beats per minute.
 * @return 0.0f if the input is invalid or fewer than two valid peaks are found.
 */
float heart_rate_process(const float *samples, size_t sample_count, float sample_rate_hz){
    if(samples == NULL || sample_count < 3 || sample_rate_hz <= 0.0f){
        return 0.0f;
    }

    size_t peak_indices[HEART_RATE_MAX_PEAKS];

    size_t minimum_peak_distance = heart_rate_minimum_peak_distance(sample_rate_hz);

    float threshold = heart_rate_calculate_threshold(samples, sample_count);

    size_t peak_count = heart_rate_find_peaks(samples, sample_count, threshold, minimum_peak_distance, peak_indices, HEART_RATE_MAX_PEAKS);

    if(peak_count < 2){
        return 0.0f;
    }
    printf("Heart-rate peaks detected: %u\n", (unsigned int)peak_count);

    return heart_rate_calculate(peak_indices, peak_count, sample_rate_hz);
}