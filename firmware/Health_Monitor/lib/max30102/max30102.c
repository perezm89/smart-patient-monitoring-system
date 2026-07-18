/******************************************************************************
 * File: max30102.c
 * Author: Daniel Delgado
 *
 * Description:
 * Driver functions for the MAX30102 pulse oximeter and heart rate sensor.
 * Provides initialization, register access, FIFO data retrieval, and helper
 * functions used by the application.
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "max30102.h"
#include "i2c_interface.h"

#include <stddef.h>

/******************************************************************************
 * Private Variables
 ******************************************************************************/

 /* I2c device handle used for communication with the MAX30102 sensor */
static i2c_master_dev_handle_t max30102_handle = NULL;

/******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

/* Private helper functions 
 * Low-level register read/write operations used internally by the driver.
 */
static esp_err_t max30102_write_register(uint8_t reg, uint8_t data);
static esp_err_t max30102_read_register(uint8_t reg, uint8_t *data, size_t length);

/* Private configuration functions
 * Configure the MAX30102 operating mode, SpO2 settings, and LED currents.
 */
static esp_err_t max30102_reset(void);
static esp_err_t max30102_set_mode(void);
static esp_err_t max30102_configure_spo2(void);
static esp_err_t max30102_configure_leds(void);

/* Private FIFO functions
 * Read the number of unread samples currently stored in the FIFO.
 */
static uint8_t max30102_get_fifo_samples(void);

/******************************************************************************
 * Public Functions
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
esp_err_t max30102_init(void)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30102_I2C_ADDR,
        .scl_speed_hz = 100000,
    };

    return i2c_master_bus_add_device(i2c_bus, &dev_cfg, &max30102_handle);
}

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
esp_err_t max30102_read_part_id(uint8_t *part_id)
{
    return max30102_read_register(
        MAX30102_REG_PART_ID,
        part_id,
        1
    );
}

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
esp_err_t max30102_start_measurement(void)
{
    esp_err_t ret;

    ret = max30102_reset();

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = max30102_set_mode();

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = max30102_configure_spo2();

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = max30102_configure_leds();

    if(ret != ESP_OK)
    {
        return ret;
    }

    return ESP_OK;
}

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
esp_err_t max30102_read_fifo(uint32_t *red_data, uint32_t *ir_data)
{
    if(red_data == NULL || ir_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t fifo_samples = max30102_get_fifo_samples();

    if(fifo_samples == 0)
    {
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t fifo_data[MAX30102_FIFO_SAMPLE_SIZE];

    esp_err_t ret;

    ret = max30102_read_register(
        MAX30102_REG_FIFO_DATA,
        fifo_data,
        MAX30102_FIFO_SAMPLE_SIZE
    );

    if(ret != ESP_OK)
    {
        return ret;
    }

    *red_data =
        ((uint32_t)fifo_data[0] << 16) |
        ((uint32_t)fifo_data[1] << 8)  |
        ((uint32_t)fifo_data[2]);

    *ir_data =
        ((uint32_t)fifo_data[3] << 16) |
        ((uint32_t)fifo_data[4] << 8)  |
        ((uint32_t)fifo_data[5]);

    /** 
     * The MAX30102 stores each ADC measurement as an 18-bit value packed
     * into three bytes. Mask off the unused upper bits after combining
     * the bytes into a 32-bit integer.
     */
    *red_data &= 0x03FFFF;
    *ir_data &= 0x03FFFF;

    return ESP_OK;
}

/******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Writes a value to a MAX30102 register.
 *
 * Performs a single I2C write transaction consisting of the register
 * address followed by the data byte.
 *
 * @param[in] reg Register address.
 * @param[in] data Value to write.
 *
 * @return ESP_OK if the write succeeds.
 * @return Error code if the I2C transaction fails.
 */
