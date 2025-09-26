#ifndef HYSRF05_H
#define HYSRF05_H

#include <driver/gpio.h>

void hysrf05_init(void);
float hysrf05_read_distance(void); // đơn vị: cm

#endif