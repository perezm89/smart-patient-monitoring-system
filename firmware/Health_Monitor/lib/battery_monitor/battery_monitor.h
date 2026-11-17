#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdbool.h>
#include "esp_err.h"

/* Demonstration thresholds for a single-cell 18650. */
#define BATTERY_LOW_ON_V  3.70f
#define BATTERY_LOW_OFF_V 3.80f

/* ESP32-C6 GPIO2 = ADC1 channel 2. The two 100 kOhm resistors halve Vbat. */
esp_err_t battery_monitor_init(void);
esp_err_t battery_monitor_read_voltage(float *battery_voltage_v);
bool battery_monitor_is_low(float battery_voltage_v, bool previously_low);

#endif
