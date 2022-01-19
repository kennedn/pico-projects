#pragma once
#include "pico/stdlib.h"
#include "hardware/pio.h"

typedef enum {
    LCD_ALIGN_LEFT,
    LCD_ALIGN_CENTRE,
    LCD_ALIGN_RIGHT
} LCDAlignment;

typedef enum {
    LCD_LINE_1,
    LCD_LINE_2
} LCDLine;

void lcd_write_blocking(char *line_1, char *line_2);
void lcd_init(LCDAlignment alignment);