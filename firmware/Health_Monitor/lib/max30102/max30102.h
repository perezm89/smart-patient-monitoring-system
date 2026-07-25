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

#include <stddef.h>
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
#define MAX30102_PART_ID                0x15

#define MAX30102_REG_MODE_CONFIG        0x09

#define MAX30102_FIFO_POINTER_RESET     0x00
#define MAX30102_REG_FIFO_WR_PTR        0x04
#define MAX30102_REG_OVF_COUNTER        0x05
#define MAX30102_REG_FIFO_RD_PTR        0x06
#define MAX30102_REG_FIFO_DATA          0x07

#define MAX30102_REG_SPO2_CONFIG        0x0A

#define MAX30102_REG_LED_RED_PA         0x0C
#define MAX30102_REG_LED_IR_PA          0x0D

#define MAX30102_REG_FIFO_CONFIG         0x08

#define MAX30102_FIFO_SAMPLE_AVERAGE_1   0x00
#define MAX30102_FIFO_ROLLOVER_ENABLE    0x10
#define MAX30102_FIFO_ALMOST_FULL_17     0x0F

#define MAX30102_FIFO_CONFIG_DEFAULT     \
    (MAX30102_FIFO_SAMPLE_AVERAGE_1 |    \
     MAX30102_FIFO_ROLLOVER_ENABLE |     \
     MAX30102_FIFO_ALMOST_FULL_17)

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

/**
 * @brief Clears the FIFO write pointer, read pointer, and overflow counter.
 *
 * Any unread samples currently stored in the FIFO are discarded.
 *
 * @return ESP_OK if all FIFO registers are reset successfully.
 * @return Error code if an I2C write fails.
 */
esp_err_t max30102_clear_fifo(void);

/**
 * @brief Collects a fixed number of fresh Red and IR PPG samples.
 *
 * Clears the MAX30102 FIFO and waits until the requested number of samples
 * has been collected. Each FIFO sample contains one Red value and one IR
 * value stored at matching indices in the output buffers.
 *
 * @param[out] red_samples Buffer where Red samples are stored.
 * @param[out] ir_samples Buffer where IR samples are stored.
 * @param[in] sample_count Number of samples to collect.
 * @param[in] timeout_ms Maximum time allowed for sample collection,
 *                       measured in milliseconds.
 *
 * @return ESP_OK if all requested samples were collected.
 * @return ESP_ERR_INVALID_ARG if either output buffer is NULL,
 *         sample_count is zero, or timeout_ms is zero.
 * @return ESP_ERR_TIMEOUT if the requested samples are not collected
 *         before the timeout expires.
 * @return Error code if clearing or reading the FIFO fails.
 */
esp_err_t max30102_collect_samples(uint32_t *red_samples, uint32_t *ir_samples, size_t sample_count, uint32_t timeout_ms);

#endif