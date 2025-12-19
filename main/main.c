#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "wifi_station.h"
#include "mqtt_client_app.h"
#include <string.h>
#include "cJSON.h"

#include "hardware_config.h"
#include "bh1750.h"
#include "srf05.h"
#include "dht11.h"
#include "rtc_ds1307.h"

static const char *TAG = "MAIN";
#define _SSID "OPPO A54"
#define _PASSWORD "12356789"

volatile float global_lux = 100;
volatile float global_distance = 11.0;
volatile int global_pump1_state = 0;
volatile int global_pump2_state = 0;
volatile int global_light_state = 0;

// ===== Định nghĩa 3 chế độ tưới =====
typedef enum {
    MODE_LIGHT = 0,   // ít
    MODE_MEDIUM,      // trung bình
    MODE_HEAVY        // nhiều
} pump_mode_t;

typedef struct {
    int on_time_s;
    int off_time_s;
} pump_cycle_t;

static pump_cycle_t pump_modes[3] = {
    {10, 200},  // Mode 1: 10s ON, 3 phút 20s phút OFF
    {20, 300},  // Mode 2: 20s ON, 5 phút OFF
    {30, 300}   // Mode 3: 30s ON, 5 phút OFF
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

// ===== Hàm đổi chế độ =====
void set_pump1_mode(pump_mode_t mode) {
    if (mode >= MODE_LIGHT && mode <= MODE_HEAVY) {
        current_mode = mode;
        ESP_LOGI(TAG, "Switched to pump mode %d", mode);
    }
}

// ===== CALLBACK XỬ LÝ RPC TỪ COREIOT =====
void rpc_handler(const char *data) {
    ESP_LOGI(TAG, "RPC received: %s", data);
    
    cJSON *root = cJSON_Parse(data);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return;
    }

    cJSON *method = cJSON_GetObjectItem(root, "method");
    cJSON *params = cJSON_GetObjectItem(root, "params");
    
    if (method && cJSON_IsString(method)) {
        const char *method_name = method->valuestring;
        
        if (strcmp(method_name, "setPumpMode") == 0) {
            if (params && cJSON_IsNumber(params)) {
                int mode = params->valueint;
                if (mode >= 0 && mode <= 2) {
                    set_pump1_mode((pump_mode_t)mode);
                    ESP_LOGI(TAG, "Đổi chế độ tưới sang: %d", mode);
                }
            }
        }
    }
    cJSON_Delete(root);
}


// ===== Task điều khiển bơm theo chế độ =====
void control_pump1_task(void *pvParameters) {
    while (1) {
        int on_s = pump_modes[current_mode].on_time_s;
        int off_s = pump_modes[current_mode].off_time_s;

        // Bật bơm
        ESP_LOGI("CONTROL_PUMP", "Pump ON (%d s)", on_s);
        gpio_set_level(RELAY_PUMP1_PIN, 1);
        global_pump1_state = 1;
        vTaskDelay(pdMS_TO_TICKS(on_s * 1000));

        // Tắt bơm
        ESP_LOGI("CONTROL_PUMP", "Pump OFF (%d s)", off_s);
        gpio_set_level(RELAY_PUMP1_PIN, 0);
        global_pump1_state = 0;
        vTaskDelay(pdMS_TO_TICKS(off_s * 1000));
    }
}

