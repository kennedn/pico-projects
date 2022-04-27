#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "led.h"
#include <time.h>


int main() {
    srand(time(NULL)); 
    stdio_init_all();
    uint8_t ledArray[] = {2, 3, 6, 7, 10};
    uint8_t reverseArray[] = {10, 7, 6, 3, 2};
    setup_pins(ledArray, sizeof(ledArray));
    bool dir = true;
    while(true) {
        dir = !dir;
        uint count = rand() % 50 + 10;
        for (uint i=0; i < count; i++) {
            toggle_pins(ledArray, sizeof(ledArray), dir);
        }
    }
}
