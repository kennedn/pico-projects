#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/pio.h"
// #include "lcd.h"
#include "dht22.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

queue_t request_queue;
queue_t response_queue;

typedef enum {
    REQUEST_TEMPERATURE,
    REQUEST_HUMIDITY,
    REQUEST_NONE,
} RequestType;

typedef struct {
    DHTReading *reading;
    RequestType request;
} ResponseType;

//! Performs queue popping operations inline with sleep_ms(), this prevents
//! sleep_ms() from blocking queue operations
//! @param reading A DHTReading object that contains dht data
//! @param ms The time to sleep in milliseconds
static void queued_sleep_ms(DHTReading *reading, uint32_t ms) {
    static RequestType req;
    static ResponseType resp;
    for (uint32_t i = 0; i < ms; i++) {
        if(queue_try_remove(&request_queue, &req)) {
            if (req == REQUEST_NONE) {sleep_ms(1); continue;}
            resp = (ResponseType){.reading = reading, .request = req};
            queue_add_blocking(&response_queue, &resp);
        }
        sleep_ms(1);
    }
}

//! Core one main loop, performs serial input / output
void core1_main() {
    stdio_init_all();
	static char command[4];
    static const uint command_length = count_of(command);
	static int chr;
	static uint i;
    static RequestType req = REQUEST_NONE;
    static ResponseType resp;
    getchar();
	while (true) {
        i = 0; chr = 0;
        command[0] = '\0';

        while (true) {
            if(queue_try_remove(&response_queue, &resp)) {
                switch(resp.request) {
                    case REQUEST_TEMPERATURE:
                        printf("%s%.1f\n", (resp.reading->status) ? "SE" : "OK", resp.reading->temperature);
                        break;
                    case REQUEST_HUMIDITY:
                        printf("%s%.1f\n", (resp.reading->status) ? "SE" : "OK", resp.reading->humidity);
                        break;
                }
            }
            if ((chr = getchar_timeout_us(0)) == PICO_ERROR_TIMEOUT) {continue;}
            if(chr == '\r' || chr == EOF) {break;}
            if (i++ >= command_length - 1) {continue;}
            command[i-1] = chr;
        }

        if (i == 0) {continue;}
        if (i >= command_length) {
            printf("BR\n");
            continue;
        } 
        
		for(int i = 0; i < 3; i++){
  			command[i] = tolower(command[i]);
		}
        command[i] = '\0';

		if (!strcmp(command, "tmp")) {
            req = REQUEST_TEMPERATURE;
        } else if (!strcmp(command, "hum")) {
            req = REQUEST_HUMIDITY;
        } else {
            printf("BR\n");
            continue;
        }
        queue_add_blocking(&request_queue, &req);
    }
}

int main() {
    queue_init(&request_queue, sizeof(RequestType), 5);
    queue_init(&response_queue, sizeof(ResponseType), 5);
    static DHTReading reading = (DHTReading){.humidity = 0, .temperature = 0, .status = 1};
    static char temp_str[40], hum_str[40];
    uint32_t data[5];

    multicore_launch_core1(core1_main);
    // lcd_init(LCD_ALIGN_CENTRE);
    dht_init();

    while (true) {
        // Checksum not ok
        if(!dht_get(&reading)) {
            // lcd_write_blocking("\1 ?", "\2 ?");
            queued_sleep_ms(&reading, 1000);
            continue;
        }
        
        snprintf(temp_str, 40, "%s %.01f%sC","\1", reading.temperature, "\xdf"); 
        snprintf(hum_str, 40, "%s %.01f %s", "\2", reading.humidity, "\x25"); 
        // lcd_write_blocking(temp_str, hum_str);
        queued_sleep_ms(&reading, 1000);
    }

}
