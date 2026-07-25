/******************************************************************************
 * File: i2c_interface.h
 * Author: Daniel Delgado
 * 
 * Description: 
 * Defines the I2C bus configuration, shared bus handle, and initialization
 * function used by all I2C sensor drivers in the project.
 ******************************************************************************/

#ifndef I2C_INTERFACE_H
#define I2C_INTERFACE_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "driver/i2c_master.h"
#include "esp_err.h"

/*
 * Shared I2C bus configuration.
 *
 * The ESP32-C6 uses I2C controller 0 with GPIO6 as SDA
 * and GPIO7 as SCL. All sensors connected to this bus
 * use the same clock frequency.
 */

#define I2C_PORT        I2C_NUM_0
#define I2C_SDA_PIN     GPIO_NUM_6
#define I2C_SCL_PIN     GPIO_NUM_7
#define I2C_FREQ_HZ     100000

/*
 * Handle for the shared I2C master bus.
 *
 * The bus is created by i2c_interface_init() and is used
 * by individual sensor drivers when registering their
 * devices with i2c_master_bus_add_device().
 */
extern i2c_master_bus_handle_t i2c_bus;

/**
 * @brief Initialize the shared I2C master bus.
 *
 * Configures the I2C controller, SDA pin, SCL pin,
 * clock source, glitch filter, and internal pull-ups.
 *
 * This function should be called once before initializing
 * any I2C sensor drivers.
 *
 * @return
 *      - ESP_OK if the bus was initialized successfully
 *      - ESP_ERR_INVALID_STATE if the bus is already initialized
 *      - Another ESP-IDF error code if initialization fails
 */
esp_err_t i2c_interface_init(void);

#endif