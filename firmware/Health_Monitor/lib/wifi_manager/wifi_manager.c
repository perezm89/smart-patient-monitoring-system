#include "wifi_manager.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_FAILED_BIT       BIT1
#define WIFI_MAX_RETRIES      5

static const char *TAG = "WIFI_MANAGER";

static EventGroupHandle_t wifi_event_group;
static int retry_count = 0;

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    if(event_base == WIFI_EVENT &&
       event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if(event_base == WIFI_EVENT &&
            event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if(retry_count < WIFI_MAX_RETRIES)
        {
            retry_count++;

            ESP_LOGW(
                TAG,
                "Connection failed. Retrying %d/%d",
                retry_count,
                WIFI_MAX_RETRIES
            );

            esp_wifi_connect();
        }
        else
        {
            xEventGroupSetBits(
                wifi_event_group,
                WIFI_FAILED_BIT
            );
        }
    }
    else if(event_base == IP_EVENT &&
            event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(
            TAG,
            "Connected. IP address: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        retry_count = 0;

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}

esp_err_t wifi_manager_connect(
    const char *ssid,
    const char *password
)
{
    if(ssid == NULL || password == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    ret = nvs_flash_init();

    if(ret == ESP_ERR_NVS_NO_FREE_PAGES ||
       ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ret = nvs_flash_erase();

        if(ret != ESP_OK)
        {
            return ret;
        }

        ret = nvs_flash_init();
    }

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = esp_netif_init();

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = esp_event_loop_create_default();

    if(ret != ESP_OK)
    {
        return ret;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config =
        WIFI_INIT_CONFIG_DEFAULT();

    ret = esp_wifi_init(&wifi_init_config);

    if(ret != ESP_OK)
    {
        return ret;
    }

    wifi_event_group = xEventGroupCreate();

    if(wifi_event_group == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    ret = esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi_event_handler,
        NULL
    );

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = esp_event_handler_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        wifi_event_handler,
        NULL
    );

    if(ret != ESP_OK)
    {
        return ret;
    }

    wifi_config_t wifi_config = {0};

    strncpy(
        (char *)wifi_config.sta.ssid,
        ssid,
        sizeof(wifi_config.sta.ssid) - 1
    );

    strncpy(
        (char *)wifi_config.sta.password,
        password,
        sizeof(wifi_config.sta.password) - 1
    );

    wifi_config.sta.threshold.authmode =
        WIFI_AUTH_WPA2_PSK;

    ret = esp_wifi_set_mode(WIFI_MODE_STA);

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = esp_wifi_set_config(
        WIFI_IF_STA,
        &wifi_config
    );

    if(ret != ESP_OK)
    {
        return ret;
    }

    retry_count = 0;

    ret = esp_wifi_start();

    if(ret != ESP_OK)
    {
        return ret;
    }

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if(bits & WIFI_CONNECTED_BIT)
    {
        return ESP_OK;
    }

    return ESP_FAIL;
}