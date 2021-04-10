#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "shift.pio.h"
#include "hello.pio.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#define SER 16
#define RCLK 17
#define SRCLK 18
#define BUTTON 15

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
#define SEG_NULL 0b00000000
#define SEG_ALL 0b11111111

// uint reverse(uint num, uint count)
// {
//     uint reverse_num = num;
//     while(count)
//     {
//        count--;
//        reverse_num <<= 1;       
//        reverse_num |= num & 1;
//        num >>= 1;
//     }
//     return reverse_num;
// }
  


int main() {
    stdio_init_all();
    const int segments[] = {SEG_ALL, SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7, 
                            SEG_8, SEG_9, SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
    const int shift_pins[] = {SER, RCLK, SRCLK};
    const int hello_pins[] = {2, 3, 4, 5};

    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, false);



    PIO pio = pio0;

    // insert pio header into given pio's memory, returns offset, name is dictacted by .program directive in .pio file
    uint shift_offset = pio_add_program(pio, &shift_program);
    uint hello_offset = pio_add_program(pio, &hello_program);
    // Get a free state machine and call our init function from injected header
    uint shift_sm = pio_claim_unused_sm(pio, true);
    uint hello_sm = pio_claim_unused_sm(pio, true);
    shift_program_init(pio, shift_sm, shift_offset, SER, RCLK,  1);
    hello_program_init(pio, hello_sm, hello_offset, 2, count_of(hello_pins));
    // insert pio header into given pio's memory, returns offset, name is dictacted by .program directive in .pio file
    // Get a free state machine and call our init function from injected header
    int timer = 0;
    bool direction = false;
    uint c = 2;
    while (true) {
        bool button = gpio_get(BUTTON);
        if (button) {break;}

        if (timer == 0) { 
            pio_sm_put_blocking(pio, hello_sm, c << sizeof(uint) * CHAR_BIT - count_of(hello_pins));
            pio_sm_put_blocking(pio, shift_sm, (c & 2 || c & 4) ? SEG_DOT : SEG_NULL);

            if (direction) {
                c = (c >> 3) | ((c << 1) % 16);
            } else {
                c = (c >> 1) | ((c << 3) % 16);
            }

            if(c & 2 || c & 4) { direction = !direction;}
        }

        sleep_ms(10);
        timer = (timer + 10) % 250;
        
    }
    bool lastButton = false;
    c = 0;
    while (true) {
        bool button = gpio_get(BUTTON);
        if (button && !lastButton) { 
            c = (c + 1) % 16;
            pio_sm_put_blocking(pio, shift_sm, segments[c]);
            //pio_sm_put_blocking(pio, hello_sm, reverse(c, count_of(hello_pins)));
            pio_sm_put_blocking(pio, hello_sm, c << sizeof(uint) * CHAR_BIT - count_of(hello_pins));
        }
        lastButton = button;
        sleep_ms(10);
    }
    
}