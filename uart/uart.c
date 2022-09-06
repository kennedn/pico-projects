#include "pico/stdlib.h"
#include <stdio.h>

#define LED 16

int main() {
    // stdio_uart_init_full(uart1, 6, 7, 9600);
    // stdio_uart_init();
    stdio_init_all();
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    while(true) {
        printf("\nEnter 1/0: ");
        switch (getchar()) {
            case '1':
                gpio_put(LED, 1);
                break;
            case '0':
                gpio_put(LED, 0);
                break;
            default:
                printf("\nPlease enter 1 or 0");
                break; 
        }
        sleep_ms(3000);
    }
}