// ==== Task điều khiển đèn theo BH1750 + DS1307 =====
void light_control_task(void *pvParameters) {
    bh1750_t dev;
    float lux;
    struct tm now;
    int last_state = -1; // -1 = chưa có trạng thái trước, 0 = tắt, 1 = bật

    // Khởi tạo BH1750
    ESP_ERROR_CHECK(bh1750_init(&dev, I2C_NUM_0, BH1750_I2C_ADDR_LO));
    ESP_ERROR_CHECK(bh1750_set_mode(&dev, BH1750_MODE_CONTINUOUS_HIGH_RES));

    while (1) {
        // Đọc thời gian từ DS1307
        if (ds1307_get_time(I2C_NUM_0, &now) != ESP_OK) {
            ESP_LOGW("LIGHT_TASK", "Không đọc được thời gian RTC");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        int new_state = 0; // trạng thái đèn hiện tại muốn đặt
        if (now.tm_hour >= 6 && now.tm_hour < 20) {  // Trong khung chiếu sáng từ 6h-22h (14 tiếng 1 ngày)
            if (bh1750_read_light(&dev, &lux) == ESP_OK) {
                global_lux = lux;
                if (lux > DEFAULT_LUX_THRESHOLD + 8.0) {
                    new_state = 0; // đủ sáng → tắt đèn
                } else if (lux < DEFAULT_LUX_THRESHOLD) {
                    new_state = 1; // thiếu sáng → bật đèn
                }
            } else {
                ESP_LOGW("LIGHT_TASK", "Không đọc được dữ liệu BH1750");
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue;
            }
        } else {
            new_state = 0; // Ngoài khung giờ chiếu sáng → tắt đèn
        }

        if (new_state != last_state) {
            gpio_set_level(RELAY_LIGHT_PIN, new_state);
            global_light_state = new_state;
            if (new_state)
                ESP_LOGI("LIGHT_TASK", "BẬT ĐÈN (%.1f lux) [%02d:%02d:%02d]",
                         lux, now.tm_hour, now.tm_min, now.tm_sec);
            else
                ESP_LOGI("LIGHT_TASK", "TẮT ĐÈN (%.1f lux) [%02d:%02d:%02d]",
                         lux, now.tm_hour, now.tm_min, now.tm_sec);
            last_state = new_state;
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // kiểm tra lại mỗi 2s
    }
}

// ===== Water refill task (SRF05) =====
void water_refill_task(void *pvParameters) {
    int pump_on = 0;
    int64_t pump_start_time = 0;

    hysrf05_init();
    while (1) {
        float distance = hysrf05_read_distance();
        ESP_LOGI("WATER_REFILL", "distance=%.1f cm", distance);
        global_distance = TANK_HEIGHT - distance;

        if (distance < 0) {
            ESP_LOGW("WATER_REFILL", "Đo khoảng cách lỗi");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        if (distance > LEVEL_LOW_CM && !pump_on) {
            // Mực nước thấp -> bật bơm
            gpio_set_level(RELAY_PUMP2_PIN, 1);
            pump_on = 1;
            global_pump2_state =1;
            pump_start_time = esp_timer_get_time();
            ESP_LOGI("WATER_REFILL", "Bơm bật (distance=%.1f cm)", distance);
        } 
        else if ((distance <= LEVEL_HIGH_CM && pump_on) || 
                 (pump_on && (esp_timer_get_time() - pump_start_time) > REFILL_MAX_RUNTIME_S * 1000000)) {
            // Đầy nước hoặc bơm chạy quá lâu -> tắt bơm
            gpio_set_level(RELAY_PUMP2_PIN, 0);
            pump_on = 0;
            global_pump2_state = 0;
            ESP_LOGI("WATER_REFILL", "Bơm tắt (distance=%.1f cm)", distance);
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // đọc lại sau 2 giây
    }
}


// ===== Send data task =====
void send_data_task(void *parameters) {
    dht11_data_t dht_data;
    
    while (1) {
        // Đọc DHT11
        if (read_dht11(&dht_data) != ESP_OK) {
            ESP_LOGW(TAG, "Không đọc được DHT11");
            dht_data.temperature = 26;
            dht_data.humidity = 60;
        }
        
        // send telemetry
        char telemetry[300];
        snprintf(telemetry, sizeof(telemetry),
                "{\"temperature\":%d,\"humidity\":%d,\"lux\":%.1f,\"water_level\":%.1f,"
                "\"pump1_state\":%d,\"pump2_state\":%d,\"light_state\":%d}",
                dht_data.temperature, dht_data.humidity, global_lux, global_distance,
                global_pump1_state, global_pump2_state, global_light_state);
        
        mqtt_app_publish("v1/devices/me/telemetry", telemetry, strlen(telemetry));
        ESP_LOGI(TAG, "Telemetry: %s", telemetry);
        
        // send attributes
        char attributes[64];
        snprintf(attributes, sizeof(attributes), "{\"mode\":%d}", current_mode);
        
        mqtt_app_publish("v1/devices/me/attributes", attributes, strlen(attributes));
        ESP_LOGI(TAG, "Attributes: %s", attributes);
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Gửi mỗi 5 giây
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Khởi tạo wifi
    wifi_setup(_SSID, _PASSWORD);
    wifi_init_sta();

    //Khởi tạo mqtt
    mqtt_app_start();
    mqtt_app_register_rpc_callback(rpc_handler);
    
    
    relay_init();

    xTaskCreate(control_pump1_task, "pump1_task", 3072, NULL, 1, NULL);
    xTaskCreate(light_control_task, "light_control_task", 2048*2, NULL, 2, NULL);
    xTaskCreate(water_refill_task, "water_refill_task", 3072, NULL, 3, NULL);
    xTaskCreate(send_data_task, "send_data_task", 4096, NULL, 4, NULL);
}
