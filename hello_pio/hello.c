#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hello.pio.h"
#include <stdlib.h>
#include <limits.h>

#define SERIAL_PIN 16
#define LATCH_PIN 17
#define CLOCK_PIN 18
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

void sendByteToShiftRegister(char byte) {
    gpio_put(LATCH_PIN, 0);
    for (int x=0; x < 8; x++) {
        gpio_put(CLOCK_PIN, 0);
        gpio_put(SERIAL_PIN, !(byte & 1)); // 5161BS is active low, so invert LSB
        gpio_put(CLOCK_PIN, 1);
        byte = byte >> 1;
    }
    gpio_put(CLOCK_PIN, 0);
    gpio_put(LATCH_PIN, 1);
}

int main() {
    stdio_init_all();
    const int segments[] = {SEG_0, SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7, 
                            SEG_8, SEG_9, SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
    gpio_init(SERIAL_PIN);
    gpio_init(LATCH_PIN);
    gpio_init(CLOCK_PIN);
    gpio_init(BUTTON_PIN);

    gpio_set_dir(SERIAL_PIN, GPIO_OUT);
    gpio_set_dir(LATCH_PIN, GPIO_OUT);
    gpio_set_dir(CLOCK_PIN, GPIO_OUT);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);

    PIO pio = pio0;

    // hello.pio should have been compiled to a header file, we need to load this
    // into the given pio block, returns offset
    uint offset = pio_add_program(pio, &hello_program);

    // Get a free state machine and call our init function from hello.pio header
    uint sm = pio_claim_unused_sm(pio, true);
    uint pin_count = 4;
    hello_program_init(pio, sm, offset, 2, pin_count);

    int c = 0;
    bool running = true;
    bool old_button = false;
    int timer = 0;
    while (true) {
        bool button = gpio_get(BUTTON_PIN);
        if (gpio_get(BUTTON_PIN) && !old_button) { running = !running; }
        if (running && timer == 0) {
            // Hack to reverse output order, push data to the end of a 32 bit word which will then be reversed by PIO
            pio_sm_put_blocking(pio, sm, c << (sizeof(int) * CHAR_BIT - pin_count));
            sendByteToShiftRegister(segments[c]);
            c = (c + 1) % 16;
        }
        old_button = button;
        sleep_ms(10);
        timer = (timer + 10) % 500;
        
    }
}