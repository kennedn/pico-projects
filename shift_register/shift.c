#include "pico/stdlib.h"
#define SER_PIN 16
#define LATCH_PIN 17
#define CLOCK_PIN 18
#define BUTTON_PIN 15

// Byte definitions for 7 segment display
#define SEG_0 0b11111100 
#define SEG_1 0b01100000 
#define SEG_2 0b11011010 
#define SEG_3 0b11110010 
#define SEG_4 0b01100110 
#define SEG_5 0b10110110 
#define SEG_6 0b10111110 
#define SEG_7 0b11100000 
#define SEG_8 0b11111110 
#define SEG_9 0b11110110 
#define SEG_A 0b11101110 
#define SEG_B 0b00111110 
#define SEG_C 0b10011100 
#define SEG_D 0b01111010 
#define SEG_E 0b10011110 
#define SEG_F 0b10001110 
#define SEG_P 0b11001110 
#define SEG_DOT 0b00000001

// Bit bang a byte to an 8 bit shift register
void shiftOut(char b) {
    gpio_put(LATCH_PIN, 0); // Latch low
    // Clock pulse 8 bits on to shift register
    for (int x=0; x < 8; x++) {
        gpio_put(CLOCK_PIN, 0); // Clock low
        // Push least significant bit to serial pin, invert because 5161BS is active low
        gpio_put(SER_PIN, !(b & 1));
        gpio_put(CLOCK_PIN, 1); // Clock high
        b = b >> 1; // Bit shift byte 1 bit to the right
    }
    gpio_put(LATCH_PIN, 1); // Latch high, pushing to output pins on register
}

int main() {
    const int segments_size = 16;
    const int segments[] = {SEG_0, SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7, 
                            SEG_8, SEG_9, SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F};
    gpio_init(SER_PIN);
    gpio_init(LATCH_PIN);
    gpio_init(CLOCK_PIN);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(SER_PIN, GPIO_OUT);
    gpio_set_dir(LATCH_PIN, GPIO_OUT);
    gpio_set_dir(CLOCK_PIN, GPIO_OUT);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN);

    int counter = 0;
    bool lastButton = false;
    bool isMoving = true;
    int ms = 0;

    while (true) {
        // If button was just pressed (not held), flip isMoving
        bool button = gpio_get(BUTTON_PIN);
        if (button && !lastButton) {
            isMoving = !isMoving;
        } 

        // isMoving and ms is back to 0 (300 ms have passed)
        if (isMoving && !ms) {
            // write a byte to shift register, incrementing counter 
            shiftOut(segments[counter++]);
            // Ensure counter wraps back around once it reaches end of array
            counter = counter % segments_size;
        } else if (!ms) {
            // Not moving so just write current byte to shift register, xoring a dot to the write too
            shiftOut(segments[counter] | SEG_DOT);
        }

        // sleep 10 and increment a counter, this allows button input to be read every 10ms
        // but lets us write out the the shift register more infrequently (every 300ms)
        sleep_ms(10);
        ms = (ms + 10) % 300;
        // Track last button state
        lastButton = button;
    }
}
