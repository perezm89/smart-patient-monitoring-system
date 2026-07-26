/******************************************************************************
 * File: main.c
 *
 * Description:
 * Main application entry point for the Smart Senior Monitoring System.
 * Initializes hardware peripherals, configures sensors, and continuously
 * acquires physiological data for processing and transmission.
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"
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


/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

#define PPG_SAMPLE_RATE_HZ           100.0f
#define PPG_SAMPLE_COUNT             1000
#define PPG_MOVING_AVERAGE_WINDOW    7
#define PPG_COLLECTION_TIMEOUT_MS    15000

#define SENSOR_STARTUP_DELAY_MS        1000U
#define COLLECTION_RETRY_DELAY_MS      1000U
#define MEASUREMENT_CYCLE_DELAY_MS     1000U

/******************************************************************************
 * Static Buffers
 ******************************************************************************/

static uint32_t red_raw_samples[PPG_SAMPLE_COUNT];
static uint32_t ir_raw_samples[PPG_SAMPLE_COUNT];

static float red_float_samples[PPG_SAMPLE_COUNT];
static float red_ac_samples[PPG_SAMPLE_COUNT];
static float red_filtered_samples[PPG_SAMPLE_COUNT];

static float ir_float_samples[PPG_SAMPLE_COUNT];
static float ir_ac_samples[PPG_SAMPLE_COUNT];
static float ir_filtered_samples[PPG_SAMPLE_COUNT];

/******************************************************************************
 * Application Entry Point
 ******************************************************************************/

#define WIFI_SSID      "wifi name goes here"
#define WIFI_PASSWORD  "password goes here"
#define DEVICE_ID   "ESP32-C6-001"
#define PATIENT_ID  "PATIENT-001"

/**
 * @brief Main application entry point.
 *
 * Initializes the shared I2C bus, MAX30102, and TMP117 sensors. Verifies
 * communication with both sensors, starts continuous PPG measurements, and
 * repeatedly acquires temperature and PPG data.
 *
 * The red and IR MAX30102 samples are used to estimate SpO2. The IR signal is
 * also converted, DC-filtered, smoothed, and used to estimate heart rate.
 */
