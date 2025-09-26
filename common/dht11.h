#ifndef DHT11_H
#define DHT11_H

typedef struct {
    int temperature;
    int humidity;
} dht11_data_t;

int read_dht11(dht11_data_t *data);

#endif      //DHT11_H
