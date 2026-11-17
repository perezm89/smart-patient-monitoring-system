#include "battery_monitor.h"
#include <stddef.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define BATTERY_ADC_UNIT    ADC_UNIT_1
#define BATTERY_ADC_CH      ADC_CHANNEL_2   /* ESP32-C6 GPIO2 */
#define BATTERY_ADC_ATTEN   ADC_ATTEN_DB_12
#define BATTERY_ADC_SAMPLES 16

/* Rtop=100k from BAT+ to GPIO2; Rbottom=100k from GPIO2 to GND. */
#define BATTERY_DIVIDER_RATIO 2.0f

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;

esp_err_t battery_monitor_init(void)
{
    if (adc_handle != NULL && cali_handle != NULL) {
        return ESP_OK;
    }

    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = BATTERY_ADC_UNIT,
    };
    esp_err_t ret = adc_oneshot_new_unit(&unit_config, &adc_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = BATTERY_ADC_ATTEN,
    };
    ret = adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CH, &channel_config);
    if (ret != ESP_OK) {
        adc_oneshot_del_unit(adc_handle);
        adc_handle = NULL;
        return ret;
    }

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = BATTERY_ADC_UNIT,
        .chan = BATTERY_ADC_CH,
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
#else
    ret = ESP_ERR_NOT_SUPPORTED;
#endif
    if (ret != ESP_OK) {
        adc_oneshot_del_unit(adc_handle);
        adc_handle = NULL;
    }
    return ret;
}

esp_err_t battery_monitor_read_voltage(float *battery_voltage_v)
{
    if (battery_voltage_v == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (adc_handle == NULL || cali_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Discard a first conversion to reduce settling effects. */
    int raw = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, BATTERY_ADC_CH, &raw);
    if (ret != ESP_OK) {
        return ret;
    }

    int sum = 0;
    for (int i = 0; i < BATTERY_ADC_SAMPLES; ++i) {
        ret = adc_oneshot_read(adc_handle, BATTERY_ADC_CH, &raw);
        if (ret != ESP_OK) {
            return ret;
        }
        sum += raw;
    }

    int avg_raw = (sum + BATTERY_ADC_SAMPLES / 2) / BATTERY_ADC_SAMPLES;
    int adc_millivolts = 0;
    ret = adc_cali_raw_to_voltage(cali_handle, avg_raw, &adc_millivolts);
    if (ret != ESP_OK) {
        return ret;
    }

    *battery_voltage_v = (adc_millivolts / 1000.0f) * BATTERY_DIVIDER_RATIO;
    return ESP_OK;
}

bool battery_monitor_is_low(float battery_voltage_v, bool previously_low)
{
    if (previously_low) {
        return battery_voltage_v < BATTERY_LOW_OFF_V;
    }
    return battery_voltage_v <= BATTERY_LOW_ON_V;
}
