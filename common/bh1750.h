#ifndef __BH1750_H
#define __BH1750_H

#include "driver/i2c.h"
#include "esp_err.h"

#define BH1750_I2C_ADDR_LO  0x23
#define BH1750_I2C_ADDR_HI  0x5C

typedef enum {
    BH1750_MODE_CONTINUOUS_HIGH_RES = 0x10,
    BH1750_MODE_CONTINUOUS_HIGH_RES2 = 0x11,
    BH1750_MODE_CONTINUOUS_LOW_RES = 0x13,
    BH1750_MODE_ONE_TIME_HIGH_RES = 0x20,
    BH1750_MODE_ONE_TIME_HIGH_RES2 = 0x21,
    BH1750_MODE_ONE_TIME_LOW_RES = 0x23
} bh1750_mode_t;

typedef struct {
    i2c_port_t i2c_port;
    uint8_t address;
} bh1750_t;

esp_err_t bh1750_init(bh1750_t *dev, i2c_port_t port, uint8_t address);
esp_err_t bh1750_set_mode(bh1750_t *dev, bh1750_mode_t mode);
esp_err_t bh1750_read_light(bh1750_t *dev, float *lux);

#endif