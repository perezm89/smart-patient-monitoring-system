/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 * 
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 *
 * @file      driver_max30102_interface.c
 * @brief     driver max30102 interface template source file
 * @version   1.0.0
 * @author    Shifeng Li
 * @date      2021-11-13
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2021/11/13  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */

#include "driver_max30102_interface.h"
#include "i2c_interface.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX30102_I2C_ADDRESS            0x57
#define MAX30102_I2C_TIMEOUT            1000

static const char *TAG = "MAX30102";

static i2c_master_dev_handle_t max30102_dev = NULL;

/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - 1 iic init failed
 * @note   none
 */
uint8_t max30102_interface_iic_init(void)
{
    if(i2c_bus == NULL)
    {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return 1;
    }

    if(max30102_dev != NULL)
        return 0;
    
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30102_I2C_ADDRESS,
        .scl_speed_hz = I2C_FREQ_HZ
    };

    esp_err_t ret = i2c_master_bus_add_device(
        i2c_bus,
        &dev_config,
        &max30102_dev
    );

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Device registration failed: %s", esp_err_to_name(ret));
        return 1;
    }

    return 0;
}

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - 1 iic deinit failed
 * @note   none
 */
uint8_t max30102_interface_iic_deinit(void)
{
    if (max30102_dev == NULL)
        return 0;

    esp_err_t ret = i2c_master_bus_rm_device(max30102_dev);

    if (ret != ESP_OK)
        return 1;

    max30102_dev = NULL;

    return 0;
}

/**
 * @brief      interface iic bus read
 * @param[in]  addr iic device write address
 * @param[in]  reg iic register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t max30102_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    (void)addr;

    if (max30102_dev == NULL || buf == NULL)
        return 1;

    esp_err_t ret = i2c_master_transmit_receive(
        max30102_dev,
        &reg,
        1,
        buf,
        len,
        MAX30102_I2C_TIMEOUT
    );

    return ret == ESP_OK ? 0 : 1;
}

/**
 * @brief     interface iic bus write
 * @param[in] addr iic device write address
 * @param[in] reg iic register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t max30102_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    (void)addr;

    if (max30102_dev == NULL || (len > 0 && buf == NULL))
        return 1;

    uint8_t *tx_buffer = malloc(len + 1);

    if (tx_buffer == NULL)
        return 1;

    tx_buffer[0] = reg;

    if (len > 0)
        memcpy(&tx_buffer[1], buf, len);

    esp_err_t ret = i2c_master_transmit(
        max30102_dev,
        tx_buffer,
        len + 1,
        MAX30102_I2C_TIMEOUT
    );

    free(tx_buffer);

    return ret == ESP_OK ? 0 : 1;
}

/**
 * @brief     interface delay ms
 * @param[in] ms time
 * @note      none
 */
void max30102_interface_delay_ms(uint32_t ms)
{
    if (ms == 0)
        return;

    vTaskDelay(pdMS_TO_TICKS(ms) > 0 ? pdMS_TO_TICKS(ms) : 1);
}

/**
 * @brief     interface print format data
 * @param[in] fmt format data
 * @note      none
 */
void max30102_interface_debug_print(const char *const fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/**
 * @brief     interface receive callback
 * @param[in] type irq type
 * @note      none
 */
void max30102_interface_receive_callback(uint8_t type)
{
    ESP_LOGI(TAG, "Interrupt event: %u", type);
}
