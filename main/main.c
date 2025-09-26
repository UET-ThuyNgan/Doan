#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "wifi_station.h"
#include "mqtt_client_app.h"
#include <string.h>

#include "hardware_config.h"
#include "bh1750.h"
#include "srf05.h"
#include "dht11.h"

static const char *TAG = "MAIN";
#define _WIFI "OPPO A54"
#define _SSID "12356789"

static pump_cycle_t pump_modes[3] = {
    {5, 300},   // Mode 1: 5s ON, 300s OFF
    {10, 180},  // Mode 2: 10s ON, 180s OFF
    {20, 120}   // Mode 3: 20s ON, 120s OFF
};

static pump_mode_t current_mode = MODE_MEDIUM; // mặc định Mode 2

// ===== Relay khởi tạo =====
void relay_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_PUMP1_PIN) |
                        (1ULL << RELAY_PUMP2_PIN) |
                        (1ULL << RELAY_LIGHT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Tắt tất cả relay ban đầu
    gpio_set_level(RELAY_PUMP1_PIN, 0);
    gpio_set_level(RELAY_PUMP2_PIN, 0);
    gpio_set_level(RELAY_LIGHT_PIN, 0);
}

// ===== Task điều khiển bơm theo chế độ =====
void control_pump1_task(void *pvParameters) {
    while (1) {
        int on_s = pump_modes[current_mode].on_time_s;
        int off_s = pump_modes[current_mode].off_time_s;

        // Bật bơm
        ESP_LOGI("CONTROL_PUMP", "Pump ON (%d s)", on_s);
        gpio_set_level(RELAY_PUMP1_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(on_s * 1000));

        // Tắt bơm
        ESP_LOGI("CONTROL_PUMP", "Pump OFF (%d s)", off_s);
        gpio_set_level(RELAY_PUMP1_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(off_s * 1000));
    }
}

// ===== Hàm đổi chế độ (có thể gọi từ MQTT callback) =====
void set_pump1_mode(pump_mode_t mode) {
    if (mode >= MODE_LIGHT && mode <= MODE_HEAVY) {
        current_mode = mode;
        ESP_LOGI(TAG, "Switched to pump mode %d", mode);
    }
}

/*
// ===== MQTT callback mẫu (chưa hoàn chỉnh) =====
// giả sử nhận payload "mode1", "mode2", "mode3"
void mqtt_callback_example(const char *payload) {
    if (strcmp(payload, "mode1") == 0) set_pump1_mode(MODE_LIGHT);
    else if (strcmp(payload, "mode2") == 0) set_pump1_mode(MODE_MEDIUM);
    else if (strcmp(payload, "mode3") == 0) set_pump1_mode(MODE_HEAVY);
}
*/
//  Task điều khiển đèn theo BH1750 =====
void light_control_task(void *pvParameters) {
    bh1750_t dev;
    float lux;

    // Khởi tạo BH1750
    ESP_ERROR_CHECK(bh1750_init(&dev, I2C_NUM_0, BH1750_I2C_ADDR_LO));
    ESP_ERROR_CHECK(bh1750_set_mode(&dev, BH1750_MODE_CONTINUOUS_HIGH_RES));

    while (1) {
        if (bh1750_read_light(&dev, &lux) == ESP_OK) {
            //ESP_LOGI("LIGHT_TASK", "độ sáng = %.1f lux", lux);
            if (lux > DEFAULT_LUX_THRESHOLD) {      //DEFAULT... là 20.0f 
                gpio_set_level(RELAY_LIGHT_PIN, 0); // tắt đèn
                ESP_LOGI("LIGHT_TASK", "TẮT ĐÈN VỚI độ sáng = %.1f lux", lux);
            } else {
                gpio_set_level(RELAY_LIGHT_PIN, 1); // bật đèn
                ESP_LOGI("LIGHT_TASK", "Bật ĐÈN VỚI độ sáng = %.1f lux", lux);
            }
        } else {
            ESP_LOGW("LIGHT_TASK", "Không đọc được dữ liệu BH1750");
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // đo lại sau 2s
    }
}

// ===== Water refill task (SRF05) =====
void water_refill_task(void *pvParameters) {
    int pump_on = 0;
    int64_t pump_start_time = 0;

    hysrf05_init();
    while (1) {
        float distance = hysrf05_read_distance();
        if (distance < 0) {
            ESP_LOGW("WATER_REFILL", "Đo khoảng cách lỗi");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        if (distance > LEVEL_LOW_CM && !pump_on) {
            // Mực nước thấp -> bật bơm
            gpio_set_level(RELAY_PUMP2_PIN, 1);
            pump_on = 1;
            pump_start_time = esp_timer_get_time();
            ESP_LOGI("WATER_REFILL", "Bơm bật (distance=%.1f cm)", distance);
        } 
        else if ((distance <= LEVEL_HIGH_CM && pump_on) || 
                 (pump_on && (esp_timer_get_time() - pump_start_time) > REFILL_MAX_RUNTIME_S * 1000000)) {
            // Đầy nước hoặc bơm chạy quá lâu -> tắt bơm
            gpio_set_level(RELAY_PUMP2_PIN, 0);
            pump_on = 0;
            ESP_LOGI("WATER_REFILL", "Bơm tắt (distance=%.1f cm)", distance);
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // đọc lại sau 2 giây
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
/*
    //Khởi tạo wifi
    wifi_setup(_WIFI, _SSID);
    wifi_init_sta();

    //Khởi tạo mqtt
    mqtt_app_start();
*/




    relay_init();

    xTaskCreate(control_pump1_task, "pump1_task", 2048, NULL, 1, NULL);
    //xTaskCreate(light_control_task, "light_control_task", 3072, NULL, 2, NULL);
    //xTaskCreate(water_refill_task, "water_refill_task", 2048, NULL, 3, NULL);


}
