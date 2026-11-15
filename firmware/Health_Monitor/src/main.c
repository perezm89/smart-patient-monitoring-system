/**
 * Smart patient monitor -- integrated ESP32-C6 application.
 *
 * Sensor task: MAX30102 FIFO, TMP117 temperature, MPU6050 fall detection.
 * Event task: debounced SOS queue, indicator LEDs, short buzzer alert.
 * Telemetry task: UTC clock, JSON and HTTP POST; clears events only on 2xx.
 *
 * LibDriver MAX30102 FIFO + original heart_rate.c and spo2.c algorithms.
 * The last successful readings are retained when later calculations fail.
 * Do not send telemetry until every required vital has been measured.
 */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "buttons.h"
#include "buzzer.h"
#include "device_config.h"
#include "driver_max30102_fifo.h"
#include "driver_mpu6050_basic.h"
#include "fall_detection.h"
#include "heart_rate.h"
#include "http_client.h"
#include "i2c_interface.h"
#include "indicators.h"
#include "json_formatter.h"
#include "signal_processing.h"
#include "spo2.h"
#include "tmp117.h"
#include "wifi_manager.h"

#ifndef DEVICE_ID
#error "Add #define DEVICE_ID \"ESP32-C6-001\" to src/device_config.h"
#endif
#ifndef PATIENT_ID
#error "Add #define PATIENT_ID \"PATIENT-001\" to src/device_config.h"
#endif

#define SENSOR_PERIOD_MS       20U
#define TEMP_PERIOD_MS       1000U
#define TELEMETRY_PERIOD_MS  5000U
#define POST_RETRY_MS        2000U
#define WIFI_RETRY_MS        5000U
#define ALERT_BEEP_MS         250U
#define ALERT_TASK_PERIOD_MS   50U
#define MAX_ADC_SAMPLE       0x3FFFFUL
#define STARTUP_DISCARD      32U
#define SPO2_WINDOW_SIZE     500U   /* 5 seconds at the configured 100 Hz */
#define SPO2_REPORT_EVERY    100U   /* Recalculate SpO2 every 1 second */
#define SPO2_SMOOTH_WINDOW     5U

static const char *TAG = "MAIN";

typedef struct {
    float skin_temperature_c;
    bool temperature_valid;
    int heart_rate_bpm;
    bool heart_rate_valid;
    int spo2_percent;
    bool spo2_valid;
    float accel_magnitude_g;
    float gyro_magnitude_dps;
    bool imu_valid;
    uint32_t last_red;
    uint32_t last_ir;
    uint32_t ppg_sample_count;
    uint32_t pending_sos_count;
    uint32_t pending_fall_count;
    uint32_t beep_requests;
    bool fall_reset_requested;
} monitor_state_t;

static monitor_state_t state;
static SemaphoreHandle_t state_mutex;

/* All SpO2 work buffers are static so the 500-sample calculation does not
 * consume the sensor task's FreeRTOS stack. Only sensor_task accesses them. */
static uint32_t red_ring[SPO2_WINDOW_SIZE];
static uint32_t ir_ring[SPO2_WINDOW_SIZE];
static float red_raw[SPO2_WINDOW_SIZE];
static float ir_raw[SPO2_WINDOW_SIZE];
static float red_ac[SPO2_WINDOW_SIZE];
static float ir_ac[SPO2_WINDOW_SIZE];
static float red_filtered[SPO2_WINDOW_SIZE];
static float ir_filtered[SPO2_WINDOW_SIZE];
static uint32_t ring_next;
static uint32_t ring_count;
static uint32_t ppg_processed_samples;

