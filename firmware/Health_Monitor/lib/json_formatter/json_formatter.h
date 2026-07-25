#ifndef JSON_FORMATTER_H
#define JSON_FORMATTER_H

#include <stddef.h>
#include "esp_err.h"

#define JSON_BUFFER_SIZE 256

esp_err_t json_format_sensor_data(
    char *json_buffer,
    size_t buffer_size,
    const char *device_id,
    const char *patient_id,
    const char *timestamp,
    int heart_rate_bpm,
    int spo2_percent,
    float skin_temperature_c
);

#endif