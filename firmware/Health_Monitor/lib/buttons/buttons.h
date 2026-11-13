
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

QueueHandle_t buttons_get_queue(void);

typedef enum
{
    BUTTON_SOS,
    BUTTON_POWER
} button_t;

esp_err_t buttons_init(void);

bool button_is_pressed(button_t button);

esp_err_t buttons_start_task(void);

#endif