/* Uses the project's original algorithms without modifying them. */
static void process_ppg_sample(uint32_t red, uint32_t ir)
{
    float bpm = 0.0f;
    if (heart_rate_add_sample(ir, &bpm) &&
        isfinite(bpm) && bpm >= 40.0f && bpm <= 180.0f) {
        xSemaphoreTake(state_mutex, portMAX_DELAY);
        state.heart_rate_bpm = (int)(bpm + 0.5f);
        state.heart_rate_valid = true;
        xSemaphoreGive(state_mutex);
    }

    red_ring[ring_next] = red;
    ir_ring[ring_next] = ir;
    ring_next = (ring_next + 1U) % SPO2_WINDOW_SIZE;
    if (ring_count < SPO2_WINDOW_SIZE) {
        ring_count++;
    }
    ppg_processed_samples++;
    if (ppg_processed_samples < SPO2_WINDOW_SIZE ||
        ((ppg_processed_samples - SPO2_WINDOW_SIZE) % SPO2_REPORT_EVERY) != 0) {
        return;
    }

    /* Oldest -> newest. The existing SpO2 module expects equally filtered
     * AC samples plus the mean DC level of each unfiltered channel. */
    for (uint32_t i = 0; i < SPO2_WINDOW_SIZE; i++) {
        uint32_t index = (ring_next + i) % SPO2_WINDOW_SIZE;
        red_raw[i] = (float)red_ring[index];
        ir_raw[i] = (float)ir_ring[index];
    }
    const float red_dc = signal_mean(red_raw, SPO2_WINDOW_SIZE);
    const float ir_dc = signal_mean(ir_raw, SPO2_WINDOW_SIZE);
    signal_remove_dc(red_raw, red_ac, SPO2_WINDOW_SIZE);
    signal_remove_dc(ir_raw, ir_ac, SPO2_WINDOW_SIZE);
    signal_moving_average(red_ac, red_filtered,
                          SPO2_WINDOW_SIZE, SPO2_SMOOTH_WINDOW);
    signal_moving_average(ir_ac, ir_filtered,
                          SPO2_WINDOW_SIZE, SPO2_SMOOTH_WINDOW);

    spo2_result_t measurement;
    if (spo2_calculate(red_filtered, ir_filtered, SPO2_WINDOW_SIZE,
                       red_dc, ir_dc, &measurement) == ESP_OK &&
        isfinite(measurement.spo2_percent) &&
        measurement.spo2_percent >= 70.0f &&
        measurement.spo2_percent <= 100.0f) {
        xSemaphoreTake(state_mutex, portMAX_DELAY);
        state.spo2_percent = (int)(measurement.spo2_percent + 0.5f);
        state.spo2_valid = true;
        xSemaphoreGive(state_mutex);
    }
}

static uint32_t uptime_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000LL);
}

static void max30102_callback(uint8_t type)
{
    (void)type;
}

