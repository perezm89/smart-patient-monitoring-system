
#include "http_client.h"

#include <stdio.h>
#include <string.h>

#include "esp_http_client.h"
#include "esp_log.h"

static const char *TAG = "HTTP_CLIENT";

esp_err_t http_client_send_vitals(
    const char *url,
    const char *api_key,
    const char *json_payload
)
{
    if (url == NULL || api_key == NULL || json_payload == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    esp_http_client_set_header(
        client,
        "Content-Type",
        "application/json"
    );

    esp_http_client_set_header(
        client,
        "X-API-Key",
        api_key
    );

    esp_http_client_set_post_field(
        client,
        json_payload,
        strlen(json_payload)
    );

    ESP_LOGI(TAG, "Sending vitals to backend...");

    esp_err_t ret = esp_http_client_perform(client);

    if (ret == ESP_OK)
    {
        int status_code =
            esp_http_client_get_status_code(client);

        ESP_LOGI(
            TAG,
            "HTTP Response: %d",
            status_code
        );

        if (status_code != 201)
        {
            ESP_LOGE(
                TAG,
                "Backend rejected vitals: HTTP %d",
                status_code
            );

            ret = ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGE(
            TAG,
            "HTTP request failed: %s",
            esp_err_to_name(ret)
        );
    }

    esp_http_client_cleanup(client);

    return ret;
}
