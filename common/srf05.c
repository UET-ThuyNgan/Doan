#include "srf05.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_log.h>
#include "hardware_config.h"

#define TAG "HYSRF05"

void hysrf05_init(void)
{
    gpio_set_direction(SRF_TRIG_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(SRF_ECHO_GPIO, GPIO_MODE_INPUT);
    gpio_set_level(SRF_TRIG_GPIO, 0);
}

float hysrf05_read_distance(void)
{
    int64_t start_time = 0;
    int64_t end_time = 0;

    // Phát xung TRIG 10µs
    gpio_set_level(SRF_TRIG_GPIO, 0);
    ets_delay_us(2);
    gpio_set_level(SRF_TRIG_GPIO, 1);
    ets_delay_us(10);
    gpio_set_level(SRF_TRIG_GPIO, 0);

    // Đợi ECHO lên mức cao
    int64_t timeout = esp_timer_get_time() + 30000; // 30ms
    while (gpio_get_level(SRF_ECHO_GPIO) == 0) {
        if (esp_timer_get_time() > timeout) {
            ESP_LOGW(TAG, "Timeout waiting for ECHO high");
            return -1;
        }
    }
    start_time = esp_timer_get_time();

    // Đợi ECHO xuống mức thấp
    timeout = esp_timer_get_time() + 30000;
    while (gpio_get_level(SRF_ECHO_GPIO) == 1) {
        if (esp_timer_get_time() > timeout) {
            ESP_LOGW(TAG, "Timeout waiting for ECHO low");
            return -1;
        }
    }
    end_time = esp_timer_get_time();

    // Tính thời gian xung
    int64_t pulse_duration = end_time - start_time; // µs
    if (pulse_duration <= 0) {
        return -1; // tránh chia lỗi
    }

    // Tính khoảng cách (cm)
    float distance_cm = (float)pulse_duration / 58.0f;

    return distance_cm;
}
