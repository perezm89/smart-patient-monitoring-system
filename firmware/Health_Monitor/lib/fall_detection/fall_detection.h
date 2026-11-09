
#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

#define FALL_IMPACT_THRESHOLD_G                     2.5f
#define FALL_ROTATION_THRESHOLD_DPS                 200.0f

#define FALL_INACTIVITY_ACCEL_MIN_G                 0.85f
#define FALL_INACTIVITY_ACCEL_MAX_G                 1.15f
#define FALL_INACTIVITY_GYRO_MAX_DPS                15.0f

#define FALL_ROTATION_WINDOW_MS                     500
#define FALL_INACTIVITY_DURATION_MS                 2000
#define FALL_CONFIRMATION_TIMEOUT_MS                4000

// Calibrate accelerometer.
void fall_detection_calibrate_accel(float accel[3]);

// Calculate total acceleration magnitude in g.
float fall_detection_accel_magnitude(const float accel[3]);

// Detect if acceleration magnitude is above impact threshold.
bool fall_detection_detect_impact(float accel_magnitude);
#endif

// Calibrate gyroscope.
void fall_detection_calibrate_gyro(float gyro[3]);

// Calculate total angular velocity magnitude in dps.
float fall_detection_gyro_magnitude(const float gyro[3]);

// Detect if gyro magnitude is above rotation threshold.
bool fall_detection_detect_rotation(float gyro_magnitude);

// Detect inactivity.
bool fall_detection_detect_inactivity(float accel_magnitude, float gyro_magnitude);

typedef enum
{
    FALL_STATE_MONITORING,
    FALL_STATE_WAITING_FOR_INACTIVITY,
    FALL_STATE_DETECTED
} fall_state_t;

void fall_detection_reset(void);

bool fall_detection_update(float accel_magnitude, float gyro_magnitude, uint32_t current_time_ms);