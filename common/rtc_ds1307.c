#include "rtc_ds1307.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "DS1307";

static uint8_t bcd2dec(uint8_t val) {
    return (val / 16 * 10) + (val % 16);
}

static uint8_t dec2bcd(uint8_t val) {
    return (val / 10 * 16) + (val % 10);
}

static bool ds1307_is_running(i2c_port_t i2c_num) {
    uint8_t sec_reg;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);  // register seconds
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &sec_reg, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    // Bit 7 = CH (clock halt) -> 0: running, 1: stopped
    return ((sec_reg & 0x80) == 0);
}

esp_err_t ds1307_get_time(i2c_port_t i2c_num, struct tm *timeinfo) {
    uint8_t data[7];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 7, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) return ret;

    timeinfo->tm_sec  = bcd2dec(data[0] & 0x7F);
    timeinfo->tm_min  = bcd2dec(data[1]);
    timeinfo->tm_hour = bcd2dec(data[2] & 0x3F);
    timeinfo->tm_mday = bcd2dec(data[4]);
    timeinfo->tm_mon  = bcd2dec(data[5]) - 1;
    timeinfo->tm_year = bcd2dec(data[6]) + 100;

    return ESP_OK;
}

esp_err_t ds1307_set_time(i2c_port_t i2c_num, const struct tm *timeinfo) {
    uint8_t data[8];
    data[0] = 0x00; // starting register
    data[1] = dec2bcd(timeinfo->tm_sec & 0x7F); // ensure CH=0
    data[2] = dec2bcd(timeinfo->tm_min);
    data[3] = dec2bcd(timeinfo->tm_hour);
    data[4] = dec2bcd(timeinfo->tm_wday + 1);
    data[5] = dec2bcd(timeinfo->tm_mday);
    data[6] = dec2bcd(timeinfo->tm_mon + 1);
    data[7] = dec2bcd(timeinfo->tm_year - 100);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS1307_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, 8, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t ds1307_init(i2c_port_t i2c_num) {
    ESP_LOGI(TAG, "Initializing DS1307...");
    // chỉ kiểm tra hoạt động, không tự set build time nữa
    if (!ds1307_is_running(i2c_num)) {
        ESP_LOGW(TAG, "RTC not running, please set time manually!");
    } else {
        ESP_LOGI(TAG, "RTC already running, keeping current time.");
    }
    return ESP_OK;
}

void ds1307_print_time(struct tm *time)
{
    printf("RTC time: %02d/%02d/%04d %02d:%02d:%02d\n",
           time->tm_mday, time->tm_mon + 1, time->tm_year + 1900,
           time->tm_hour, time->tm_min, time->tm_sec);
}

