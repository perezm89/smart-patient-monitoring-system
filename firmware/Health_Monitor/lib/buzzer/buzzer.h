
#ifndef BUZZER_H
#define BUZZER_H

#include "esp_err.h"

esp_err_t buzzer_init(void);

esp_err_t buzzer_on(void);

esp_err_t buzzer_off(void);

#endif