static esp_err_t max30102_write_register(uint8_t reg, uint8_t data)
{
    uint8_t buffer[2];

    buffer[0] = reg;
    buffer[1] = data;

    return i2c_master_transmit(
        max30102_handle,
        buffer,
        2,
        100
    );
}

/**
 * @brief Reads one or more bytes from the MAX30102.
 *
 * Starts reading from the specified register and reads the requested
 * number of consecutive bytes.
 *
 * @param[in] reg Starting register address.
 * @param[out] data Buffer that receives the data.
 * @param[in] length Number of bytes to read.
 *
 * @return ESP_OK if the read succeeds.
 * @return Error code if the I2C transaction fails.
 */
static esp_err_t max30102_read_register(uint8_t reg, uint8_t *data, size_t length)
{
    return i2c_master_transmit_receive(
        max30102_handle,
        &reg,
        1,
        data,
        length,
        100
    );
}

/**
 * @brief Resets the MAX30102.
 *
 * Sets the reset bit in the MODE_CONFIG register, causing the device
 * to return to its default operating state.
 *
 * @return ESP_OK if the command is successfully sent.
 * @return Error code if the I2C transaction fails.
 */
static esp_err_t max30102_reset(void)
{
    return max30102_write_register(
        MAX30102_REG_MODE_CONFIG,
        MAX30102_RESET_BIT
    );
}

/**
 * @brief Places the MAX30102 into SpO2 operating mode.
 *
 * Configures the sensor to collect alternating Red and IR LED samples
 * for heart rate and SpO2 measurements.
 *
 * @return ESP_OK if the command succeeds.
 * @return Error code if the I2C transaction fails.
 */
static esp_err_t max30102_set_mode(void)
{
    return max30102_write_register(
        MAX30102_REG_MODE_CONFIG,
        MAX30102_MODE_SPO2
    );
}

/**
 * @brief Configures the SpO2 measurement settings.
 *
 * Programs the SPO2_CONFIG register with the desired ADC range,
 * sample rate, and LED pulse width.
 *
 * @note The configuration value (0x27) should match the application's
 * desired sampling parameters.
 *
 * @return ESP_OK if the configuration succeeds.
 * @return Error code if the I2C transaction fails.
 */
static esp_err_t max30102_configure_spo2(void)
{
    return max30102_write_register(
        MAX30102_REG_SPO2_CONFIG,
        MAX30102_SPO2_CONFIG_DEFAULT
    );
}

/**
 * @brief Configures the Red and IR LED drive currents.
 *
 * Sets the LED pulse amplitudes used during SpO2 measurements.
 *
 * @return ESP_OK if both LEDs are successfully configured.
 * @return Error code if either I2C transaction fails.
 */
static esp_err_t max30102_configure_leds(void)
{
    esp_err_t ret;

    ret = max30102_write_register(
        MAX30102_REG_LED_RED_PA,
        MAX30102_LED_CURRENT_DEFAULT
    );

    if(ret != ESP_OK)
    {
        return ret;
    }

    return max30102_write_register(
        MAX30102_REG_LED_IR_PA,
        MAX30102_LED_CURRENT_DEFAULT
    );
}

/**
 * @brief Returns the number of unread samples in the FIFO.
 *
 * Reads the FIFO write and read pointers and calculates the number of
 * samples currently waiting to be processed, accounting for pointer
 * wrap-around.
 *
 * @return Number of unread FIFO samples.
 */
static uint8_t max30102_get_fifo_samples(void)
{
    uint8_t write_ptr;
    uint8_t read_ptr;

    max30102_read_register(
        MAX30102_REG_FIFO_WR_PTR,
        &write_ptr,
        1
    );

    max30102_read_register(
        MAX30102_REG_FIFO_RD_PTR,
        &read_ptr,
        1
    );

    if(write_ptr >= read_ptr)
    {
        return write_ptr - read_ptr;
    }
    else
    {
        return (MAX30102_FIFO_DEPTH - read_ptr) + write_ptr;
    }
}
