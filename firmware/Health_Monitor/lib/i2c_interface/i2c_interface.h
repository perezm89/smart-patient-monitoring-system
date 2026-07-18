#ifndef I2C_INTERFACE_H
#define I2C_INTERFACE_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_PORT        I2C_NUM_0
#define I2C_SDA_PIN     GPIO_NUM_6
#define I2C_SCL_PIN     GPIO_NUM_7
#define I2C_FREQ_HZ     100000

extern i2c_master_bus_handle_t i2c_bus;

esp_err_t i2c_interface_init(void);

#endif