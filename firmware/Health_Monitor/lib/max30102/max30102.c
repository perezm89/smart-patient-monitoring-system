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

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
static esp_err_t max30102_get_fifo_samples(uint8_t *sample_count);
static esp_err_t max30102_configure_fifo(void);

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
        .scl_speed_hz = I2C_FREQ_HZ,
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

    vTaskDelay(pdMS_TO_TICKS(10));

    ret = max30102_clear_fifo();

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = max30102_configure_fifo();
    if(ret != ESP_OK){
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

    uint8_t fifo_samples = 0;
    esp_err_t ret = max30102_get_fifo_samples(&fifo_samples);
    if(ret != ESP_OK){
        return ret;
    }
    if(fifo_samples == 0)
    {
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t fifo_data[MAX30102_FIFO_SAMPLE_SIZE];

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

/**
 * @brief Clears the FIFO write pointer, read pointer, and overflow counter.
 *
 * Any unread samples currently stored in the FIFO are discarded.
 *
 * @return ESP_OK if all FIFO registers are reset successfully.
 * @return Error code if an I2C write fails.
 */
esp_err_t max30102_clear_fifo(void)
{
    esp_err_t ret;

    ret = max30102_write_register(
        MAX30102_REG_FIFO_WR_PTR,
        MAX30102_FIFO_POINTER_RESET
    );

    if(ret != ESP_OK)
    {
        return ret;
    }

    ret = max30102_write_register(
        MAX30102_REG_OVF_COUNTER,
        MAX30102_FIFO_POINTER_RESET
    );

    if(ret != ESP_OK)
    {
        return ret;
    }

    return max30102_write_register(
        MAX30102_REG_FIFO_RD_PTR,
        MAX30102_FIFO_POINTER_RESET
    );
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
 * @brief Retrieves the number of unread samples stored in the FIFO.
 *
 * Reads the FIFO write and read pointers, then calculates how many sample
 * pairs are currently available for processing. The calculation accounts
 * for the FIFO pointers wrapping around at the end of the buffer.
 *
 * @param[out] sample_count Pointer where the number of unread samples
 *                          will be stored.
 *
 * @return ESP_OK if the sample count was retrieved successfully.
 * @return ESP_ERR_INVALID_ARG if sample_count is NULL.
 * @return Error code if either FIFO pointer cannot be read.
 */
static esp_err_t max30102_get_fifo_samples(uint8_t *sample_count){
    if(sample_count == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t write_ptr = 0;
    uint8_t read_ptr = 0;

    esp_err_t ret = max30102_read_register(MAX30102_REG_FIFO_WR_PTR, &write_ptr, 1);
    if(ret != ESP_OK){
        return ret;
    }

    ret = max30102_read_register(MAX30102_REG_FIFO_RD_PTR, &read_ptr, 1);
    if(ret != ESP_OK){
        return ret;
    }

    write_ptr &= 0x1F;
    read_ptr &= 0x1F;

    if(write_ptr >= read_ptr)
    {
        *sample_count = write_ptr - read_ptr;
    }
    else
    {
        *sample_count = (MAX30102_FIFO_DEPTH - read_ptr) + write_ptr;
    }
    return ESP_OK;
}

/**
 * @brief Collects a fixed number of fresh Red and IR PPG samples.
 *
 * Clears the MAX30102 FIFO and repeatedly reads available samples until the
 * requested number has been collected. When the FIFO is temporarily empty,
 * the task briefly yields before checking again.
 *
 * @param[out] red_samples Buffer where Red samples are stored.
 * @param[out] ir_samples Buffer where IR samples are stored.
 * @param[in] sample_count Number of samples to collect.
 * @param[in] timeout_ms Maximum collection time in milliseconds.
 *
 * @return ESP_OK if all requested samples were collected.
 * @return ESP_ERR_INVALID_ARG if an argument is invalid.
 * @return ESP_ERR_TIMEOUT if sample collection exceeds timeout_ms.
 * @return Error code if a MAX30102 FIFO operation fails.
 */
esp_err_t max30102_collect_samples(uint32_t *red_samples, uint32_t *ir_samples, size_t sample_count, uint32_t timeout_ms){
    if(red_samples == NULL || ir_samples == NULL || sample_count == 0 || timeout_ms == 0){
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = max30102_clear_fifo();

    if(ret != ESP_OK){
        return ret;
    }

    size_t collected_samples = 0;
    int64_t start_time_us = esp_timer_get_time();

    while(collected_samples < sample_count){
        int64_t elapsed_time_ms = (esp_timer_get_time() - start_time_us) / 1000;
        if(elapsed_time_ms >= timeout_ms){
            printf("MAX30102 timeout: collected %u of %u samples\n", (unsigned int)collected_samples, (unsigned int)sample_count);
            return ESP_ERR_TIMEOUT;
        }

        uint32_t red_sample = 0;
        uint32_t ir_sample = 0;

        ret = max30102_read_fifo(&red_sample, &ir_sample);

        if(ret == ESP_OK){
            red_samples[collected_samples] = red_sample;
            ir_samples[collected_samples] = ir_sample;

            collected_samples++;
        }
        else if(ret == ESP_ERR_NOT_FOUND){
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        else{
            return ret;
        }
    }
    printf("Collected %u PPG samples successfully.\n", (unsigned int)collected_samples);
    return ESP_OK;
}

/**
 * @brief Configures the MAX30102 FIFO.
 *
 * Disables sample averaging, enables FIFO rollover, and configures the
 * almost-full threshold.
 *
 * @return ESP_OK if the FIFO configuration succeeds.
 * @return Error code if the I2C transaction fails.
 */
static esp_err_t max30102_configure_fifo(void){
    return max30102_write_register(MAX30102_REG_FIFO_CONFIG, MAX30102_FIFO_CONFIG_DEFAULT);
}