void app_main(void)
{
    esp_err_t ret;

    /*
     * Give the serial monitor time to reconnect after flashing or reset.
     */
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\n");
    printf("========================================\n");
    printf(" Smart Senior Monitoring System\n");
    printf("========================================\n");
    printf("Application starting...\n");

    /**************************************************************************
     * I2C Initialization
     **************************************************************************/

    ret = i2c_interface_init();

    if(ret != ESP_OK)
    {
        printf(
            "I2C initialization failed: %s\n",
            esp_err_to_name(ret)
        );

        return;
    }

    printf("I2C interface initialized successfully.\n");

    /**************************************************************************
     * MAX30102 Initialization
     **************************************************************************/

    ret = max30102_init();

    if(ret != ESP_OK)
    {
        printf(
            "MAX30102 initialization failed: %s\n",
            esp_err_to_name(ret)
        );

        return;
    }

    printf("MAX30102 initialized successfully.\n");

    /*
     * Brief delay before attempting to read the device ID.
     */
    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t part_id = 0;

    ret = max30102_read_part_id(&part_id);

    if(ret != ESP_OK)
    {
        printf(
            "MAX30102 part ID read failed: %s\n",
            esp_err_to_name(ret)
        );

        return;
    }

    printf("MAX30102 Part ID: 0x%02X\n", part_id);

    if(part_id != MAX30102_PART_ID)
    {
        printf(
            "Invalid MAX30102 Part ID: expected 0x%02X, "
            "received 0x%02X\n",
            MAX30102_PART_ID,
            part_id
        );

        return;
    }

    ret = max30102_start_measurement();

    if(ret != ESP_OK)
    {
        printf(
            "MAX30102 measurement start failed: %s\n",
            esp_err_to_name(ret)
        );

        return;
    }

    printf("MAX30102 measurement started.\n");

    /*
     * Allow the optical signal and FIFO to stabilize before collecting the
     * first full measurement window.
     */
    vTaskDelay(pdMS_TO_TICKS(SENSOR_STARTUP_DELAY_MS));

    /**************************************************************************
     * TMP117 Initialization
     **************************************************************************/

    ret = tmp117_init();

    if(ret != ESP_OK)
    {
        printf(
            "TMP117 initialization failed: %s\n",
            esp_err_to_name(ret)
        );

        return;
    }

    printf("TMP117 initialized successfully.\n");

    uint16_t device_id = 0;

    ret = tmp117_read_device_id(&device_id);

    if(ret == ESP_OK)
    {
        printf("TMP117 Device ID register: 0x%04X\n", device_id);
    }
    else
    {
        printf(
            "Failed to read TMP117 device ID: %s\n",
            esp_err_to_name(ret)
        );
    }

    printf("Sensor initialization complete.\n");

    /**************************************************************************
     * Wi-Fi Connection
     **************************************************************************/

    ret = wifi_manager_connect(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    if(ret != ESP_OK)
    {
        printf(
            "Wi-Fi connection failed: %s\n",
            esp_err_to_name(ret)
        );

        printf("Continuing in offline mode.\n");
    }
    else
    {
        printf("Wi-Fi connected successfully.\n");
    }

    printf("Beginning measurement loop...\n\n");

    /**************************************************************************
     * Main Measurement Loop
     **************************************************************************/

    while(1)
    {
        int16_t raw_temperature = 0;
        float temperature_c = 0.0f;

        /**********************************************************************
         * TMP117 Temperature Measurement
         **********************************************************************/

        ret = tmp117_read_raw_temperature(
            &raw_temperature
        );

        if(ret == ESP_OK)
        {
            printf(
                "Raw temperature: %d\n",
                raw_temperature
            );
        }
        else
        {
            printf(
                "Raw temperature read failed: %s\n",
                esp_err_to_name(ret)
            );
        }

        ret = tmp117_read_temperature(
            &temperature_c
        );

        if(ret == ESP_OK)
        {
            float temperature_f =
                (temperature_c * 9.0f / 5.0f) + 32.0f;

            printf(
                "Temperature: %.3f C | %.2f F\n",
                temperature_c,
                temperature_f
            );
        }
        else
        {
            printf(
                "Temperature read failed: %s\n",
                esp_err_to_name(ret)
            );
        }

        /**********************************************************************
         * MAX30102 Sample Collection
         **********************************************************************/

        printf(
            "Collecting %u PPG samples...\n",
            (unsigned int)PPG_SAMPLE_COUNT
        );

        ret = max30102_collect_samples(
            red_raw_samples,
            ir_raw_samples,
            PPG_SAMPLE_COUNT,
            PPG_COLLECTION_TIMEOUT_MS
        );

        if(ret != ESP_OK)
        {
            printf(
               "PPG sample collection failed: %s\n",
               esp_err_to_name(ret)
            );

            vTaskDelay(
                pdMS_TO_TICKS(1000)
            );

            continue;
        }

        /*
         * Remove this printf if max30102_collect_samples() already prints
         * the same success message.
         */
        printf(
            "Collected %u PPG samples successfully.\n",
            (unsigned int)PPG_SAMPLE_COUNT
        );

        /**********************************************************************
         * Convert Raw PPG Samples
         **********************************************************************/

        signal_convert_u32_to_float(
            red_raw_samples,
            red_float_samples,
            PPG_SAMPLE_COUNT
        );

        signal_convert_u32_to_float(
            ir_raw_samples,
            ir_float_samples,
            PPG_SAMPLE_COUNT
        );

        /**********************************************************************
         * Calculate Raw DC Levels
         **********************************************************************/

        float red_dc = signal_mean(
            red_float_samples,
            PPG_SAMPLE_COUNT
        );

        float ir_dc = signal_mean(
            ir_float_samples,
            PPG_SAMPLE_COUNT
        );

        /**********************************************************************
         * Remove DC Components
         **********************************************************************/

        signal_remove_dc(
            red_float_samples,
            red_ac_samples,
            PPG_SAMPLE_COUNT
        );

        signal_remove_dc(
            ir_float_samples,
            ir_ac_samples,
            PPG_SAMPLE_COUNT
        );

        /**********************************************************************
         * Apply Identical Filtering
         **********************************************************************/

        signal_moving_average(
            red_ac_samples,
            red_filtered_samples,
            PPG_SAMPLE_COUNT,
            PPG_MOVING_AVERAGE_WINDOW
        );

        signal_moving_average(
            ir_ac_samples,
            ir_filtered_samples,
            PPG_SAMPLE_COUNT,
            PPG_MOVING_AVERAGE_WINDOW
        );

        /**********************************************************************
         * Signal Diagnostics
         **********************************************************************/

        float red_min = signal_min(
            red_filtered_samples,
            PPG_SAMPLE_COUNT
        );

        float red_max = signal_max(
            red_filtered_samples,
            PPG_SAMPLE_COUNT
        );

        float red_peak_to_peak = signal_peak_to_peak(
            red_filtered_samples,
            PPG_SAMPLE_COUNT
        );

        float red_rms = signal_rms(
            red_filtered_samples,
            PPG_SAMPLE_COUNT
        );

        float ir_min = signal_min(
            ir_filtered_samples,
            PPG_SAMPLE_COUNT
        );

        float ir_max = signal_max(
            ir_filtered_samples,
            PPG_SAMPLE_COUNT
        );

        float ir_peak_to_peak = signal_peak_to_peak(
            ir_filtered_samples,
            PPG_SAMPLE_COUNT
        );

        float ir_rms = signal_rms(
            ir_filtered_samples,
            PPG_SAMPLE_COUNT
        );

        printf(
            "Red signal: min=%.2f, max=%.2f, "
            "p-p=%.2f, RMS=%.2f, DC=%.2f\n",
            red_min,
            red_max,
            red_peak_to_peak,
            red_rms,
            red_dc
        );

        printf(
            "IR signal: min=%.2f, max=%.2f, "
            "p-p=%.2f, RMS=%.2f, DC=%.2f\n",
            ir_min,
            ir_max,
            ir_peak_to_peak,
            ir_rms,
            ir_dc
        );

        /**********************************************************************
         * SpO2 Calculation
         **********************************************************************/

        spo2_result_t spo2_result;

        esp_err_t spo2_ret = spo2_calculate(
            red_filtered_samples,
            ir_filtered_samples,
            PPG_SAMPLE_COUNT,
            red_dc,
            ir_dc,
            &spo2_result
        );

        if(spo2_ret == ESP_OK)
        {
            printf(
                "SpO2: %.1f %% | "
                "Raw SpO2=%.2f %% | "
                "R=%.3f | "
                "Red AC RMS=%.2f | "
                "IR AC RMS=%.2f\n",
                spo2_result.spo2_percent,
                spo2_result.raw_spo2_percent,
                spo2_result.ratio,
                spo2_result.red_ac_rms,
                spo2_result.ir_ac_rms
            );
        }
        else
        {
            printf(
                "SpO2 could not be calculated: %s. "
                "Keep the sensor steady and maintain "
                "consistent contact.\n",
                esp_err_to_name(spo2_ret)
            );
        }

        /**********************************************************************
         * Heart-Rate Calculation
         **********************************************************************/

        float bpm = heart_rate_process(
            ir_filtered_samples,
            PPG_SAMPLE_COUNT,
            PPG_SAMPLE_RATE_HZ
        );

        if(bpm > 0.0f)
        {
            printf(
                "Heart rate: %.1f BPM\n",
                bpm
            );
        }
        else
       {
            printf(
                "Heart rate could not be calculated. "
                "Keep the sensor steady and maintain contact.\n"
            );
        }

        char json_buffer[JSON_BUFFER_SIZE];

        ret = json_format_sensor_data(
            json_buffer,
            sizeof(json_buffer),
            DEVICE_ID,
            PATIENT_ID,
            "2026-07-12T22:00:00Z",
            (int)bpm,
            (int)spo2_result.spo2_percent,
            temperature_c
        );

        if(ret == ESP_OK)
        {
            printf("%s\n", json_buffer);
        }

        printf(
            "----------------------------------------\n"
        );

        vTaskDelay(
            pdMS_TO_TICKS(1000)
        );
    }
}