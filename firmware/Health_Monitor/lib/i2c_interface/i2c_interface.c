/******************************************************************************
 * File: i2c_interface.c
 * Author: Daniel Delgado
 * 
 * Description: 
 * This module configures and creates the ESP32-C6 I2C master bus using
 * the ESP-IDF I2C driver. The resulting bus handle is shared by all
 * sensor drivers, allowing multiple I2C devicesto communicate over the 
 * same SDA and SCL lines.
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "i2c_interface.h"

/*
 * Shared I2C master bus handle.
 *
 * It is initialized by i2c_interface_init() and accessed
 * by sensor drivers such as the MAX30102 and TMP117 drivers.
 */
i2c_master_bus_handle_t i2c_bus = NULL;

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
esp_err_t i2c_interface_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&bus_config, &i2c_bus);
}