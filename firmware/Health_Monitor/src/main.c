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

#include "heart_rate.h"
#include "i2c_interface.h"
#include "max30102.h"
#include "signal_processing.h"
#include "tmp117.h"

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

#define PPG_SAMPLE_RATE_HZ           100.0f
#define PPG_SAMPLE_COUNT             1000
#define PPG_MOVING_AVERAGE_WINDOW    7
#define PPG_COLLECTION_TIMEOUT_MS    15000

/******************************************************************************
 * Static Buffers
 ******************************************************************************/

static uint32_t red_raw_samples[PPG_SAMPLE_COUNT];
static uint32_t ir_raw_samples[PPG_SAMPLE_COUNT];

static float ir_float_samples[PPG_SAMPLE_COUNT];
static float ir_ac_samples[PPG_SAMPLE_COUNT];
static float ir_filtered_samples[PPG_SAMPLE_COUNT];

/**
 * @brief Main application entry point.
 *
 * Initializes the shared I2C bus, MAX30102, and TMP117 sensors. Verifies
 * communication with both sensors, starts continuous PPG measurements, and
 * repeatedly acquires temperature and PPG data. The IR signal is preprocessed
 * and used to calculate heart rate in beats per minute.
 */
void app_main(void){
    /* Give the serial monitor time to reconnect after flashing or reset. */
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("Application starting...\n");

    esp_err_t ret = i2c_interface_init();
    if(ret != ESP_OK)    {
        printf("I2C initialization failed: %s\n", esp_err_to_name(ret));
        return;
    }

    ret = max30102_init();
    if(ret != ESP_OK){
        printf("MAX30102 initialization failed: %s\n", esp_err_to_name(ret));
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t part_id = 0;
    ret = max30102_read_part_id(&part_id);
    if(ret != ESP_OK){
        printf("MAX30102 part ID read failed: %s\n", esp_err_to_name(ret));
        return;
    }
    printf("MAX30102 Part ID: 0x%02X\n", part_id);

    if(part_id != MAX30102_PART_ID){
        printf("Invalid MAX30102 Part ID: expected 0x%02X, received 0x%02X\n", MAX30102_PART_ID, part_id);
        return;
    }

    ret = max30102_start_measurement();
    if(ret != ESP_OK){
        printf("MAX30102 measurement start failed: %s\n", esp_err_to_name(ret));
        return;
    }
    printf("MAX30102 measurement started.\n");

    ret = tmp117_init();
    if(ret != ESP_OK){
        printf("TMP117 initialization failed: %s\n", esp_err_to_name(ret));
        return;
    }

    uint16_t device_id = 0;
    ret = tmp117_read_device_id(&device_id);
    if(ret == ESP_OK){
        printf("Device ID register: 0x%04X\n", device_id);
    }
    else{
        printf("Failed to read device ID: %s\n", esp_err_to_name(ret));
    }

    while(1){
        int16_t raw_temperature;
        float temperature_c;

        ret = tmp117_read_raw_temperature(&raw_temperature);
        if(ret == ESP_OK){
            printf("Raw temperature: %d\n", raw_temperature);
        }
        else{
            printf("Raw temperature read failed: %s\n", esp_err_to_name(ret));
        }

        ret = tmp117_read_temperature(&temperature_c);
        if(ret == ESP_OK){
            float temperature_f = (temperature_c * 9.0f / 5.0f) + 32.0f;
            printf("Temperature: %.3f C | %.2f F\n", temperature_c, temperature_f);
        }
        else{
            printf("Temperature read failed: %s\n", esp_err_to_name(ret));
        }

        printf("Collecting %d PPG samples...\n", PPG_SAMPLE_COUNT);

        ret = max30102_collect_samples(red_raw_samples, ir_raw_samples, PPG_SAMPLE_COUNT, PPG_COLLECTION_TIMEOUT_MS);
        if(ret != ESP_OK){
            printf("PPG sample collection failed: %s\n", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        signal_convert_u32_to_float(ir_raw_samples, ir_float_samples, PPG_SAMPLE_COUNT);

        signal_remove_dc(ir_float_samples, ir_ac_samples, PPG_SAMPLE_COUNT);

        signal_moving_average(ir_ac_samples, ir_filtered_samples, PPG_SAMPLE_COUNT, PPG_MOVING_AVERAGE_WINDOW);

        float ir_min = signal_min(ir_filtered_samples, PPG_SAMPLE_COUNT);
        float ir_max = signal_max(ir_filtered_samples, PPG_SAMPLE_COUNT);
        float ir_peak_to_peak = signal_peak_to_peak(ir_filtered_samples, PPG_SAMPLE_COUNT);
        float ir_rms = signal_rms(ir_filtered_samples, PPG_SAMPLE_COUNT);
        printf("IR signal: min=%.2f, max=%.2f, p-p=%.2f, RMS=%.2f\n", ir_min, ir_max, ir_peak_to_peak, ir_rms);

        float ir_mean = signal_mean(ir_float_samples, PPG_SAMPLE_COUNT);
        printf("IR raw mean: %.2f\n", ir_mean);

        float bpm = heart_rate_process(ir_filtered_samples, PPG_SAMPLE_COUNT, PPG_SAMPLE_RATE_HZ);

        if(bpm > 0.0f){
            printf("Heart rate: %.1f BPM\n", bpm);
        }
        else{
            printf("Heart rate could not be calculated. Keep the sensor steady and maintain contact.\n");
        }
    }
}