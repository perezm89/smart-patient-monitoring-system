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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_interface.h"
#include "max30102.h"

/**
 * @brief Main application entry point.
 *
 * Initializes the I2C bus, configures the MAX30102 sensor for continuous
 * SpO2 measurements, verifies communication by reading the Part ID, and
 * continuously reads Red and IR samples from the FIFO.
 */
void app_main(void)
{
    /* Allow time for the USB serial monitor to connect after boot. */
    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("Starting application...\n");

    esp_err_t ret;

    /**************************************************************************
     * Initialize I2C Interface
     *
     * The I2C bus is used for communication between the ESP32 and connected
     * sensors.
     **************************************************************************/
    ret = i2c_interface_init();

    if(ret != ESP_OK)
    {
        printf("I2C initialization failed: %s\n",
               esp_err_to_name(ret));
        return;
    }

    printf("I2C initialized\n");

    /**************************************************************************
     * Initialize MAX30102
     *
     * Register the MAX30102 device on the I2C bus and create a device handle
     * used for future sensor communication.
     **************************************************************************/
    ret = max30102_init();

    if(ret != ESP_OK)
    {
        printf("MAX30102 initialization failed: %s\n",
               esp_err_to_name(ret));
        return;
    }

    printf("MAX30102 initialized\n");

    /**************************************************************************
     * Configure MAX30102 Measurement Settings
     *
     * Reset the sensor, configure SpO2 operating mode, set ADC parameters,
     * and configure Red/IR LED current levels.
     **************************************************************************/
    ret = max30102_start_measurement();

    if(ret != ESP_OK)
    {
        printf("MAX30102 configuration failed: %s\n",
               esp_err_to_name(ret));
        return;
    }

    printf("MAX30102 measurement started\n");

    /**************************************************************************
     * Verify Sensor Communication
     *
     * Read the Part ID register to confirm that the ESP32 can successfully
     * communicate with the MAX30102.
     **************************************************************************/
    uint8_t part_id;
    ret = max30102_read_part_id(&part_id);

    if(ret != ESP_OK)
    {
        printf("Failed to read Part ID: %s\n",
               esp_err_to_name(ret));
        return;
    }

    printf("MAX30102 Part ID: 0x%02X\n\n", part_id);

    /******************************************************************************
    * Main Sampling Loop
    *
    * Continuously reads Red and IR PPG samples from the MAX30102 FIFO.
    ******************************************************************************/
    while(1)
    {
        uint32_t red;
        uint32_t ir;

        /* Attempt to read the next available PPG sample from the FIFO. */
        ret = max30102_read_fifo(&red, &ir);

        if(ret == ESP_OK)
        {
            printf("RED: %-7lu   IR: %-7lu\n",
                   (unsigned long)red,
                   (unsigned long)ir);
        }
        else if(ret != ESP_ERR_NOT_FOUND)
        {
            printf("FIFO read failed: %s\n",
                   esp_err_to_name(ret));
        }

        /* Poll the FIFO every 10 ms (approximately 100 Hz). */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}