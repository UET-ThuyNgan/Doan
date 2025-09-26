#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#include <driver/gpio.h>

//Cảm biến khoảng cách - srf05
#define SRF_TRIG_GPIO GPIO_NUM_4
#define SRF_ECHO_GPIO GPIO_NUM_18

//Cảm biến ánh sáng - bh1750
#define BH1750_SDA_GPIO GPIO_NUM_21
#define BH1750_SCL_GPIO GPIO_NUM_22
#define I2C_PORT_USED I2C_NUM_0
#define I2C_FREQ_HZ   100000


//Cảm biến nhiệt độ, độ ẩm - dht11
#define DHT11_GPIO GPIO_NUM_23

// Relay
#define RELAY_PUMP1_PIN GPIO_NUM_25
#define RELAY_PUMP2_PIN GPIO_NUM_26
#define RELAY_LIGHT_PIN GPIO_NUM_19

// ===== Ngưỡng & cấu hình mặc định =====
#define DEFAULT_LUX_THRESHOLD     20.0f  // dưới ngưỡng thì bật đèn
#define LEVEL_HIGH_CM             8.0f    // đầy (khoảng cách ngắn)
#define LEVEL_LOW_CM              18.0f   // cạn (khoảng cách dài) -> bật bơm nạp
#define REFILL_MAX_RUNTIME_S      120     // chống kẹt bơm nạp


/*
// === Analog sensors (ADC1)
#define TDS_ADC_GPIO      GPIO_NUM_34
#define PH_ADC_GPIO       GPIO_NUM_35


Ghi chú cắm dây nhanh

BH1750: VCC(3V3), GND, SDA→21, SCL→22; nhớ pull-up 4.7k–10k trên SDA/SCL nếu module không có sẵn.

SRF05: VCC(5V), GND; TRIG→4, ECHO→5 (nếu ECHO 5V, dùng chia áp về ~3.3V!)

DHT11: VCC(3V3), GND, DATA→23 + pull-up ~10k.

TDS/pH: OUT analog → 34/35; cấp nguồn theo module (thường 5V cho mạch đo, ngõ ra vẫn 0–3.3V).

Pump/Relay: GPIO25/26/27/33 → module relay (có opto, transistor); đừng kéo tải AC/DC trực tiếp từ GPIO.
*/

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


#endif      //HW_CONFIG_H