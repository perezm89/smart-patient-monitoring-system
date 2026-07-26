#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

esp_err_t wifi_manager_connect(
    const char *ssid,
    const char *password
);

#endif