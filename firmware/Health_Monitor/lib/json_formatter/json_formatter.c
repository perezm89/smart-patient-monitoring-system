#include "json_formatter.h"

#include <stdio.h>
#include <time.h>

#include "cJSON.h"

esp_err_t json_get_timestamp(
    char *timestamp_buffer,
    size_t buffer_size
)
{
    if(timestamp_buffer == NULL || buffer_size < 21)
    {
        return ESP_ERR_INVALID_ARG;
    }

    time_t now;
    struct tm time_info;

    time(&now);

    if(now <= 0)
    {
        return ESP_FAIL;
    }

    if(gmtime_r(&now, &time_info) == NULL)
    {
        return ESP_FAIL;
    }

    size_t written = strftime(
        timestamp_buffer,
        buffer_size,
        "%Y-%m-%dT%H:%M:%SZ",
        &time_info
    );

    if(written == 0)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t json_format_sensor_data(
    char *json_buffer,
    size_t buffer_size,
    const char *device_id,
    const char *patient_id,
    const char *timestamp,
    int heart_rate_bpm,
    int spo2_percent,
    float skin_temperature_c
)
{
    if(json_buffer == NULL ||
       device_id == NULL ||
       patient_id == NULL ||
       timestamp == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();

    if(root == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "deviceId", device_id);
    cJSON_AddStringToObject(root, "patientId", patient_id);
    cJSON_AddStringToObject(root, "timestamp", timestamp);

    cJSON_AddNumberToObject(root, "heartRateBpm", heart_rate_bpm);
    cJSON_AddNumberToObject(root, "spo2Percent", spo2_percent);
    cJSON_AddNumberToObject(root, "skinTemperatureC", skin_temperature_c);

    char *json_string = cJSON_PrintUnformatted(root);

    if(json_string == NULL)
    {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    int written = snprintf(
        json_buffer,
        buffer_size,
        "%s",
        json_string
    );

    cJSON_free(json_string);
    cJSON_Delete(root);

    if(written < 0 || (size_t)written >= buffer_size)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}