static void sensor_task(void *arg)
{
    (void)arg;
    const bool temp_ready = (tmp117_init() == ESP_OK);
    const bool imu_ready = (mpu6050_basic_init(MPU6050_ADDRESS_AD0_LOW) == 0);
    const bool ppg_ready = (max30102_fifo_init(max30102_callback) == 0);

    ESP_LOGI(TAG, "Sensors: TMP117=%s MPU6050=%s MAX30102=%s",
             temp_ready ? "OK" : "FAILED",
             imu_ready ? "OK" : "FAILED",
             ppg_ready ? "OK" : "FAILED");

    fall_detection_reset();
    heart_rate_init();
    ring_next = 0;
    ring_count = 0;
    ppg_processed_samples = 0;
    uint32_t discarded_ppg = 0;
    uint32_t next_temp_ms = 0;
    uint32_t last_ppg_error_ms = 0;
    uint32_t last_sensor_log_ms = 0;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        uint32_t now = uptime_ms();

        if (imu_ready) {
            bool reset_requested;
            xSemaphoreTake(state_mutex, portMAX_DELAY);
            reset_requested = state.fall_reset_requested;
            state.fall_reset_requested = false;
            xSemaphoreGive(state_mutex);
            if (reset_requested) {
                fall_detection_reset();
            }

            float accel[3], gyro[3];
            if (mpu6050_basic_read(accel, gyro) == 0) {
                fall_detection_calibrate_accel(accel);
                fall_detection_calibrate_gyro(gyro);
                float accel_g = fall_detection_accel_magnitude(accel);
                float gyro_dps = fall_detection_gyro_magnitude(gyro);
                xSemaphoreTake(state_mutex, portMAX_DELAY);
                state.accel_magnitude_g = accel_g;
                state.gyro_magnitude_dps = gyro_dps;
                state.imu_valid = true;
                xSemaphoreGive(state_mutex);

                if (fall_detection_update(accel_g, gyro_dps, now)) {
                    xSemaphoreTake(state_mutex, portMAX_DELAY);
                    state.pending_fall_count++;
                    state.beep_requests++;
                    xSemaphoreGive(state_mutex);
                    ESP_LOGW(TAG, "Fall event detected; queued for backend");
                }
            } else {
                xSemaphoreTake(state_mutex, portMAX_DELAY);
                state.imu_valid = false;
                xSemaphoreGive(state_mutex);
            }
        }

        if (temp_ready && (int32_t)(now - next_temp_ms) >= 0) {
            float temperature_c = 0.0f;
            esp_err_t ret = tmp117_read_temperature(&temperature_c);
            xSemaphoreTake(state_mutex, portMAX_DELAY);
            if (ret == ESP_OK && isfinite(temperature_c)) {
                state.skin_temperature_c = temperature_c;
                state.temperature_valid = true;
            }
            /* On a failed read, preserve the last valid temperature. */
            xSemaphoreGive(state_mutex);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "TMP117 read failed: %s", esp_err_to_name(ret));
            }
            next_temp_ms = now + TEMP_PERIOD_MS;
        }

        if (ppg_ready) {
            uint32_t red[32], ir[32];
            uint8_t length = 32;
            if (max30102_fifo_read(red, ir, &length) == 0) {
                if (length > 0) {
                    uint8_t valid = 0;
                    uint32_t latest_red = 0, latest_ir = 0;
                    for (uint8_t i = 0; i < length; i++) {
                        if (discarded_ppg < STARTUP_DISCARD) {
                            discarded_ppg++;
                            continue;
                        }
                        if (red[i] > MAX_ADC_SAMPLE || ir[i] > MAX_ADC_SAMPLE) {
                            continue;
                        }
                        process_ppg_sample(red[i], ir[i]);
                        valid++;
                        latest_red = red[i];
                        latest_ir = ir[i];
                    }
                    if (valid > 0) {
                        xSemaphoreTake(state_mutex, portMAX_DELAY);
                        state.last_red = latest_red;
                        state.last_ir = latest_ir;
                        state.ppg_sample_count += valid;
                        xSemaphoreGive(state_mutex);
                    }
                }
            } else if ((uint32_t)(now - last_ppg_error_ms) >= 5000U) {
                ESP_LOGW(TAG, "MAX30102 FIFO read failed");
                last_ppg_error_ms = now;
            }
        }

        if ((uint32_t)(now - last_sensor_log_ms) >= 5000U) {
            last_sensor_log_ms = now;
            monitor_state_t snapshot;
            xSemaphoreTake(state_mutex, portMAX_DELAY);
            snapshot = state;
            xSemaphoreGive(state_mutex);
            if (ppg_ready && snapshot.ppg_sample_count > 0) {
                ESP_LOGI(TAG, "MAX30102 raw RED=%lu IR=%lu samples=%lu",
                         (unsigned long)snapshot.last_red,
                         (unsigned long)snapshot.last_ir,
                         (unsigned long)snapshot.ppg_sample_count);
                if (snapshot.heart_rate_valid) {
                    ESP_LOGI(TAG, "Last valid HR=%d BPM", snapshot.heart_rate_bpm);
                }
                if (snapshot.spo2_valid) {
                    ESP_LOGI(TAG, "Last valid SpO2=%d%%", snapshot.spo2_percent);
                }
            }
            if (temp_ready && snapshot.temperature_valid) {
                ESP_LOGI(TAG, "TMP117 skin temperature=%.2f C",
                         (double)snapshot.skin_temperature_c);
            }
            if (imu_ready && snapshot.imu_valid) {
                ESP_LOGI(TAG, "MPU6050 acceleration=%.2f g rotation=%.1f dps",
                         (double)snapshot.accel_magnitude_g,
                         (double)snapshot.gyro_magnitude_dps);
            }
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}

