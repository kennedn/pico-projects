#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "nec.pio.h"
// Macros to generate the lookup table (at compile-time)
#define R2(n) n, n + 2*64, n + 1*64, n + 3*64
#define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
#define REVERSE_BITS R6(0), R6(2), R6(1), R6(3)
 
// lookup table to store the reverse of each index of the table.
// The macro `REVERSE_BITS` generates the table
unsigned int lookup[256] = { REVERSE_BITS };
 
// Function to reverse bits of `n` using a lookup table
int reverseBits(int n) {
    return lookup[n & 0xff] << 24 |                // consider the first 8 bits
           lookup[(n >> 8) & 0xff] << 16 |         // consider the next 8 bits
           lookup[(n >> 16) & 0xff] << 8 |         // consider the next 8 bits
           lookup[(n >> 24) & 0xff];               // consider last 8 bits
}

int main() {
    stdio_init_all();
    PIO pio = pio0;
    uint nec_offset = pio_add_program(pio, &nec_program);
    nec_program_init(pio, 0 , nec_offset, 15);
    while (true) {
        // printf("hello");
        // printf("Normal:%d\nReversed:%d\n",0x00ff02fd, reverseBits(0x00ff02fd));
        // pio_sm_put_blocking(pio, 0, 0xBF40FF00);
        // uint value;
        // scanf("%x", &value);
        pio_sm_put_blocking(pio, 0, reverseBits(0xff1ae5)); // Red
        sleep_ms(1000);
        pio_sm_put_blocking(pio, 0, reverseBits(0xff9a65)); // Green
        sleep_ms(1000);
        pio_sm_put_blocking(pio, 0, reverseBits(0xffa25d)); // Blue
        sleep_ms(1000);
        pio_sm_put_blocking(pio, 0, reverseBits(0xff22dd)); // White
        sleep_ms(1000);
    }
}
