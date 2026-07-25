/******************************************************************************
 * File: signal_processing.c
 * Author: Daniel Delgado
 *
 * Description:
 * Generic digital signal processing (DSP) utility functions for PPG signals.
 * Provides statistical operations, DC offset removal, moving average
 * filtering, peak-to-peak amplitude calculation, and RMS computation for
 * use by higher-level heart rate and SpO₂ algorithms.
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "signal_processing.h"

#include <stddef.h>
#include <stdint.h>
#include <math.h>

/**
 * @brief Converts uint32_t input sample buffer to a float type.
 *
 * @param input Pointer to the raw input samples.
 * @param output Pointer to the output buffer.
 * @param sample_count Number of samples in the buffer.
 */
void signal_convert_u32_to_float(const uint32_t *input, float *output, size_t sample_count){
    if(input == NULL || output == NULL || sample_count == 0)
    {
        return;
    }

    for(size_t i = 0; i < sample_count; i++){
        output[i] = (float)input[i];
    }
}

/**
 * @brief Finds the minimum value in a signal buffer.
 *
 * @param samples Pointer to the input sample buffer.
 * @param sample_count Number of samples in the buffer.
 *
 * @return Minimum sample value, or 0.0f if the input is invalid.
 */
float signal_min(const float *samples, size_t sample_count){
    if(samples == NULL || sample_count == 0){
        return 0.0f;
    }

    float min = samples[0];

    for(size_t i = 1; i < sample_count; i++){
        if(samples[i] < min){
            min = samples[i];
        }
    }
    
    return min;
}

/**
 * @brief Finds the maximum value in a signal buffer.
 *
 * @param samples Pointer to the input sample buffer.
 * @param sample_count Number of samples in the buffer.
 *
 * @return Maximum sample value, or 0.0f if the input is invalid.
 */
float signal_max(const float *samples, size_t sample_count){
    if(samples == NULL || sample_count == 0){
        return 0.0f;
    }

    float max = samples[0];

    for(size_t i = 1; i < sample_count; i++){
        if(samples[i] > max){
            max = samples[i];
        }
    }

    return max;
}

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
float signal_peak_to_peak(const float *samples, size_t sample_count){
    if(samples == NULL || sample_count == 0){
        return 0.0f;
    }

    float max = signal_max(samples, sample_count);
    float min = signal_min(samples, sample_count);

    return max-min;
}

/**
 * @brief Calculates the arithmetic mean of a signal buffer.
 *
 * @param samples Pointer to the input sample buffer.
 * @param sample_count Number of samples in the buffer.
 *
 * @return Mean sample value, or 0.0f if the input is invalid.
 */
float signal_mean(const float *samples, size_t sample_count){
    if(samples == NULL || sample_count == 0)
    {
        return 0.0f;
    }

    float sum = 0.0f;

    for(size_t i = 0; i < sample_count; i++)
    {
        sum += samples[i];
    }

    return sum / (float)sample_count;
}

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
void signal_remove_dc(const float *input, float *output, size_t sample_count){
    if(input==NULL || output==NULL || sample_count==0){
        return;
    }

    float mean = signal_mean(input, sample_count);

    for(size_t i = 0; i < sample_count; i++){
        output[i] = input[i] - mean;
    }
}

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
void signal_moving_average(const float *input, float *output, size_t sample_count, size_t window_size){
    if(input==NULL || output==NULL || sample_count==0 || window_size==0){
        return;
    }

    float running_sum = 0.0f;

    for(size_t i = 0; i < sample_count; i++){
        running_sum += input[i];
        if(i >= window_size){
            running_sum -= input[i - window_size];
        }
        size_t samples_used;
        if(i + 1 < window_size){
            samples_used = i + 1;
        }
        else{
            samples_used = window_size;
        }
        output[i] = running_sum / (float)samples_used;
    }
}

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
float signal_rms(const float *samples, size_t sample_count){
    if(samples == NULL || sample_count == 0)
    {
        return 0.0f;
    }
    double sum_of_squares = 0.0;
    for(size_t i = 0; i < sample_count; i++){
        double sample = samples[i];
        sum_of_squares += sample * sample;
    }

    double mean_square = sum_of_squares / (double)sample_count;

    return (float)sqrtf(mean_square);
}