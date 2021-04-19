#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "lcd.pio.h"
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void seed_random_from_rosc()
{
  uint32_t random = 0;
  uint32_t random_bit;
  volatile uint32_t *rnd_reg = (uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);

  for (int k = 0; k < 32; k++) {
    while (1) {
      random_bit = (*rnd_reg) & 1;
      if (random_bit != ((*rnd_reg) & 1)) break;
    }

    random = (random << 1) | random_bit;
  }

  srand(random);
}

typedef enum {
    DATA_MASK = 0b100000000,
    CGRAM_MASK = 0b001000000,
    DDRAM_MASK = 0b010000000,

    RETURN_HOME = 0b000000010,
    SHIFT_LEFT = 0b000011000,
    SHIFT_RIGHT = 0b000011100,
    FUNC_SET = 0b000111000,   // 8 bit operation, 2 line mode, 5 x 8 font
    CURSOR_SET = 0b000001100, // Display on, cursor on, cursor blink
    SHIFT_SET =  0b000000110  // Set cursor right shift
} commands;

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
    if (buffer) {
        // Prepend spaces to string based on pad
        sprintf(buffer, "%*s%s", pad, "", string);
    } else {
        return pad;
    }
}

// Write out a padded two line message to a 16 x 2 display
void lcd_write_blocking(PIO pio, uint sm, char *line_1, char *line_2, alignment align_1, alignment align_2) {
    //
    pio_sm_put_blocking(pio, sm, RETURN_HOME); // Return home (Cursor at position 0)
    if (!line_1 && !line_2) { return; }
    char line_buffer[40];

    align(line_buffer, line_1, align_1);
    snprintf(line_buffer, 41, "%-40s", line_buffer); // Pad row 1 to 40 chars because row 2 starts at pos 41
    // Write each character in string to 16x2 display
    for(int i = 0; i < strlen(line_buffer); i++) {
        pio_sm_put_blocking(pio, sm, DATA_MASK | line_buffer[i]); // register_select = 1 + data
    }

    align(line_buffer, line_2, align_2);
    snprintf(line_buffer, 41, "%-40s", line_buffer); // Pad string to 40 chars too
    for(int i = 0; i < strlen(line_buffer); i++) {
        pio_sm_put_blocking(pio, sm, DATA_MASK | line_buffer[i]); // register_select = 1 + data
    }
}

// Write an icon set to CGRAM and then display each custom icon
void next_frame(PIO pio, uint sm, uint icons, uint lines, uint icon_array[][lines], uint line_2_break, uint wait_ms) {
    pio_sm_put_blocking(pio, sm, CGRAM_MASK | 8 ); // Point AC at CGRAM slot 1 (character generator RAM)
    
    // Write custom characters to LCD module, AC auto increments on each write
    for (int i = 0; i < icons; i++) {
        for (int j = 0; j < lines; j ++) {
            pio_sm_put_blocking(pio, sm, DATA_MASK| icon_array[i][j]); // write each line of 5x8 character
        }
    }
    pio_sm_put_blocking(pio, sm, DDRAM_MASK | 6); // Set AC to DDRAM character 6 (middle screen line 1)
    for (int i = 0; i < icons; i++) {
        if (line_2_break && i == line_2_break) { pio_sm_put_blocking(pio, sm, DDRAM_MASK | 46); } // Set AC to DDRAM character 46 (middle screen line 2)
        pio_sm_put_blocking(pio, sm, DATA_MASK| i + 1); // write custom icon
    }

    sleep_ms(wait_ms);
}

/* Pin map:
|GPIO|LCD |FUNC        |
|----|--- |------------|
| 7  | 6  | Enable     |
| 8  | 5  | Read/Write |
| 9  | 7  | DB0        |
| 10 | 8  | DB1        |
| 11 | 9  | DB2        |
| 12 | 10 | DB3        |
| 13 | 11 | DB4        |
| 14 | 12 | DB5        |
| 15 | 13 | DB6        |
| 16 | 14 | DB7        |
| 17 | 4  | Reg Select |
*/

int main() {
    sleep_ms(15);
    seed_random_from_rosc();
    stdio_init_all();

    PIO pio = pio0;

    // lcd.pio should have been compiled to a header file, we need to load this
    // into the given pio block, returns offset
    uint offset = pio_add_program(pio, &lcd_program);

    // Get a free state machine and call our init function from lcd.pio header
    uint sm = pio_claim_unused_sm(pio, true);
    lcd_program_init(pio, sm, offset, 7, 1);


    // Initalise display
    pio_sm_put_blocking(pio, sm, FUNC_SET); 
    pio_sm_put_blocking(pio, sm, CURSOR_SET);
    pio_sm_put_blocking(pio, sm, SHIFT_SET);
    lcd_write_blocking(pio, sm, " ", " ", LEFT, LEFT); // Clear display
    pio_sm_put_blocking(pio, sm, RETURN_HOME);

    // Animation frame icon sets
    uint coffee_f1[5][8] = {{0x0, 0x0, 0x0, 0x2, 0x2, 0x1, 0x1, 0x0},         // top left
                            {0x0, 0x0, 0x0, 0x14, 0x14, 0xa, 0xa, 0x0},       // top centre
                            {0x7, 0x7, 0x7, 0x7, 0x7, 0x3, 0x1, 0x0},         // bottom left
                            {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1e, 0x1c, 0x0},  // bottom centre
                            {0x18, 0x4, 0x4, 0x18, 0x0, 0x0, 0x0, 0x0}};      // bottom right

    uint coffee_f2[2][8] = {{0x0, 0x0, 0x0, 0x1, 0x1, 0x1, 0x1, 0x0},         // top left
                                {0x0, 0x0, 0x0, 0xa, 0xa, 0xa, 0xa, 0x0}};      // top centre

    uint coffee_f3[2][8] = {{0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x0},         // top left
                            {0x0, 0x0, 0x0, 0x15, 0x15, 0xa, 0xa, 0x0}};      // top centre

    while (true) {
        next_frame(pio, sm, 5, 8, coffee_f1, 2, 300);
        next_frame(pio, sm, 2, 8, coffee_f2, 0, 300);
        next_frame(pio, sm, 2, 8, coffee_f3, 0, 300);
        next_frame(pio, sm, 2, 8, coffee_f2, 0, 300);
    }
}
