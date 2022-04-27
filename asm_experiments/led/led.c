#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "led.h"
#include <time.h>


int main() {
    srand(time(NULL)); 
    stdio_init_all();
    uint32_t ledArray[] = {2, 3, 6, 7};
    uint32_t reverseArray[] = {7, 6, 3, 2};
    setup_pins(ledArray, count_of(ledArray));
    bool dir = true;
    while(true) {
        dir = !dir;
        uint count = rand() % 50 + 10;
        for (uint i=0; i < count; i++) {
            if (dir) {
                toggle_pins(ledArray, count_of(ledArray));
            } else {
                toggle_pins(reverseArray, count_of(reverseArray));
            }
        }
    }
}