static void alert_task(void *arg)
{
    (void)arg;
    QueueHandle_t button_queue = buttons_get_queue();
    uint32_t seen_beep_requests = 0;
    uint32_t beep_end_ms = 0;
    uint32_t last_blink_ms = uptime_ms();
    bool buzzer_active = false;
    bool connection_led_on = false;

    for (;;) {
        button_t event;
        while (xQueueReceive(button_queue, &event, 0) == pdTRUE) {
            if (event == BUTTON_SOS) {
                xSemaphoreTake(state_mutex, portMAX_DELAY);
                state.pending_sos_count++;
                state.beep_requests++;
                xSemaphoreGive(state_mutex);
                ESP_LOGW(TAG, "SOS pressed; queued for backend");
            }
        }

        uint32_t beeps, pending_sos, pending_fall;
        xSemaphoreTake(state_mutex, portMAX_DELAY);
        beeps = state.beep_requests;
        pending_sos = state.pending_sos_count;
        pending_fall = state.pending_fall_count;
        xSemaphoreGive(state_mutex);

        uint32_t now = uptime_ms();
        if (beeps != seen_beep_requests) {
            seen_beep_requests = beeps;
            buzzer_on();
            buzzer_active = true;
            beep_end_ms = now + ALERT_BEEP_MS;
        }
        if (buzzer_active && (int32_t)(now - beep_end_ms) >= 0) {
            buzzer_off();
            buzzer_active = false;
        }

        indicator_led_set(LED_ALERT, pending_sos > 0 || pending_fall > 0);
        if (!wifi_manager_is_connected()) {
            connection_led_on = false;
            indicator_led_set(LED_CONNECTION, false);
        } else if ((uint32_t)(now - last_blink_ms) >= 500U) {
            last_blink_ms = now;
            connection_led_on = !connection_led_on;
            indicator_led_set(LED_CONNECTION, connection_led_on);
        }

        /* No battery ADC driver is available yet; don't invent a low-battery state. */
        indicator_led_set(LED_LOW_BATTERY, false);
        vTaskDelay(pdMS_TO_TICKS(ALERT_TASK_PERIOD_MS));
    }
}

static bool make_utc_timestamp(char out[21])
{
    time_t now;
    time(&now);
    /* Reject an unset RTC (1970) instead of sending a fabricated timestamp. */
    if (now < (time_t)1735689600) {
        return false;
    }
    struct tm utc;
    if (gmtime_r(&now, &utc) == NULL) {
        return false;
    }
    return strftime(out, 21, "%Y-%m-%dT%H:%M:%SZ", &utc) == 20;
}

