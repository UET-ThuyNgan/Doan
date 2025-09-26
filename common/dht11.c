#include "dht11.h"
#include "driver/gpio.h"
#include "esp32/rom/ets_sys.h"
#include "esp_log.h"
#include "hardware_config.h"

#define DHT_TAG "DHT11"

int read_dht11(dht11_data_t *data)
{
    uint8_t bits[5] = {0};

    // Gửi tín hiệu START
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 0);
    ets_delay_us(18000); // giữ LOW 18ms
    gpio_set_level(DHT11_GPIO, 1);
    ets_delay_us(20); // nhả ra HIGH 20us

    // Chuyển sang INPUT để chờ cảm biến phản hồi
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);

    // Chờ phản hồi từ cảm biến (LOW khoảng 80us)
    int timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 1) {
        if (++timeout > 100) {
            ESP_LOGE(DHT_TAG, "Timeout waiting for response (LOW start)");
            return ESP_FAIL;
        }
        ets_delay_us(1);
    }

    timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 0) {
        if (++timeout > 100) {
            ESP_LOGE(DHT_TAG, "Timeout during response LOW");
            return ESP_FAIL;
        }
        ets_delay_us(1);
    }

    // Cảm biến kéo HIGH khoảng 80us
    timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 1) {
        if (++timeout > 100) {
            ESP_LOGE(DHT_TAG, "Timeout during response HIGH");
            return ESP_FAIL;
        }
        ets_delay_us(1);
    }

    // Đọc 40 bit
    for (int i = 0; i < 40; i++) {
        // Chờ LOW (bắt đầu 1 bit)
        timeout = 0;
        while (gpio_get_level(DHT11_GPIO) == 0) {
            if (++timeout > 100) {
                ESP_LOGE(DHT_TAG, "Timeout waiting for bit start LOW");
                return ESP_FAIL;
            }
            ets_delay_us(1);
        }

        // Bắt đầu đo độ dài của HIGH (bit 0 hay 1)
        timeout = 0;
        while (gpio_get_level(DHT11_GPIO) == 1) {
            ets_delay_us(1);
            timeout++;
            if (timeout > 100) {
                ESP_LOGE(DHT_TAG, "Timeout during bit HIGH");
                return ESP_FAIL;
            }
        }

        // Nếu HIGH dài hơn ~40us → bit = 1, ngược lại bit = 0
        int bit_value = (timeout > 40) ? 1 : 0;
        bits[i / 8] <<= 1;
        bits[i / 8] |= bit_value;
    }

    // Check checksum
    uint8_t checksum = bits[0] + bits[1] + bits[2] + bits[3];
    if (checksum != bits[4]) {
        ESP_LOGE(DHT_TAG, "Checksum failed! Got: %02X, expected: %02X",
                 bits[4], checksum);
        return ESP_FAIL;
    }

    // Gán giá trị nhiệt độ và độ ẩm
    data->humidity = bits[0];
    data->temperature = bits[2];

    ESP_LOGI(DHT_TAG, "Temp: %d°C, Humi: %d%%",
             data->temperature, data->humidity);

    return ESP_OK;
}
