#include "json_formatter.h"

#include <stdio.h>

#include "cJSON.h"

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
    if(json_buffer == NULL)
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

    snprintf(
        json_buffer,
        buffer_size,
        "%s",
        json_string
    );

    cJSON_free(json_string);
    cJSON_Delete(root);

    return ESP_OK;
}