static void telemetry_task(void *arg)
{
    (void)arg;
    bool sntp_started = false;
    uint32_t last_sent_ms = uptime_ms();
    uint32_t last_post_attempt_ms = 0;
    uint32_t last_wifi_retry_ms = 0;
    uint32_t last_wait_log_ms = 0;

    for (;;) {
        uint32_t now = uptime_ms();
        if (!wifi_manager_is_connected()) {
            if ((uint32_t)(now - last_wifi_retry_ms) >= WIFI_RETRY_MS) {
                last_wifi_retry_ms = now;
                (void)wifi_manager_reconnect();
            }
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (!sntp_started) {
            esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("time.google.com");
            esp_err_t ret = esp_netif_sntp_init(&sntp_config);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "SNTP setup failed: %s", esp_err_to_name(ret));
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue;
            }
            sntp_started = true;
        }

        char timestamp[21];
        if (!make_utc_timestamp(timestamp)) {
            esp_err_t sync_result =
            esp_netif_sntp_sync_wait(pdMS_TO_TICKS(2000));

            ESP_LOGI(TAG, "SNTP sync result: %s", esp_err_to_name(sync_result));
            
            if (!make_utc_timestamp(timestamp)) {
                ESP_LOGW(TAG, "Waiting for UTC time synchronization");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
        }

        monitor_state_t snapshot;
        xSemaphoreTake(state_mutex, portMAX_DELAY);
        snapshot = state;
        xSemaphoreGive(state_mutex);

        now = uptime_ms();
        /* The backend rejects nulls. Do not invent vital signs before the
         * first successful measurements; pending SOS/fall events stay queued. */
        if (!snapshot.heart_rate_valid || !snapshot.spo2_valid ||
            !snapshot.temperature_valid) {
            if ((uint32_t)(now - last_wait_log_ms) >= 5000U) {
                last_wait_log_ms = now;
                ESP_LOGW(TAG, "Waiting for initial valid HR=%d SpO2=%d TEMP=%d",
                         snapshot.heart_rate_valid, snapshot.spo2_valid,
                         snapshot.temperature_valid);
            }
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        bool pending = (snapshot.pending_sos_count > 0 || snapshot.pending_fall_count > 0);
        bool periodic_due = (uint32_t)(now - last_sent_ms) >= TELEMETRY_PERIOD_MS;
        bool retry_allowed = last_post_attempt_ms == 0 ||
                             (uint32_t)(now - last_post_attempt_ms) >= POST_RETRY_MS;
        if ((!periodic_due && !pending) || !retry_allowed) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        char json[JSON_BUFFER_SIZE];
        esp_err_t err = json_format_sensor_data(
            json, sizeof(json), DEVICE_ID, PATIENT_ID, timestamp,
            snapshot.heart_rate_bpm, snapshot.spo2_percent,
            snapshot.skin_temperature_c, snapshot.temperature_valid,
            snapshot.pending_fall_count > 0,
            snapshot.pending_sos_count > 0
        );
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "JSON formatting failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        ESP_LOGI(TAG, "POST %s", json);
        last_post_attempt_ms = now;
        err = http_client_send_vitals(BACKEND_URL, DEVICE_API_KEY, json);
        if (err == ESP_OK) {
            last_sent_ms = uptime_ms();
            xSemaphoreTake(state_mutex, portMAX_DELAY);
            /* Acknowledge only the events included in THIS successful POST.
             * A new SOS press while HTTP was in flight remains pending. */
            if (snapshot.pending_sos_count > 0 && state.pending_sos_count > 0) {
                state.pending_sos_count--;
            }
            if (snapshot.pending_fall_count > 0 && state.pending_fall_count > 0) {
                state.pending_fall_count--;
                if (state.pending_fall_count == 0) {
                    state.fall_reset_requested = true;
                }
            }
            xSemaphoreGive(state_mutex);
        } else {
            ESP_LOGW(TAG, "POST failed; pending SOS/fall flags retained for retry");
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{
    state_mutex = xSemaphoreCreateMutex();
    if (state_mutex == NULL) {
        ESP_LOGE(TAG, "Cannot allocate state mutex");
        return;
    }

    ESP_ERROR_CHECK(indicators_init());
    ESP_ERROR_CHECK(indicator_led_set(LED_POWER, true));
    ESP_ERROR_CHECK(buzzer_init());
    ESP_ERROR_CHECK(buttons_init());
    ESP_ERROR_CHECK(buttons_start_task());
    ESP_ERROR_CHECK(i2c_interface_init());

    if (xTaskCreate(sensor_task, "sensor_task", 6144, NULL, 5, NULL) != pdPASS ||
        xTaskCreate(alert_task, "alert_task", 3072, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Could not start sensor/alert tasks");
        return;
    }

    /* Button and sensor tasks keep running while Wi-Fi connects. */
    esp_err_t wifi_ret = wifi_manager_connect(WIFI_SSID, WIFI_PASSWORD);
    if (wifi_ret != ESP_OK) {
        ESP_LOGW(TAG, "Initial Wi-Fi connection failed; will retry");
    }

    if (xTaskCreate(telemetry_task, "telemetry_task", 6144, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Could not start telemetry task");
    }
}
