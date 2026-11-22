
#include "heart_rate.h"

#define SAMPLE_RATE_HZ      100
#define BASELINE_WINDOW    100
#define SMOOTH_WINDOW      5

#define MIN_BPM            40
#define MAX_BPM            180

#define MIN_PEAK_INTERVAL  (SAMPLE_RATE_HZ * 60 / MAX_BPM)
#define MAX_PEAK_INTERVAL  (SAMPLE_RATE_HZ * 60 / MIN_BPM)

#define PEAK_THRESHOLD     20.0f

static float baseline_buffer[BASELINE_WINDOW];
static float smooth_buffer[SMOOTH_WINDOW];

static float baseline_sum;
static float smooth_sum;

static uint32_t sample_count;
static uint32_t last_peak_sample;

static float previous_filtered;
static float previous_previous_filtered;

static bool have_peak;

void heart_rate_init(void)
{
    baseline_sum = 0;
    smooth_sum = 0;
    sample_count = 0;
    last_peak_sample = 0;
    previous_filtered = 0;
    previous_previous_filtered = 0;
    have_peak = false;

    for (int i = 0; i < BASELINE_WINDOW; i++)
        baseline_buffer[i] = 0;

    for (int i = 0; i < SMOOTH_WINDOW; i++)
        smooth_buffer[i] = 0;
}

bool heart_rate_add_sample(uint32_t ir_value, float *bpm)
{
    float sample = (float)ir_value;

    // Update moving-average baseline
    uint32_t baseline_index = sample_count % BASELINE_WINDOW;

    baseline_sum -= baseline_buffer[baseline_index];
    baseline_buffer[baseline_index] = sample;
    baseline_sum += sample;

    // Wait until baseline buffer is full
    if (sample_count < BASELINE_WINDOW - 1)
    {
        sample_count++;
        return false;
    }

    float baseline = baseline_sum / BASELINE_WINDOW;

    // Remove DC component
    float filtered = sample - baseline;

    // Smooth filtered signal
    uint32_t smooth_index = sample_count % SMOOTH_WINDOW;

    smooth_sum -= smooth_buffer[smooth_index];
    smooth_buffer[smooth_index] = filtered;
    smooth_sum += filtered;

    float smoothed = smooth_sum / SMOOTH_WINDOW;

    bool result = false;

    // Detect local maximum
    if (previous_filtered > previous_previous_filtered &&
        previous_filtered >= smoothed &&
        previous_filtered > PEAK_THRESHOLD)
    {
        uint32_t peak_sample = sample_count - 1;

        if (!have_peak)
        {
            last_peak_sample = peak_sample;
            have_peak = true;
        }
        else
        {
            uint32_t interval = peak_sample - last_peak_sample;

            if (interval >= MIN_PEAK_INTERVAL)
            {
                if (interval <= MAX_PEAK_INTERVAL)
                {
                    *bpm = (60.0f * SAMPLE_RATE_HZ) / interval;
                    result = true;
                }

                last_peak_sample = peak_sample;
            }
        }
    }

    previous_previous_filtered = previous_filtered;
    previous_filtered = smoothed;

    sample_count++;

    return result;
}
