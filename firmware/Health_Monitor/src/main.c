/******************************************************************************
 * File: main.c
 *
 * Description:
 * Main application entry point for the Smart Senior Monitoring System.
 * Initializes sensors, connects to Wi-Fi, synchronizes UTC time with SNTP,
 * acquires vital signs, formats them as JSON, prints the JSON payload, and
 * sends it to the backend.
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_netif_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_manager.h"
#include "i2c_interface.h"
#include "tmp117.h"
#include "max30102.h"
#include "signal_processing.h"
#include "heart_rate.h"
#include "spo2.h"
#include "json_formatter.h"

/* PPG configuration */
#define PPG_SAMPLE_RATE_HZ           100.0f
#define PPG_SAMPLE_COUNT             1000U
#define PPG_MOVING_AVERAGE_WINDOW    7U
#define PPG_COLLECTION_TIMEOUT_MS    15000U

/* Timing configuration */
#define STARTUP_DELAY_MS             2000U
#define SENSOR_STARTUP_DELAY_MS      1000U
#define COLLECTION_RETRY_DELAY_MS    1000U
#define MEASUREMENT_CYCLE_DELAY_MS   15000U
#define SNTP_SYNC_TIMEOUT_MS         10000U
#define HTTP_TIMEOUT_MS              5000

/* Network and device configuration */
#define WIFI_SSID      "FINDWAYH"
#define WIFI_PASSWORD  "helpfulresort273"

#define DEVICE_ID      "ESP32-C6-001"
#define PATIENT_ID     "PATIENT-001"

#define VITALS_API_URL \
    "http://192.168.1.33:5000/api/v1/vitals"

/*
 * Two raw buffers are required for simultaneous FIFO collection.
 * Processing buffers are reused for both optical channels.
 */
static uint32_t red_raw_samples[PPG_SAMPLE_COUNT];
static uint32_t ir_raw_samples[PPG_SAMPLE_COUNT];

static float work_samples[PPG_SAMPLE_COUNT];
static float ac_samples[PPG_SAMPLE_COUNT];
static float red_filtered_samples[PPG_SAMPLE_COUNT];
static float ir_filtered_samples[PPG_SAMPLE_COUNT];

static esp_http_client_handle_t http_client = NULL;

/**
 * @brief Synchronize system time using SNTP.
 */
static esp_err_t sync_system_time(void)
{
    const esp_sntp_config_t config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

    esp_err_t ret = esp_netif_sntp_init(&config);

    if(ret != ESP_OK)
    {
        printf(
            "SNTP initialization failed: %s\n",
            esp_err_to_name(ret)
        );

        return ret;
    }

    ret = esp_netif_sntp_sync_wait(
        pdMS_TO_TICKS(SNTP_SYNC_TIMEOUT_MS)
    );

    if(ret != ESP_OK)
    {
        printf(
            "SNTP synchronization failed: %s\n",
            esp_err_to_name(ret)
        );
    }

    return ret;
}

/**
 * @brief Initialize one reusable HTTP client.
 */
static esp_err_t http_client_init_once(void)
{
    const esp_http_client_config_t config = {
        .url = VITALS_API_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .keep_alive_enable = true
    };

    http_client = esp_http_client_init(&config);

    if(http_client == NULL)
    {
        return ESP_FAIL;
    }

    return esp_http_client_set_header(
        http_client,
        "Content-Type",
        "application/json"
    );
}

/**
 * @brief Send one JSON payload to the backend.
 */
static esp_err_t send_vitals_to_server(
    const char *json_data
)
{
    if((json_data == NULL) || (http_client == NULL))
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_http_client_set_post_field(
        http_client,
        json_data,
        strlen(json_data)
    );

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = esp_http_client_perform(http_client);

    if(ret == ESP_OK)
    {
        int status_code =
            esp_http_client_get_status_code(http_client);

        printf(
            "HTTP response: %d\n",
            status_code
        );
    }
    else
    {
        printf(
            "HTTP POST failed: %s\n",
            esp_err_to_name(ret)
        );

        /*
         * Close the failed connection so the next request starts
         * with a new socket.
         */
        esp_http_client_close(http_client);
    }

    return ret;
}

/**
 * @brief Convert, remove DC, and filter one PPG channel.
 *
 * @return Mean DC level of the unfiltered channel.
 */
static float process_ppg_channel(
    const uint32_t *raw_samples,
    float *filtered_output
)
{
    signal_convert_u32_to_float(
        raw_samples,
        work_samples,
        PPG_SAMPLE_COUNT
    );

    const float dc_level = signal_mean(
        work_samples,
        PPG_SAMPLE_COUNT
    );

    signal_remove_dc(
        work_samples,
        ac_samples,
        PPG_SAMPLE_COUNT
    );

    signal_moving_average(
        ac_samples,
        filtered_output,
        PPG_SAMPLE_COUNT,
        PPG_MOVING_AVERAGE_WINDOW
    );

    return dc_level;
}

/**
 * @brief Initialize the I2C interface and sensors.
 */
