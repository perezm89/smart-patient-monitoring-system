#ifndef JSON_FORMATTER_H
#define JSON_FORMATTER_H

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#define JSON_BUFFER_SIZE 384

/* Every JSON value is required and non-null. If any vital has never been
 * measured successfully, return ESP_ERR_INVALID_STATE rather than sending. */
esp_err_t json_format_sensor_data(
    char *json_buffer,
    size_t buffer_size,
    const char *device_id,
    const char *patient_id,
    const char *timestamp,
    int heart_rate_bpm,
    int spo2_percent,
    float skin_temperature_c,
    bool temperature_valid,
    bool fall_detected,
    bool sos_triggered
);

#endif
