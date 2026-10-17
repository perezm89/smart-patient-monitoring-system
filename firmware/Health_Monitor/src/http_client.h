
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_err.h"

esp_err_t http_client_send_vitals(
    const char *url,
    const char *api_key,
    const char *json_payload
);

#endif
