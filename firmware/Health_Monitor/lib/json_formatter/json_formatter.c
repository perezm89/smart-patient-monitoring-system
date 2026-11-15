#include "json_formatter.h"

#include <math.h>
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
    float skin_temperature_c,
    bool temperature_valid,
    bool fall_detected,
    bool sos_triggered
)
{
    if (json_buffer == NULL || buffer_size == 0 || device_id == NULL ||
        patient_id == NULL || timestamp == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    /* The backend requires every value to be present and non-null. */
    if (heart_rate_bpm <= 0 || spo2_percent <= 0 || spo2_percent > 100 ||
        !temperature_valid || !isfinite(skin_temperature_c)) {
        return ESP_ERR_INVALID_STATE;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "deviceId", device_id);
    cJSON_AddStringToObject(root, "patientId", patient_id);
    cJSON_AddStringToObject(root, "timestamp", timestamp);

    cJSON_AddNumberToObject(root, "heartRateBpm", heart_rate_bpm);
    cJSON_AddNumberToObject(root, "spo2Percent", spo2_percent);
    cJSON_AddNumberToObject(root, "skinTemperatureC", skin_temperature_c);

    cJSON_AddBoolToObject(root, "fallDetected", fall_detected);
    cJSON_AddBoolToObject(root, "sosTriggered", sos_triggered);

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json_string == NULL) {
        return ESP_ERR_NO_MEM;
    }

    int written = snprintf(json_buffer, buffer_size, "%s", json_string);
    cJSON_free(json_string);
    if (written < 0 || (size_t)written >= buffer_size) {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}
