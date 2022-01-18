#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "lcd.pio.h"
#include "dht22.pio.h"
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


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
        // No buffer passed, just return pad
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
    snprintf(line_buffer, 41, "%-40s", line_buffer); // Pad row 2 to 40 chars too
    for(int i = 0; i < strlen(line_buffer); i++) {
        pio_sm_put_blocking(pio, sm, DATA_MASK | line_buffer[i]); // register_select = 1 + data
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

// Fifo's / OSR likely dirty from sensor disconnect, reset everything for a clean slate
void dht_reset_sm(PIO pio, uint sm) {
    pio_sm_clear_fifos(pio,sm);
    pio_sm_restart(pio, sm);
}

int main() {
    seed_random_from_rosc();
    stdio_init_all();

    PIO pio = pio0;

    uint dht_offset = pio_add_program(pio, &dht22_program);
    uint lcd_offset = pio_add_program(pio, &lcd_program);

    uint dht_sm = pio_claim_unused_sm(pio, true);
    uint lcd_sm = pio_claim_unused_sm(pio, true);

    dht22_program_init(pio, dht_sm, dht_offset, 6);
    lcd_program_init(pio, lcd_sm, lcd_offset, 7, 1);

    // Initalise LCD module
    pio_sm_put_blocking(pio, lcd_sm, FUNC_SET);
    pio_sm_put_blocking(pio, lcd_sm, CURSOR_SET); 
    pio_sm_put_blocking(pio, lcd_sm, SHIFT_SET); 

    // Custom 5x8 icons to send to module
    const uint patterns[16] = {0x4, 0xa, 0xa, 0xe, 0xe, 0x1f, 0x1f, 0xe,  // thermometer
                               0x4, 0x4, 0xa, 0xa, 0x11, 0x11, 0x11, 0xe};   // water droplet
    
    // Starting at first CG offset will cause character 1 to be accessable at \0, which causes issues for null terminated 
    //string functions. Quick hack is to lose a character and just start at offset 8 (\1)
    pio_sm_put_blocking(pio, lcd_sm, CGRAM_MASK | 8); // Point AC at CGRAM slot 1 (character generator RAM)

    // Write custom characters to LCD module, AC auto increments on each write
    for (int i = 0; i < 16; i++) {
        pio_sm_put_blocking(pio, lcd_sm, DATA_MASK | patterns[i]); // write each line of 5x8 character
        sleep_ms(1); // PIO driver can stall if consecutive puts happen to fast
    }
    pio_sm_put(pio, lcd_sm, RETURN_HOME); // Set Address counter back to DDRAM

    float humidity, temperature;
    char temp_str[40], hum_str[40];
    uint32_t data[5];
    dht_reset_sm(pio, dht_sm);
    while (true) {
        pio_sm_exec(pio, dht_sm, pio_encode_jmp(dht_offset));

        for(int i=0; i < count_of(data); i++) {
            data[i] = pio_sm_get(pio, dht_sm);
        }

        // Checksum not ok
        if(((data[0] + data[1] + data[2] + data[3]) & 0xFF) != data[4] && data[4] != 0) {
            if(data[4] == 0) {
                printf("\nchecksum error (%d == 0)\n", data[4]);
            } else {
                printf("\nchecksum error (%d != %d)\n", data[4], ((data[0] + data[1] + data[2] + data[3]) & 0xFF));
            }
            dht_reset_sm(pio, dht_sm);
            lcd_write_blocking(pio, lcd_sm, "\1 ?", "\2 ?", CENTRE, CENTRE);
            sleep_ms(1000);
            continue;
        }
        
        humidity = (data[0] << 8 | data[1]) * 0.1;
        // First bit of data[2] must be treated as a positive / negative flag
        temperature = ((data[2] & 0x80) ? -1 : 1) * ((data[2] & 0x7F) << 8 | data[3]) * 0.1;

          // printf("\nChecksum %d == %d : %s", ((data[0] + data[1] + data[2] + data[3]) & 0xFF), data[4], (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) ? "true" : "false");
        // printf("\nData [1]: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(data[0]));
        // printf("\nData [2]: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(data[1]));
        // printf("\nData [3]: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(data[2]));
        // printf("\nData [4]: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(data[3]));
        // printf("\nSum: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(data[4]));
        printf(".");

        snprintf(temp_str, 40, "%s %.02f%sC","\1", temperature, "\xdf"); 
        snprintf(hum_str, 40, "%s %.02f %s", "\2", humidity, "\x25"); 
        lcd_write_blocking(pio, lcd_sm, temp_str, hum_str, CENTRE, CENTRE);
        sleep_ms(1000);
    }

    // PIO pio = pio0;

    // // lcd.pio should have been compiled to a header file, we need to load this
    // // into the given pio block, returns offset
    // uint offset = pio_add_program(pio, &lcd_program);

    // // Get a free state machine and call our init function from lcd.pio header
    // uint sm = pio_claim_unused_sm(pio, true);
    // lcd_program_init(pio, sm, offset, 7, 1);

    // // Initalise LCD module
    // pio_sm_put_blocking(pio, sm, FUNC_SET);
    // pio_sm_put_blocking(pio, sm, CURSOR_SET); 
    // pio_sm_put_blocking(pio, sm, SHIFT_SET); 

    // // Custom 5x8 icons to send to module
    // const uint patterns[4][8] = {{0xe, 0x10, 0x10, 0x1f, 0x1b, 0x1b, 0x1f, 0x0},  // Unlock
    //                     {0xe, 0x11, 0x11, 0x1f, 0x1b, 0x1b, 0x1f, 0x0},           // Lock
    //                     {0x0, 0xa, 0x1f, 0x1f, 0xe, 0x4, 0x0, 0x0},               // Heart
    //                     {0x1f, 0x15, 0x1f, 0x1f, 0xe, 0xa, 0x1b, 0x0}};           // Guy
    
    // // Starting at first CG offset will cause character 1 to be accessable at \0, which causes issues for null terminated 
    // //string functions. Quick hack is to lose a character and just start at offset 8 (\1)
    // pio_sm_put_blocking(pio, sm, CGRAM_MASK | 8); // Point AC at CGRAM slot 1 (character generator RAM)

    // // Write custom characters to LCD module, AC auto increments on each write
    // for (int i = 0; i < count_of(patterns); i++) {
    //     for (int j = 0; j < count_of(patterns[i]); j ++) {
    //         pio_sm_put_blocking(pio, sm, DATA_MASK | patterns[i][j]); // write each line of 5x8 character
    //     }
    // }

    // pio_sm_put_blocking(pio, sm, RETURN_HOME); // Set Address counter back to DDRAM

    // char *line_1_words[] = {"Stinky", "Radical", "Wholesome", "Hungis", "Chocolate", "Hugh", "Quantum", "Holy", "Ultimate", "Beefy"};
    // char *line_2_words[] = {"Bepzinky", "Larry", "Rimbonski", "Bungo", "Mungus", "Rat", "Monke", "Goblin", "Beef Boy"};
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
    //     snprintf(line_1, 41, "%s %s %s", customs[icon],line_1_words[rand() % count_of(line_1_words)], customs[icon]); 

    //     icon = rand() % count_of(customs);
    //     snprintf(line_2, 41, "%s %s %s", customs[icon], line_2_words[rand() % count_of(line_2_words)], customs[icon]);

    //     direction = !direction;
    //     // write lines to centre stage on LCD
    //     lcd_write_blocking(pio, sm, line_1, line_2, CENTRE, CENTRE);
    //     // Find biggest string length in two lines
    //     uint line_1_pad = (direction) ? align(NULL, line_1, CENTRE) : 16 - strlen(line_1) - align(NULL, line_1, CENTRE);  
    //     uint line_2_pad = (direction) ? align(NULL, line_2, CENTRE) : 16 - strlen(line_2) - align(NULL, line_2, CENTRE);  
    //     max_length = MAX(strlen(line_1) + line_1_pad, strlen(line_2) + line_2_pad);
    //     printf("direction: %d\nline_1_pad: %d\nline_2_pad: %d\nmax_length: %d\n", direction, line_1_pad, line_2_pad, max_length);
    //     // Rotate to the edge of stage
    //     for (int i = 0; i < max_length; i ++) {
    //         pio_sm_put_blocking(pio, sm, (direction) ? SHIFT_LEFT : SHIFT_RIGHT );
    //     }
    //     // Rotate back through stage, sleeping each rotate
    //     for (int i = 0; i < 16  + max_length;  i ++) {
    //         pio_sm_put_blocking(pio, sm, (direction) ? SHIFT_RIGHT : SHIFT_LEFT ); 
    //         sleep_ms(200);
    //     }
    // }
}