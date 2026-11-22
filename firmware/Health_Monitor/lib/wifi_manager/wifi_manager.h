#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t wifi_manager_connect(const char *ssid, const char *password);
bool wifi_manager_is_connected(void);
/* Restart connection attempts after the initial retry budget is exhausted. */
esp_err_t wifi_manager_reconnect(void);

#endif
