#ifndef INDICATORS_H
#define INDICATORS_H

#include <stdbool.h>
#include "esp_err.h"

typedef enum
{
    LED_POWER,
    LED_CONNECTION,
    LED_LOW_BATTERY,
    LED_ALERT
} indicator_led_t;

esp_err_t indicators_init(void);

esp_err_t indicator_led_set(
    indicator_led_t led,
    bool on
);

esp_err_t indicator_led_toggle(
    indicator_led_t led
);

#endif