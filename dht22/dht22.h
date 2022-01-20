#pragma once
#include "hardware/pio.h"
#include "pico/stdlib.h"
typedef struct {
    float temperature;
    float humidity;
    uint8_t status;
} DHTReading;

bool dht_get(DHTReading *reading_buffer);
void dht_init();