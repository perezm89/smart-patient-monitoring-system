
#include "fall_detection.h"
#include <math.h>

// Accelerometer calibration parameters
static const float accel_offset[3] = {
    0.0615f,    // X-axis
    -0.0090f,   // Y-axis
    -0.2000f    // Z-axis
};
static const float accel_scale[3] = {
    0.9825f,    // X-axis
    0.9770f,    // Y-axis
    1.0220f     // Z-axis
};

// Gyroscope calibration offsets (degrees per second)
static const float gyro_offset[3] = {
    -0.6793f,  // X-axis
    -1.7753f,  // Y-axis
    -2.1143f   // Z-axis
};

static fall_state_t fall_state = FALL_STATE_MONITORING;
static uint32_t last_rotation_time = 0;
static uint32_t impact_time = 0;
static uint32_t inactivity_start_time = 0;
static bool rotation_recorded = false;
static bool inactivity_started = false;

// Calibrate accelerometer.
void fall_detection_calibrate_accel(float accel[3])
{
    for (int i = 0; i < 3; i++)
    {
        accel[i] = (accel[i] - accel_offset[i]) / accel_scale[i];
    }
}

// Calculate total acceleration magnitude in g.
float fall_detection_accel_magnitude(const float accel[3])
{
    return sqrtf(
        accel[0] * accel[0] +
        accel[1] * accel[1] +
        accel[2] * accel[2]
    );
}

// Detect if acceleration magnitude is above impact threshold.
bool fall_detection_detect_impact(float accel_magnitude)
{
    return accel_magnitude >= FALL_IMPACT_THRESHOLD_G;
}

// Calibrate gyroscope.
void fall_detection_calibrate_gyro(float gyro[3])
{
    for (int i = 0; i < 3; i++)
    {
        gyro[i] -= gyro_offset[i];
    }
}

// Calculate total angular velocity magnitude in dps.
float fall_detection_gyro_magnitude(const float gyro[3])
{
    return sqrtf(
        gyro[0] * gyro[0] +
        gyro[1] * gyro[1] +
        gyro[2] * gyro[2]
    );
}

// Detect if gyro magnitude is above rotation threshold.
bool fall_detection_detect_rotation(float gyro_magnitude)
{
    return gyro_magnitude >= FALL_ROTATION_THRESHOLD_DPS;
}

// Detect inactivity.
bool fall_detection_detect_inactivity(float accel_magnitude, float gyro_magnitude)
{
    bool accel_stable =
        (accel_magnitude >= FALL_INACTIVITY_ACCEL_MIN_G) &&
        (accel_magnitude <= FALL_INACTIVITY_ACCEL_MAX_G);

    bool gyro_stable =
        gyro_magnitude < FALL_INACTIVITY_GYRO_MAX_DPS;
    
    return accel_stable && gyro_stable;
}

void fall_detection_reset(void)
{
    fall_state = FALL_STATE_MONITORING;
    last_rotation_time = 0;
    impact_time = 0;
    inactivity_start_time = 0;

    rotation_recorded = false;
    inactivity_started = false;
}

bool fall_detection_update(float accel_magnitude, float gyro_magnitude, uint32_t current_time_ms)
{
    bool impact =
        fall_detection_detect_impact(accel_magnitude);

    bool rotation =
        fall_detection_detect_rotation(gyro_magnitude);

    bool inactive =
        fall_detection_detect_inactivity(accel_magnitude, gyro_magnitude);

    switch (fall_state)
    {
        case FALL_STATE_MONITORING:
        {
            if (rotation)
            {
                last_rotation_time = current_time_ms;
                rotation_recorded = true;
            }

            if (rotation_recorded && (uint32_t)(current_time_ms - last_rotation_time) > FALL_ROTATION_WINDOW_MS)
            {
                rotation_recorded = false;
            }

            if (impact && rotation_recorded)
            {
                impact_time = current_time_ms;
                inactivity_started = false;

                fall_state =
                    FALL_STATE_WAITING_FOR_INACTIVITY;
            }

            break;
        }

        case FALL_STATE_WAITING_FOR_INACTIVITY:
        {
            if ((uint32_t)(current_time_ms - impact_time) > FALL_CONFIRMATION_TIMEOUT_MS)
            {
                fall_detection_reset();
                break;
            }

            if (inactive)
            {
                if (!inactivity_started)
                {
                    inactivity_start_time = current_time_ms;
                    inactivity_started = true;
                }
                else if ((uint32_t)(current_time_ms - inactivity_start_time) >= FALL_INACTIVITY_DURATION_MS)
                {
                    fall_state = FALL_STATE_DETECTED;
                    return true;
                }
            }
            else
            {
                inactivity_started = false;
            }

            break;
        }

        case FALL_STATE_DETECTED:
        {
            // Remain here until explicitly reset.
            break;
        }

        default:
        {
            fall_detection_reset();
            break;
        }
    }

    return false;
}