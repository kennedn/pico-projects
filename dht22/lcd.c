#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "lcd.pio.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef enum {
    LCD_DATA_MASK = 0b100000000,
    LCD_CGRAM_MASK = 0b001000000,
    LCD_DDRAM_MASK = 0b010000000,

    LCD_RETURN_HOME = 0b000000010,
    LCD_SHIFT_LEFT = 0b000011000,
    LCD_SHIFT_RIGHT = 0b000011100,
    LCD_FUNC_SET = 0b000111000,   // 8 bit operation, 2 line mode, 5 x 8 font
    LCD_CURSOR_SET = 0b000001100, // Display on, cursor on, cursor blink
    LCD_SHIFT_SET =  0b000000110  // Set cursor right shift
} LCDCommands;

static const uint s_lcd_icons[] = {0x4, 0xa, 0xa, 0xe, 0xe, 0x1f, 0x1f, 0xe,     // thermometer
                                   0x4, 0x4, 0xa, 0xa, 0x11, 0x11, 0x11, 0xe};   // water droplet
static PIO s_pio;
static uint s_sm;
static uint s_offset;
static LCDAlignment s_alignment;

// align string in 16 character space based on alignment
static uint lcd_align(char *buffer, const char *text) {
    uint pad;
    int len = strlen(text);
    switch(s_alignment) {
        case LCD_ALIGN_CENTRE:
            pad = MAX(0, MIN(7, (16 - len) / 2));
            break;
        case LCD_ALIGN_RIGHT:
            pad = MAX(0, MIN(15, 16 - len));
            break;
        case LCD_ALIGN_LEFT:
        default:
            pad = 0;
            break;
    }
    if (buffer) {
        sprintf(buffer, "%*s%s", pad, "", text);
        snprintf(buffer, 41, "%-40s", buffer); // Pad row 1 to 40 chars because row 2 starts at pos 41
    }
    return pad;
}

static void s_lcd_write_icons() {
    // Custom 5x8 icons to send to module
    // Starting at first CG offset will cause character 1 to be accessable at \0, which causes issues for null terminated 
    //string functions. Quick hack is to lose a character and just start at offset 8 (\1)
    pio_sm_put_blocking(s_pio, s_sm, LCD_CGRAM_MASK | 8); // Point AC at CGRAM slot 1 (character generator RAM)

    // Write custom characters to LCD module, AC auto increments on each write
    for (int i = 0; i < count_of(s_lcd_icons); i++) {
        pio_sm_put_blocking(s_pio, s_sm, LCD_DATA_MASK | s_lcd_icons[i]); // write each line of 5x8 character
    }
    pio_sm_put_blocking(s_pio, s_sm, LCD_RETURN_HOME); // Set Address counter back to DDRAM
}


// Write out a padded two line message to a 16 x 2 display
void lcd_write_blocking(char *line_1, char *line_2) {
    pio_sm_put_blocking(s_pio, s_sm, LCD_RETURN_HOME); // Return home (Cursor at position 0)
    if (!line_1 && !line_2) { return; }
    const char *line_array[] = {line_1, line_2};
    char line_buffer[40];
    for (int i = 0; i < count_of(line_array); i++) {
        lcd_align(line_buffer, line_array[i]);
        // Write each character in string to 16x2 display
        for(int i = 0; i < strlen(line_buffer); i++) {
            pio_sm_put_blocking(s_pio, s_sm, LCD_DATA_MASK | line_buffer[i]); // register_select = 1 + data
        }
    }
}

void lcd_init(LCDAlignment alignment) {
    s_alignment = alignment;

    s_pio = pio0;
    s_offset = pio_add_program(s_pio, &lcd_program);
    s_sm = pio_claim_unused_sm(s_pio, true);

    // 1602m module needs clocked quite low (100khz) when D7 is in a voltage divider due to rise and fall times being atrocious
    // The pio program has been shown to run fine at up to around 1Mhz when not in a makeshift voltage divider
    lcd_program_init(s_pio, s_sm, s_offset, 7, 100);

    // Initalise LCD module
    pio_sm_put_blocking(s_pio, s_sm, LCD_FUNC_SET);
    pio_sm_put_blocking(s_pio, s_sm, LCD_CURSOR_SET); 
    pio_sm_put_blocking(s_pio, s_sm, LCD_SHIFT_SET); 

    s_lcd_write_icons();
}