static esp_err_t initialize_sensors(void)
{
    esp_err_t ret = i2c_interface_init();

    if(ret != ESP_OK)
    {
        printf(
            "I2C initialization failed: %s\n",
            esp_err_to_name(ret)
        );

        return ret;
    }

    ret = max30102_init();

    if(ret != ESP_OK)
    {
        printf(
            "MAX30102 initialization failed: %s\n",
            esp_err_to_name(ret)
        );

        return ret;
    }

    uint8_t part_id = 0U;

    ret = max30102_read_part_id(&part_id);

    if(ret != ESP_OK)
    {
        printf(
            "MAX30102 part ID read failed: %s\n",
            esp_err_to_name(ret)
        );

        return ret;
    }

    if(part_id != MAX30102_PART_ID)
    {
        printf(
            "Invalid MAX30102 part ID: 0x%02X\n",
            part_id
        );

        return ESP_ERR_NOT_FOUND;
    }

    ret = max30102_start_measurement();

    if(ret != ESP_OK)
    {
        printf(
            "MAX30102 measurement start failed: %s\n",
            esp_err_to_name(ret)
        );

        return ret;
    }

    vTaskDelay(
        pdMS_TO_TICKS(SENSOR_STARTUP_DELAY_MS)
    );

    ret = tmp117_init();

    if(ret != ESP_OK)
    {
        printf(
            "TMP117 initialization failed: %s\n",
            esp_err_to_name(ret)
        );
    }

    return ret;
}

void app_main(void)
{
    vTaskDelay(
        pdMS_TO_TICKS(STARTUP_DELAY_MS)
    );

    esp_err_t ret = initialize_sensors();

    if(ret != ESP_OK)
    {
        return;
    }

    const bool wifi_connected =
        (wifi_manager_connect(
            WIFI_SSID,
            WIFI_PASSWORD
        ) == ESP_OK);

    bool upload_ready = false;

    if(!wifi_connected)
    {
        printf("Wi-Fi connection failed.\n");
    }
    else
    {
        ret = sync_system_time();

        if(ret == ESP_OK)
        {
            ret = http_client_init_once();

            if(ret == ESP_OK)
            {
                upload_ready = true;
            }
            else
            {
                printf(
                    "HTTP client initialization failed: %s\n",
                    esp_err_to_name(ret)
                );
            }
        }
    }

    while(1)
    {
        float temperature_c = 0.0f;

        ret = tmp117_read_temperature(
            &temperature_c
        );

        if(ret != ESP_OK)
        {
            temperature_c = 0.0f;
        }

        ret = max30102_collect_samples(
            red_raw_samples,
            ir_raw_samples,
            PPG_SAMPLE_COUNT,
            PPG_COLLECTION_TIMEOUT_MS
        );

        if(ret != ESP_OK)
        {
            printf(
                "PPG collection failed: %s\n",
                esp_err_to_name(ret)
            );

            vTaskDelay(
                pdMS_TO_TICKS(
                    COLLECTION_RETRY_DELAY_MS
                )
            );

            continue;
        }

        const float red_dc = process_ppg_channel(
            red_raw_samples,
            red_filtered_samples
        );

        const float ir_dc = process_ppg_channel(
            ir_raw_samples,
            ir_filtered_samples
        );

        spo2_result_t spo2_result = {0};

        const esp_err_t spo2_ret =
            spo2_calculate(
                red_filtered_samples,
                ir_filtered_samples,
                PPG_SAMPLE_COUNT,
                red_dc,
                ir_dc,
                &spo2_result
            );

        const float bpm = heart_rate_process(
            ir_filtered_samples,
            PPG_SAMPLE_COUNT,
            PPG_SAMPLE_RATE_HZ
        );

        const int heart_rate_bpm =
            (bpm > 0.0f) ?
            (int)bpm :
            0;

        const int spo2_percent =
            (spo2_ret == ESP_OK) ?
            (int)spo2_result.spo2_percent :
            0;

        if(upload_ready)
        {
            char timestamp[32];
            char json_buffer[JSON_BUFFER_SIZE];

            ret = json_get_timestamp(
                timestamp,
                sizeof(timestamp)
            );

            if(ret != ESP_OK)
            {
                printf(
                    "Timestamp generation failed: %s\n",
                    esp_err_to_name(ret)
                );
            }
            else
            {
                ret = json_format_sensor_data(
                    json_buffer,
                    sizeof(json_buffer),
                    DEVICE_ID,
                    PATIENT_ID,
                    timestamp,
                    heart_rate_bpm,
                    spo2_percent,
                    temperature_c
                );

                if(ret != ESP_OK)
                {
                    printf(
                        "JSON formatting failed: %s\n",
                        esp_err_to_name(ret)
                    );
                }
                else
                {
                    printf(
                        "JSON payload:\n%s\n",
                        json_buffer
                    );

                    send_vitals_to_server(
                        json_buffer
                    );
                }
            }
        }

        vTaskDelay(
            pdMS_TO_TICKS(
                MEASUREMENT_CYCLE_DELAY_MS
            )
        );
    }
}