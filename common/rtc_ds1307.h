#ifndef RTC_DS1307_H
#define RTC_DS1307_H

#include "driver/i2c.h"
#include <time.h>

#define DS1307_ADDR 0x68

void ds1307_print_time(struct tm *time);

esp_err_t ds1307_init(i2c_port_t i2c_num);
esp_err_t ds1307_get_time(i2c_port_t i2c_num, struct tm *timeinfo);
esp_err_t ds1307_set_time(i2c_port_t i2c_num, const struct tm *timeinfo);

#endif


/*
//set lại giờ cho rtc nếu sai giờ nhéeee
    
    bh1750_t dev;
    float lux;
    struct tm now; 

    // Khởi tạo BH1750
    ESP_ERROR_CHECK(bh1750_init(&dev, I2C_NUM_0, BH1750_I2C_ADDR_LO));
    ESP_ERROR_CHECK(bh1750_set_mode(&dev, BH1750_MODE_CONTINUOUS_HIGH_RES));

    ESP_ERROR_CHECK(ds1307_init(I2C_NUM_0));
    struct tm timeinfo = {
    .tm_year = 2025 - 1900,  // năm tính từ 1900
    .tm_mon  = 9,            // tháng (0 = Jan, nên 9 = October)
    .tm_mday = 11,           // ngày
    .tm_hour = 3,
    .tm_min  = 43,
    .tm_sec  = 0
    };
    ESP_ERROR_CHECK(ds1307_set_time(I2C_NUM_0, &timeinfo));

*/