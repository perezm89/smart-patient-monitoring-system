/******************************************************************************
 * File: max30102.h
 * Author: Daniel Delgado
 * 
 * Description:
 * Public interface for the MAX30102 pulse oximeter and heart rate sensor
 * driver. This module provides functions for initializing the sensor,
 * configuring measurement mode, and reading Red/IR FIFO samples.
 ******************************************************************************/

#ifndef MAX30102_H
#define MAX30102_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

/******************************************************************************
 * MAX30102 Device Information
 ******************************************************************************/

/* 7-bit I2C address */
#define MAX30102_I2C_ADDR               0x57

/******************************************************************************
 * Register Addresses
 ******************************************************************************/

#define MAX30102_REG_PART_ID            0xFF

#define MAX30102_REG_MODE_CONFIG        0x09

#define MAX30102_REG_FIFO_WR_PTR        0x04
#define MAX30102_REG_OVF_COUNTER        0x05
#define MAX30102_REG_FIFO_RD_PTR        0x06
#define MAX30102_REG_FIFO_DATA          0x07

#define MAX30102_REG_SPO2_CONFIG        0x0A

#define MAX30102_REG_LED_RED_PA         0x0C
#define MAX30102_REG_LED_IR_PA          0x0D

/******************************************************************************
 * Register Configuration Values
 ******************************************************************************/

#define MAX30102_RESET_BIT              0x40
#define MAX30102_MODE_SPO2              0x03
#define MAX30102_SPO2_CONFIG_DEFAULT    0x27
#define MAX30102_LED_CURRENT_DEFAULT    0x24

/******************************************************************************
 * FIFO Constants
 ******************************************************************************/

#define MAX30102_FIFO_SAMPLE_SIZE       6
#define MAX30102_FIFO_DEPTH             32

/******************************************************************************
 * Public Function Prototypes
 ******************************************************************************/

/**
 * @brief Initializes the MAX30102 device on the I2C bus.
 *
 * Configures the I2C device settings and creates a device handle that is
 * used for all subsequent communication with the sensor.
 *
 * @return ESP_OK if the device was successfully added to the I2C bus.
 * @return Error code if initialization fails.
 */
esp_err_t max30102_init(void);

/**
 * @brief Reads the MAX30102 part ID register.
 *
 * The part ID can be used to verify that communication with the sensor
 * is functioning correctly and that the connected device is a MAX30102.
 *
 * @param[out] part_id Pointer where the part ID will be stored.
 *
 * @return ESP_OK if the read was successful.
 * @return Error code if the I2C transaction fails.
 */
esp_err_t max30102_read_part_id(uint8_t *part_id);

/**
 * @brief Configures the MAX30102 for continuous SpO2 measurements.
 *
 * Performs the complete sensor initialization sequence by resetting the
 * device, selecting SpO2 operating mode, configuring the ADC and sampling
 * parameters, and setting the LED drive currents.
 *
 * @return ESP_OK if the sensor is successfully configured.
 * @return Error code if any configuration step fails.
 */
esp_err_t max30102_start_measurement(void);

/**
 * @brief Reads one sample pair from the MAX30102 FIFO.
 *
 * Checks whether unread samples are available in the FIFO. If data is
 * present, one Red LED sample and one IR LED sample are read, converted
 * from the sensor's 18-bit packed format, and returned.
 *
 * @param[out] red_data Pointer to store the Red LED sample.
 * @param[out] ir_data Pointer to store the IR LED sample.
 *
 * @return ESP_OK if a sample was successfully read.
 * @return ESP_ERR_INVALID_ARG if either pointer is NULL.
 * @return ESP_ERR_NOT_FOUND if the FIFO contains no unread samples.
 * @return Error code if the I2C transaction fails.
 */
esp_err_t max30102_read_fifo(uint32_t *red_data, uint32_t *ir_data);

#endif