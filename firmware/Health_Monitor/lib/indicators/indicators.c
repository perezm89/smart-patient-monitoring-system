
#include "indicators.h"
#include "driver/gpio.h"

#define LED_POWER_GPIO       GPIO_NUM_23
#define LED_CONNECTION_GPIO  GPIO_NUM_22
#define LED_LOW_BATTERY_GPIO GPIO_NUM_3
#define LED_ALERT_GPIO       GPIO_NUM_21

static gpio_num_t get_led_gpio(indicator_led_t led)
{
    switch (led)
    {
        case LED_POWER:
            return LED_POWER_GPIO;

        case LED_CONNECTION:
            return LED_CONNECTION_GPIO;

        case LED_LOW_BATTERY:
            return LED_LOW_BATTERY_GPIO;

        case LED_ALERT:
            return LED_ALERT_GPIO;

        default:
            return GPIO_NUM_NC;
    }
}

esp_err_t indicators_init(void)
{
    gpio_config_t config = {
        .pin_bit_mask =
            (1ULL << LED_POWER_GPIO) |
            (1ULL << LED_CONNECTION_GPIO) |
            (1ULL << LED_LOW_BATTERY_GPIO) |
            (1ULL << LED_ALERT_GPIO),

        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&config);

    if (ret != ESP_OK)
        return ret;

    gpio_set_level(LED_POWER_GPIO, 0);
    gpio_set_level(LED_CONNECTION_GPIO, 0);
    gpio_set_level(LED_LOW_BATTERY_GPIO, 0);
    gpio_set_level(LED_ALERT_GPIO, 0);

    return ESP_OK;
}

esp_err_t indicator_led_set(indicator_led_t led, bool on)
{
    gpio_num_t gpio = get_led_gpio(led);

    if (gpio == GPIO_NUM_NC)
        return ESP_ERR_INVALID_ARG;

    return gpio_set_level(gpio, on ? 1 : 0);
}

esp_err_t indicator_led_toggle(indicator_led_t led)
{
    gpio_num_t gpio = get_led_gpio(led);

    if (gpio == GPIO_NUM_NC)
        return ESP_ERR_INVALID_ARG;

    return gpio_set_level(gpio, !gpio_get_level(gpio));
}
