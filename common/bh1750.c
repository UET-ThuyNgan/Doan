// bh1750.c
#include "bh1750.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define TAG "BH1750"

#include "freertos/FreeRTOS.h"
#include "hardware_config.h"

esp_err_t i2c_master_write_to_device(i2c_port_t i2c_num, uint8_t device_address,
                                     const uint8_t *write_buffer, size_t write_size, TickType_t ticks_to_wait)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, write_buffer, write_size, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_master_read_from_device(i2c_port_t i2c_num, uint8_t device_address,
                                      uint8_t *read_buffer, size_t read_size, TickType_t ticks_to_wait)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, read_buffer, read_size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);
    return ret;
}


esp_err_t bh1750_init(bh1750_t *dev, i2c_port_t port, uint8_t address)
{
    dev->i2c_port = port;
    dev->address = address;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BH1750_SDA_GPIO, 
        .scl_io_num = BH1750_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    esp_err_t err = i2c_param_config(dev->i2c_port, &conf);
    if (err != ESP_OK) return err;

    return i2c_driver_install(dev->i2c_port, conf.mode, 0, 0, 0);
}

esp_err_t bh1750_set_mode(bh1750_t *dev, bh1750_mode_t mode)
{
    uint8_t cmd = (uint8_t)mode;

    return i2c_master_write_to_device(dev->i2c_port, dev->address, &cmd, 1, pdMS_TO_TICKS(100));
}

esp_err_t bh1750_read_light(bh1750_t *dev, float *lux)
{
    uint8_t data[2];
    esp_err_t err = i2c_master_read_from_device(dev->i2c_port, dev->address, data, 2, pdMS_TO_TICKS(200));
    if (err != ESP_OK) return err;

    uint16_t raw = (data[0] << 8) | data[1];
    *lux = raw / 1.2; // Tính theo datasheet

    return ESP_OK;
}
