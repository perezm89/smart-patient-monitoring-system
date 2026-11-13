
#include "buzzer.h"
#include "driver/ledc.h"

#define BUZZER_GPIO         GPIO_NUM_18
#define BUZZER_FREQUENCY    4000

#define BUZZER_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_TIMER   LEDC_TIMER_0
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0

#define BUZZER_DUTY_RES     LEDC_TIMER_10_BIT
#define BUZZER_DUTY         512

esp_err_t buzzer_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = BUZZER_LEDC_MODE,
        .timer_num = BUZZER_LEDC_TIMER,
        .duty_resolution = BUZZER_DUTY_RES,
        .freq_hz = BUZZER_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t ret = ledc_timer_config(&timer_config);

    if (ret != ESP_OK)
        return ret;

    ledc_channel_config_t channel_config = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = BUZZER_LEDC_MODE,
        .channel = BUZZER_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BUZZER_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };

    return ledc_channel_config(&channel_config);
}

esp_err_t buzzer_on(void)
{
    esp_err_t ret = ledc_set_duty(
        BUZZER_LEDC_MODE,
        BUZZER_LEDC_CHANNEL,
        BUZZER_DUTY
    );

    if (ret != ESP_OK)
        return ret;

    return ledc_update_duty(
        BUZZER_LEDC_MODE,
        BUZZER_LEDC_CHANNEL
    );
}

esp_err_t buzzer_off(void)
{
    esp_err_t ret = ledc_set_duty(
        BUZZER_LEDC_MODE,
        BUZZER_LEDC_CHANNEL,
        0
    );

    if (ret != ESP_OK)
        return ret;

    return ledc_update_duty(
        BUZZER_LEDC_MODE,
        BUZZER_LEDC_CHANNEL
    );
}
