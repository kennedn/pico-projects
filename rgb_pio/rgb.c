#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "rgb.pio.h"
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#define BRIGHT 6
#define GREEN 7
#define BLUE 8
#define RED 9
#define BUTTON_PIN 15

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


static uint reverse_lookup[] = { 0x0, 0x4, 0x2, 0x6, 0x1, 0x5, 0x3, 0x7 };

uint create_mask(uint bright, uint rgb) {
    // return (bright) | // Map bright bit to pin position 0
    //        (((rgb >> 1) & 1) << 2) | // Map green bit in position 1 to pin position 3  
    //        (((rgb >> 2) & 1) << 3) | // Map red bit in position 2 to pin position 1
    //        ((rgb & 1) << 1); // Map blue bit in position 0 to pin position 2
    return bright | (rgb << 1);
}

int main() {
    stdio_init_all();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, false);

    PIO pio = pio0;

    // rgb.pio should have been compiled to a header file, we need to load this
    // into the given pio block, returns offset
    uint offset = pio_add_program(pio, &rgb_program);

    // Get a free state machine and call our init function from rgb.pio header
    uint sm = pio_claim_unused_sm(pio, true);
    rgb_program_init(pio, sm, offset, BRIGHT, GREEN, BLUE, RED);

    uint i = 0;
    uint rgb_mask = 1;
    bool old_button = false;
    while (true) {
        bool button = gpio_get(BUTTON_PIN);
        if (gpio_get(BUTTON_PIN) && !old_button) { 
            pio_sm_put_blocking(pio, sm, create_mask(1, rgb_mask));
            printf("mask: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(create_mask(1, rgb_mask)));
            printf(",rgb_raw: "BYTE_TO_BINARY_PATTERN"\r", BYTE_TO_BINARY(rgb_mask));
            rgb_mask = (rgb_mask + 1) % 8;
            // rgb_mask = ((rgb_mask << 2) % 8) | (rgb_mask >> 1);
        }
        old_button = button;
        sleep_ms(10);
        
    }
}