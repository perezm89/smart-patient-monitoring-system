#ifndef TMP117_H
#define TMP117_H

#include "i2c_interface.h"
#include <stdint.h>
#include <stddef.h>



#define TMP117_REG_CONFIGURATION     0x01
#define TMP117_REG_HIGH_LIMIT        0x02
#define TMP117_REG_LOW_LIMIT         0x03
#define TMP117_REG_EEPROM_UNLOCK     0x04
#define TMP117_REG_EEPROM1           0x05
#define TMP117_REG_EEPROM2           0x06
#define TMP117_REG_TEMP_OFFSET       0x07
#define TMP117_REG_EEPROM3           0x08


#define TMP117_I2C_ADDRESS_DEFAULT   0x48

#define TMP117_REG_DEVICE_ID         0x0F

#define TMP117_DEVICE_ID             0x0117
#define TMP117_DEVICE_ID_MASK        0x0FFF

#define TMP117_REG_TEMPERATURE       0x00

esp_err_t tmp117_init();

esp_err_t tmp117_read_device_id(uint16_t *device_id);

esp_err_t tmp117_read_raw_temperature(int16_t *raw_temperature);

esp_err_t tmp117_read_temperature(float *temperature_c);

esp_err_t tmp117_shutdown(void);

esp_err_t tmp117_take_one_shot(float *temperature_c);

#endif