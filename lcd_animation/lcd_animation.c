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
    snprintf(line_buffer, 41, "%-40s", line_buffer); // Pad string to 16 chars
    for(int i = 0; i < strlen(line_buffer); i++) {
        pio_sm_put_blocking(pio, sm, 1u << 8 | line_buffer[i]); // register_select = 1 + data
    }
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
    pio_sm_put_blocking(pio, sm, 0b000111000); // 8 bit operation, 2 line mode, 5 x 8 font
    pio_sm_put_blocking(pio, sm, 0b000001100); // Display on, cursor on, cursor blink
    pio_sm_put_blocking(pio, sm, 0b000000110); // Set cursor right shift
    lcd_write_blocking(pio, sm, " ", " ", LEFT, LEFT); // Clear display
    pio_sm_put_blocking(pio, sm, 0b000000010); // Return Home

    while (true) {

        // Starting at first CG offset will cause character 1 to be accessable at \0, which causes issues for string functons
        // Quick hack is to lose a character and just start at offset 0x48 (\1)
        // const uint patterns[4][8] = {{0xe, 0x10, 0x10, 0x1f, 0x1b, 0x1b, 0x1f, 0x0},  // Unlock
        //                     {0xe, 0x11, 0x11, 0x1f, 0x1b, 0x1b, 0x1f, 0x0},  // Lock
        //                     {0x0, 0xa, 0x1f, 0x1f, 0xe, 0x4, 0x0, 0x0},      // Heart
        //                     {0x1f, 0x15, 0x1f, 0x1f, 0xe, 0xa, 0x1b, 0x0}};    // Guy
        const uint coffee_f1[5][8] = {{0, 0x2, 0x2, 0x1, 0x1, 0, 0x7, 0x5},             // top left
                                    {0x0, 0x14, 0x14, 0xa, 0xa, 0x0, 0x1f, 0x1f},       // top centre
                                    {0x5, 0x5, 0x5, 0x6, 0x7, 0x3, 0x1, 0x0},          // bottom left
                                    {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1e, 0x1c, 0x0},    // bottom centre
                                    {0x18, 0x4, 0x4, 0x18, 0x0, 0x0, 0x0, 0x0}};        // bottom right
        
        const uint coffee_f2[2][8] = {{0x0, 0x1, 0x1, 0x1, 0x1, 0x0, 0x7, 0x5},         // top left
                                      {0x0, 0xa, 0xa, 0xa, 0xa, 0x0, 0x1f, 0x1f}};      // top centre

        const uint coffee_f3[2][8] = {{0x0, 0x0, 0x0, 0x1, 0x1, 0x0, 0x7, 0x5},         // top left
                                      {0x0, 0x15, 0x15, 0xa, 0xa, 0x0, 0x1f, 0x1f}};      // top centre
        pio_sm_put_blocking(pio, sm, 0x48); // Point AC at CGRAM (character generator RAM)
        // Write custom characters to LCD module, AC auto increments on each write
        for (int i = 0; i < count_of(coffee_f1); i++) {
            for (int j = 0; j < count_of(coffee_f1[i]); j ++) {
                pio_sm_put_blocking(pio, sm, 1u << 8 | coffee_f1[i][j]); // write each line of 5x8 character
            }
        }

        pio_sm_put_blocking(pio, sm, 0x80); 
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\1');
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\2'); 
        pio_sm_put_blocking(pio, sm, 0xa8); 
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\3');
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\4'); 
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\5'); 

        sleep_ms(150);

        pio_sm_put_blocking(pio, sm, 0x48); // Point AC at CGRAM (character generator RAM)
        // Write custom characters to LCD module, AC auto increments on each write
        for (int i = 0; i < count_of(coffee_f2); i++) {
            for (int j = 0; j < count_of(coffee_f2[i]); j ++) {
                pio_sm_put_blocking(pio, sm, 1u << 8 | coffee_f2[i][j]); // write each line of 5x8 character
            }
        }

        pio_sm_put_blocking(pio, sm, 0x80); 
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\1');
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\2'); 

        sleep_ms(300);

        pio_sm_put_blocking(pio, sm, 0x48); // Point AC at CGRAM (character generator RAM)
        // Write custom characters to LCD module, AC auto increments on each write
        for (int i = 0; i < count_of(coffee_f3); i++) {
            for (int j = 0; j < count_of(coffee_f3[i]); j ++) {
                pio_sm_put_blocking(pio, sm, 1u << 8 | coffee_f3[i][j]); // write each line of 5x8 character
            }
        }

        pio_sm_put_blocking(pio, sm, 0x80); 
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\1');
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\2'); 

        sleep_ms(300);

        pio_sm_put_blocking(pio, sm, 0x48); // Point AC at CGRAM (character generator RAM)
        // Write custom characters to LCD module, AC auto increments on each write
        for (int i = 0; i < count_of(coffee_f2); i++) {
            for (int j = 0; j < count_of(coffee_f2[i]); j ++) {
                pio_sm_put_blocking(pio, sm, 1u << 8 | coffee_f2[i][j]); // write each line of 5x8 character
            }
        }

        pio_sm_put_blocking(pio, sm, 0x80); 
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\1');
        pio_sm_put_blocking(pio, sm, 1u << 8 | '\2'); 

        sleep_ms(300);

    }

    // char *words[] = {"Bepzinky", "Roblox", "Wholesome", "Bungo", "Mungus", "Hugh", "Tummy rumbles", "Monke", "Rapscallion", "Beefaroni"};
    // char *customs[] = {"\1", "\2", "\3", "\4"};
    // char line_1[40], line_2[40];

    // char c = 0;
    // uint timer = 0;
    // uint icon = 0;
    // uint direction = 0;
    // uint max_length = 0;

    // while(true) {
    //     icon = rand() % count_of(customs);  // Pick a random icon for line
    //     // Concat an icon on either side of random word
    //     snprintf(line_1, 41, "%s %s %s", customs[icon], words[rand() % count_of(words)], customs[icon]); 

    //     icon = rand() % count_of(customs);
    //     snprintf(line_2, 41, "%s %s %s", customs[icon], words[rand() % count_of(words)], customs[icon]);

    //     direction = !direction;
    //     // write lines to centre stage on LCD
    //     lcd_write_blocking(pio, sm, line_1, line_2, CENTRE, CENTRE);
    //     // Find biggest string length in two lines
    //     uint line_1_pad = (direction) ? align(NULL, line_1, CENTRE) : 16 - strlen(line_1) - align(NULL, line_1, CENTRE);  
    //     uint line_2_pad = (direction) ? align(NULL, line_2, CENTRE) : 16 - strlen(line_2) - align(NULL, line_2, CENTRE);  
    //     max_length = MAX(strlen(line_1) + line_1_pad, strlen(line_2) + line_2_pad);
    //     printf("direction: %d\nline_1_pad: %d\nline_2_pad: %d\nmax_length: %d\n", direction, line_1_pad, line_2_pad, max_length);
    //     // Rotate to the edge of stage
    //     for (int i = 0; i < max_length + !direction; i ++) {
    //         pio_sm_put_blocking(pio, sm, (direction) ? 0b000011000 : 0b000011100 );
    //     }
    //     // Rotate back through stage, sleeping each rotate
    //     for (int i = 0; i < 16  + max_length;  i ++) {
    //         pio_sm_put_blocking(pio, sm, (direction) ? 0b000011100 : 0b000011000 ); 
    //         sleep_ms(300);
    //     }
    // }
}