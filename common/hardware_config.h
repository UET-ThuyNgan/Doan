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
#define RELAY_PUMP1_PIN GPIO_NUM_25         //đây là pump tiếp nước nhó
#define RELAY_PUMP2_PIN GPIO_NUM_26         //đây là pump phun sương nhó
#define RELAY_LIGHT_PIN GPIO_NUM_19

// ===== Ngưỡng & cấu hình mặc định =====
#define DEFAULT_LUX_THRESHOLD     15.0f  // dưới ngưỡng thì bật đèn
#define LEVEL_HIGH_CM             5.0f    // đầy (khoảng cách ngắn)
#define LEVEL_LOW_CM              8.0f   // cạn (khoảng cách dài) -> bật bơm nạp
#define REFILL_MAX_RUNTIME_S      120     // chống kẹt bơm nạp


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