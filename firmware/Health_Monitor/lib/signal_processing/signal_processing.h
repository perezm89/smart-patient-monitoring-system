/******************************************************************************
 * File: signal_processing.h
 * Author: Daniel Delgado
 *
 * Description:
 * Public interface for the signal processing module. Defines generic digital
 * signal processing (DSP) utility functions used to preprocess PPG signals,
 * including statistical calculations, DC offset removal, moving average
 * filtering, peak-to-peak amplitude calculation, and RMS computation.
 ******************************************************************************/

#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include <stddef.h>
#include <stdint.h>

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Calculates the arithmetic mean of a signal buffer.
 *
 * @param samples Pointer to the input sample buffer.
 * @param sample_count Number of samples in the buffer.
 *
 * @return Mean sample value, or 0.0f if the input is invalid.
 */
float signal_mean(const uint32_t *samples, size_t sample_count);

/**
 * @brief Removes the DC component from a signal.
 *
 * The mean of the input signal is subtracted from each sample,
 * producing a zero-centered output signal.
 *
 * @param input Pointer to the raw input samples.
 * @param output Pointer to the output buffer.
 * @param sample_count Number of samples in the buffer.
 */
void signal_remove_dc(const uint32_t *input, float *output, size_t sample_count);

/**
 * @brief Applies a trailing moving-average filter to a signal.
 *
 * Each output sample is computed as the average of the current sample
 * and a configurable number of previous samples. At the beginning of
 * the signal, only the available samples are averaged.
 *
 * @param input Pointer to the input signal.
 * @param output Pointer to the filtered output buffer.
 * @param sample_count Number of samples in the buffer.
 * @param window_size Number of samples in the averaging window.
 */
void signal_moving_average(const float *input, float *output, size_t sample_count, size_t window_size);

/**
 * @brief Calculates the root mean square (RMS) value of a signal.
 *
 * RMS provides a measure of the signal's average magnitude and is
 * commonly used to estimate the AC component of physiological signals.
 *
 * @param samples Pointer to the input signal.
 * @param sample_count Number of samples in the buffer.
 *
 * @return RMS value, or 0.0f if the input is invalid.
 */
float signal_rms(const float *samples, size_t sample_count);

/**
 * @brief Calculates the peak-to-peak amplitude of a signal.
 *
 * Peak-to-peak amplitude is computed as the difference between the
 * maximum and minimum sample values.
 *
 * @param samples Pointer to the input sample buffer.
 * @param sample_count Number of samples in the buffer.
 *
 * @return Peak-to-peak amplitude, or 0.0f if the input is invalid.
 */
float signal_peak_to_peak(const float *samples, size_t sample_count);

/**
 * @brief Finds the minimum value in a signal buffer.
 *
 * @param samples Pointer to the input sample buffer.
 * @param sample_count Number of samples in the buffer.
 *
 * @return Minimum sample value, or 0.0f if the input is invalid.
 */
float signal_min(const float *samples, size_t sample_count);

/**
 * @brief Finds the maximum value in a signal buffer.
 *
 * @param samples Pointer to the input sample buffer.
 * @param sample_count Number of samples in the buffer.
 *
 * @return Maximum sample value, or 0.0f if the input is invalid.
 */
float signal_max(const float *samples, size_t sample_count);

#endif