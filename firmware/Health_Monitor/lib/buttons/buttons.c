
#include "buttons.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define BUTTON_SOS_GPIO    GPIO_NUM_20
#define BUTTON_POWER_GPIO  GPIO_NUM_19

#define BUTTON_POLL_MS        10
#define BUTTON_DEBOUNCE_MS    30
#define POWER_HOLD_MS         3000

static const char *TAG = "BUTTONS";

static TaskHandle_t button_task_handle = NULL;

static QueueHandle_t button_queue = NULL;

typedef struct 
{
    bool stable_pressed;
    bool last_raw_pressed;
    TickType_t last_change;
} button_state_t;

static gpio_num_t get_button_gpio(button_t button)
{
    switch(button)
    {
        case BUTTON_SOS:
            return BUTTON_SOS_GPIO;

        case BUTTON_POWER:
            return BUTTON_POWER_GPIO;

        default:
            return GPIO_NUM_NC;
    }
}

esp_err_t buttons_init(void)
{
    gpio_config_t config = {
        .pin_bit_mask =
            (1ULL << BUTTON_SOS_GPIO) |
            (1ULL << BUTTON_POWER_GPIO),

        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    return gpio_config(&config);
}

bool button_is_pressed(button_t button)
{
    gpio_num_t gpio = get_button_gpio(button);

    if(gpio == GPIO_NUM_NC)
        return false;

    return gpio_get_level(gpio) == 0;
}

static bool update_button(button_state_t *state, bool raw_pressed, TickType_t now)
{
    if(raw_pressed != state->last_raw_pressed)
    {
        state->last_raw_pressed = raw_pressed;
        state->last_change = now;
    }

    if((now - state->last_change) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS))
    {
        if(state->stable_pressed != raw_pressed)
        {
            state->stable_pressed = raw_pressed;
            return true;
        }
    }

    return false;
}

static void enter_light_sleep(void)
{
    ESP_LOGI(TAG, "Release power button to sleep");

    // Wait for button release
    while(button_is_pressed(BUTTON_POWER))
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Allow switch contacts to settle
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "Entering light sleep");

    esp_err_t ret = gpio_wakeup_enable(BUTTON_POWER_GPIO, GPIO_INTR_LOW_LEVEL);

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO wake configuration failed");
        return;
    }

    ret = esp_sleep_enable_gpio_wakeup();

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Sleep wake configuration failed");
        return;
    }

    ret = esp_light_sleep_start();

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Light sleep failed: %s", esp_err_to_name(ret));
    }
    else{
        ESP_LOGI(TAG, "Woke from light sleep");
    }

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);

    // Wait until wake button is released
    while(button_is_pressed(BUTTON_POWER))
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelay(pdMS_TO_TICKS(50));
}

static void button_task(void *arg)
{
    TickType_t now = xTaskGetTickCount();

    button_state_t sos = {
        .stable_pressed = button_is_pressed(BUTTON_SOS),
        .last_raw_pressed = button_is_pressed(BUTTON_SOS),
        .last_change = now
    };

    button_state_t power = {
        .stable_pressed = button_is_pressed(BUTTON_POWER),
        .last_raw_pressed = button_is_pressed(BUTTON_POWER),
        .last_change = now
    };

    TickType_t power_press_start = 0;
    bool power_long_handled = false;

    while(1)
    {
        now = xTaskGetTickCount();

        bool sos_changed = update_button(&sos, button_is_pressed(BUTTON_SOS), now);

        bool power_changed = update_button(&power, button_is_pressed(BUTTON_POWER), now);

        if(sos_changed && sos.stable_pressed)
        {
            ESP_LOGI(TAG, "SOS pressed");

            button_t event = BUTTON_SOS;

            if(xQueueSend(button_queue, &event, 0) != pdTRUE)
            {
                ESP_LOGW(TAG, "Button queue full");
            }
        }

        if(power_changed)
        {
            if(power.stable_pressed)
            {
                power_press_start = now;
                power_long_handled = false;
            }
            else
            {
                power_long_handled = false;
            }
        }

        if(power.stable_pressed && !power_long_handled && (now - power_press_start) >= pdMS_TO_TICKS(POWER_HOLD_MS))
        {
            power_long_handled = true;

            ESP_LOGI(TAG, "Power long press detected");

            enter_light_sleep();

            // Reset button state after waking
            now = xTaskGetTickCount();

            power.stable_pressed = false;
            power.last_raw_pressed = false;
            power.last_change = now;
        }

        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}

esp_err_t buttons_start_task(void)
{
    if(button_task_handle != NULL)
        return ESP_ERR_INVALID_STATE;

    if(button_queue == NULL)
    {
        button_queue = xQueueCreate(10, sizeof(button_t));

        if(button_queue == NULL)
            return ESP_ERR_NO_MEM;
    }

    BaseType_t result = xTaskCreate(
        button_task,
        "button_task",
        3072,
        NULL,
        5,
        &button_task_handle
    );

    return result == pdPASS ? ESP_OK : ESP_FAIL;
}

QueueHandle_t buttons_get_queue(void)
{
    return button_queue;
}
