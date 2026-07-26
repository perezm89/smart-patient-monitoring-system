#include "tmp117.h"
#include <stdio.h>

#define TMP117_I2C_TIMEOUT_MS       100

static i2c_master_dev_handle_t tmp117_device_handle = NULL;

// static esp_err_t tmp117_write_register(uint8_t register_address, uint16_t register_value){
//     if(tmp117_device_handle == NULL){
//         return ESP_ERR_INVALID_STATE;
//     }

//     uint8_t transmit_data[3];

//     transmit_data[0] = register_address;
//     transmit_data[1] = (uint8_t)(register_value >> 8);
//     transmit_data[2] = (uint8_t)(register_value & 0xFF);

//     return i2c_master_transmit(tmp117_device_handle, transmit_data, sizeof(transmit_data), TMP117_I2C_TIMEOUT_MS);
// }

static esp_err_t tmp117_read_register(uint8_t register_address, uint16_t *register_value){
    if(tmp117_device_handle == NULL || register_value == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t received_data[2];

    esp_err_t result = i2c_master_transmit_receive(tmp117_device_handle, &register_address, sizeof(register_address), received_data, sizeof(received_data), TMP117_I2C_TIMEOUT_MS);
    if(result != ESP_OK){
        return result;
    }

    *register_value = ((uint16_t)received_data[0] << 8) | received_data[1];
    return ESP_OK;
}

esp_err_t tmp117_init(void){
    i2c_device_config_t device_config = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = TMP117_I2C_ADDRESS_DEFAULT, .scl_speed_hz = I2C_FREQ_HZ};
    
    esp_err_t result = i2c_master_bus_add_device(i2c_bus, &device_config, &tmp117_device_handle);
    if(result != ESP_OK){
        printf("Failed to add TMP117 to I2C bus: %s\n", esp_err_to_name(result));
        tmp117_device_handle = NULL;
        return result;
    }

    uint16_t device_id;
    result = tmp117_read_device_id(&device_id);
    if(result != ESP_OK){
        printf("Failed to read device ID: %s\n", esp_err_to_name(result));
        i2c_master_bus_rm_device(tmp117_device_handle);
        tmp117_device_handle = NULL;
        return result;
    }

    if((device_id & TMP117_DEVICE_ID_MASK) != TMP117_DEVICE_ID){
        printf("Unexpected device ID: 0x%04X\n", device_id);
        i2c_master_bus_rm_device(tmp117_device_handle);
        tmp117_device_handle = NULL;
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t revision = (uint8_t)((device_id >> 12) & 0x0F);
    printf("TMP117 initialized: ID=0x%03X, revision=%u, address=0x%02X\n", device_id & TMP117_DEVICE_ID_MASK, revision, TMP117_I2C_ADDRESS_DEFAULT);
    return ESP_OK;
}

esp_err_t tmp117_read_device_id(uint16_t *device_id){
    if(device_id == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    return tmp117_read_register(TMP117_REG_DEVICE_ID, device_id);
}

esp_err_t tmp117_read_raw_temperature(int16_t *raw_temperature){
    if(raw_temperature == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t register_value;
    esp_err_t result = tmp117_read_register(TMP117_REG_TEMPERATURE, &register_value);
    if(result != ESP_OK){
        return result;
    }

    *raw_temperature = (int16_t)register_value;
    return ESP_OK;
}

esp_err_t tmp117_read_temperature(float *temperature_c){
    if(temperature_c == NULL){
        return ESP_ERR_INVALID_ARG;
    }
    int16_t raw_temperature;
    esp_err_t result = tmp117_read_raw_temperature(&raw_temperature);
    if(result != ESP_OK){
        return result;
    }

    *temperature_c = (float)raw_temperature / 128.0f;
    return ESP_OK;
}