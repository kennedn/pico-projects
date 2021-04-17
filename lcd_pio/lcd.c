#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "lcd.pio.h"
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define DOT 0b10110000
typedef enum {
    LEFT,
    CENTRE,
    RIGHT
} alignment;

// align string in 16 character space based on alignment
uint align(char *buffer, char *string, alignment alignment) {
    uint pad;
    int len = strlen(string);
    switch(alignment) {
        case CENTRE:
            pad = MAX(0, MIN(7, (16 - len) / 2));
            break;
        case RIGHT:
            pad = MAX(0, MIN(15, 16 - len));
            break;
        case LEFT:
        default:
            pad = 0;
            break;
    }
    // Prepend spaces to string based on pad
    sprintf(buffer, "%*s%s", pad, "", string);
}

// Write out a padded two line message to a 16 x 2 display
void lcd_write_blocking(PIO pio, uint sm, char *line_1, char *line_2, alignment align_1, alignment align_2) {
    //
    pio_sm_put_blocking(pio, sm, 0b000000010); // Return home (Cursor at position 0)
    if (!line_1 && !line_2) { return; }
    char line_buffer[40];

    align(line_buffer, line_1, align_1);
    snprintf(line_buffer, 41, "%-40s", line_buffer); // Pad row 1 to 40 chars because row 2 starts at pos 41
    // Write each character in string to 16x2 display
    for(int i = 0; i < strlen(line_buffer); i++) {
        pio_sm_put_blocking(pio, sm, 1u << 8 | line_buffer[i]); // register_select = 1 + data
    }

    align(line_buffer, line_2, align_2);
    snprintf(line_buffer, 17, "%-16s", line_buffer); // Pad string to 16 chars
    for(int i = 0; i < strlen(line_buffer); i++) {
        pio_sm_put_blocking(pio, sm, 1u << 8 | line_buffer[i]); // register_select = 1 + data
    }
}

/* Pin map:
|GPIO|LCD |FUNC        |
|----|--- |------------|
| 7  | 5  | Enable     |
| 8  | 4  | Read/Write |
| 9  | 7  | DB0        |
| 10 | 8  | DB1        |
| 11 | 9  | DB2        |
| 12 | 10 | DB3        |
| 13 | 11 | DB4        |
| 14 | 12 | DB5        |
| 15 | 13 | DB6        |
| 16 | 14 | DB7        |
| 17 | 6  | Reg Select |
*/
int main() {
    srand(time(0));
    // uint control_pins[] = {6, 7, 8};
    // uint data_pins[] = {9, 10, 11, 12, 13, 14 ,15 ,16};
    stdio_init_all();

    PIO pio = pio0;

    // lcd.pio should have been compiled to a header file, we need to load this
    // into the given pio block, returns offset
    uint offset = pio_add_program(pio, &lcd_program);

    // Get a free state machine and call our init function from lcd.pio header
    uint sm = pio_claim_unused_sm(pio, true);
    lcd_program_init(pio, sm, offset, 7, 0.01);
    pio_sm_put_blocking(pio, sm, 0b000111000); // 8 bit operation, 2 line mode, 5 x 8 font
    printf("Set mode\n");
    pio_sm_put_blocking(pio, sm, 0b000001100); // Display on, cursor on, cursor blink
    printf("Set cursor\n");
    pio_sm_put_blocking(pio, sm, 0b000000110); // Set cursor right shift
    printf("Set shift\n");
    pio_sm_put_blocking(pio, sm, 0b000000001); // Clear display and reset counter to 0
    printf("Cleared display\n");

    char *words[10] = {"Bepzink", "Roblox", "Wholesome", "Bungo", "Mungus", "Hugh", "Tummy rumbles", "Monke", "Rapscallion", "Beefaroni"};
    char c = 0;
    while(true) {
        lcd_write_blocking(pio, sm, words[rand() % 10], words[rand() % 10], rand() % 3, rand() % 3);
        sleep_ms(500);
    }
}