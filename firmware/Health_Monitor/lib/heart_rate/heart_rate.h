/******************************************************************************
 * File: heart_rate.h
 * Author: Daniel Delgado
 *
 * Description:
 * Public interface for the heart rate processing module. Defines the heart
 * rate limits, peak-detection settings, and functions used to calculate BPM
 * from preprocessed IR PPG samples.
 ******************************************************************************/
#ifndef HEART_RATE_H
#define HEART_RATE_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/******************************************************************************
 * Heart Rate Configuration
 ******************************************************************************/

#define HEART_RATE_BPM_MIN                  50.0f
#define HEART_RATE_BPM_MAX                  200.0f
#define HEART_RATE_MAX_PEAKS                32
#define HEART_RATE_THRESHOLD_FACTOR         0.4f

/******************************************************************************
 * Public Function Prototypes
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
size_t heart_rate_minimum_peak_distance(float sample_rate_hz);

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
float heart_rate_calculate_threshold(const float *samples, size_t sample_count);

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
size_t heart_rate_find_peaks(const float *samples, size_t sample_count, float threshold, size_t minimum_peak_distance, size_t *peak_indices, size_t max_peaks);

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
float heart_rate_calculate(const size_t *peak_indices, size_t peak_count, float sample_rate_hz);

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
float heart_rate_process(const float *samples, size_t sample_count, float sample_rate_hz);

#endif