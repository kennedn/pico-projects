#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "nec.pio.h"
// Macros to generate the lookup table (at compile-time)
#define R2(n) n, n + 2*64, n + 1*64, n + 3*64
#define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
#define REVERSE_BITS R6(0), R6(2), R6(1), R6(3)
 
#define LED PICO_DEFAULT_LED_PIN
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
    nec_program_init(pio, 0 , nec_offset, 15, true);
    nec_program_init(pio, 1 , nec_offset, LED, false);

    getchar();
    char hex_string[9];
    int i, chr, hex_code;
    while (true) {
        i = 0;
        chr = 0;
        hex_code = 0;
        while ((chr = getchar()) != '\r' && chr != EOF) {
            if (i >= count_of(hex_string)) { break; }
            hex_string[i++] = chr;
        }

        if (i >= count_of(hex_string)) { continue; }
        
        hex_string[i] = '\0';
        if ((hex_code = (int)strtol(hex_string, NULL, 16))) {
            printf("OK\r\n"); 
            pio_sm_put_blocking(pio, 0, reverseBits(hex_code));
            pio_sm_put_blocking(pio, 1, reverseBits(hex_code));
        } else { printf("NG\r\n"); }
    }
}
