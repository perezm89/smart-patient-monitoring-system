/******************************************************************************
 * File: main.c
 *
 * Description:
 * Main application entry point for MPU6050 testing.
 *
 * Initializes the shared I2C interface and prepares the system
 * for accelerometer, gyroscope, and fall detection testing.
 ******************************************************************************/

#include <stdio.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "i2c_interface.h"
#include "driver_mpu6050_basic.h"
#include "fall_detection.h"

/******************************************************************************
 * Application Entry Point
 ******************************************************************************/

void app_main(void)
{
    esp_err_t ret;

    // Allow serial monitor to reconnect.
    vTaskDelay(pdMS_TO_TICKS(2000));

    printf("\n");
    printf("========================================\n");
    printf(" MPU6050 Fall Detection Test\n");
    printf("========================================\n");
    printf("Application starting...\n");

    /**************************************************************************
     * I2C Initialization
     **************************************************************************/

    ret = i2c_interface_init();

    if (ret != ESP_OK)
    {
        printf(
            "I2C initialization failed: %s\n",
            esp_err_to_name(ret)
        );

        return;
    }

    printf("I2C interface initialized successfully.\n");

    /**************************************************************************
     * MPU6050 Initialization
     **************************************************************************/

    if(mpu6050_basic_init(MPU6050_ADDRESS_AD0_LOW) != 0)
    {
        printf("MPU6050 initialization failed.\n");
        return;
    }

    printf("MPU6050 initialized successfully.\n");

    float accel[3];
    float gyro[3];

    /**************************************************************************
     * Main Measurement Loop
     **************************************************************************/

    printf("Beginning IMU measurement loop...\n\n");

    while (1)
    {
        if(mpu6050_basic_read(accel, gyro) == 0)
        {
            // Apply accelerometer calibration
            fall_detection_calibrate_accel(accel);

            // Apply gyroscope calibration
            fall_detection_calibrate_gyro(gyro);

            // Calculate accel & gyro magnitudes
            float accel_magnitude = fall_detection_accel_magnitude(accel);
            float gyro_magnitude = fall_detection_gyro_magnitude(gyro);

            uint32_t current_time_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

            if (fall_detection_detect_impact(accel_magnitude))
            {
                printf("IMPACT: %.2f g | Gyro: %.2f dps\n",
                    accel_magnitude, gyro_magnitude);
            }

            if (fall_detection_detect_rotation(gyro_magnitude))
            {
                printf("ROTATION: %.2f dps | Accel: %.2f g\n",
                    gyro_magnitude, accel_magnitude);
            }
            
            if(fall_detection_update(accel_magnitude, gyro_magnitude, current_time_ms))
            {
                printf("\n================================\n");
                printf("       FALL DETECTED!\n");
                printf("================================\n");
            }

            // printf("Acceleration Magnitude: %.3f g\n", accel_magnitude);
            // printf(
            //     "Calibrated Accel: X=%.3f Y=%.3f Z=%.3f g\n",
            //     accel[0],
            //     accel[1],
            //     accel[2]
            // );
            // printf("Gyroscope Magnitude: %.2f dps\n", gyro_magnitude);
            // printf(
            //     "Gyro: X=%.2f Y=%.2f Z=%.2f dps\n\n",
            //     gyro[0],
            //     gyro[1],
            //     gyro[2]
            // );
        }
        else{
            printf("MPU6050 read failed.\n");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}