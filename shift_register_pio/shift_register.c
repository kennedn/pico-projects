#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "shift_register.pio.h"
#include <stdlib.h>
#define SER 16
#define RCLK 17
#define SRCLK 18
#define BUTTON_PIN 15

// Byte definitions for 7 segment display
#define SEG_0   0b11111100 
#define SEG_1   0b01100000 
#define SEG_2   0b11011010 
#define SEG_3   0b11110010 
#define SEG_4   0b01100110 
#define SEG_5   0b10110110 
#define SEG_6   0b10111110 
#define SEG_7   0b11100000 
#define SEG_8   0b11111110 
#define SEG_9   0b11110110 
#define SEG_A   0b11101110 
#define SEG_B   0b00111110 
#define SEG_C   0b10011100 
#define SEG_D   0b01111010 
#define SEG_E   0b10011110 
#define SEG_F   0b10001110 
#define SEG_P   0b11001110 
#define SEG_DOT 0b00000001

int main() {
    stdio_init_all();
    const int segments[] = {SEG_0, SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7, 
                            SEG_8, SEG_9, SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};

    PIO pio = pio0;

    // insert pio header into given pio's memory, returns offset, name is dictacted by .program directive in .pio file
    uint offset = pio_add_program(pio, &shift_register_program);

    // Get a free state machine and call our init function from injected header
    uint sm = pio_claim_unused_sm(pio, true);
    shift_register_program_init(pio, sm, offset, SER, RCLK, 3, 10);

    uint c = 0;
    while (true) {
        pio_sm_put_blocking(pio, sm, segments[c]);
        sleep_ms(1000);
        c = (c + 1) % 16;
    }
    
}