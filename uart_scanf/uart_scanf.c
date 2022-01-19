/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include <ctype.h>
#include <string.h>

#define LED_PIN 25
#define BUFFER_LEN 20

typedef enum {
	COMMAND_TEMPERATURE,
	COMMAND_HUMIDITY,
	COMMAND_REBOOT,
	COMMAND_NONE
} SerialCommand;

int main() {
	stdio_init_all();
	static char buffer[BUFFER_LEN];// = {"tmp 1"};
	static char command[5];
	static char chr;
	static int arg;
	static uint i;
	static SerialCommand s_cmd;
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	while(1)
	{
        int i = 0, chr = 0, arg = -1;
		s_cmd = COMMAND_NONE;
		buffer[0] = '\0';
        while ((chr = getchar()) != '\r' && chr != EOF) {
            if (i++ >= BUFFER_LEN - 1) {continue;}
            buffer[i-1] = chr;
        }
        if (i >= BUFFER_LEN) {continue;}
		if (sscanf(buffer, "%4s %d", command, &arg) <= 0) {
			printf("sscanf error\n");
			continue;
		}
		printf("command: %s (%d), arg: %d\n", command, strlen(command), arg);
		for(int i = 0; command[i]; i++){
  			command[i] = tolower(command[i]);
		}

		if (!strcmp(command, "tmp")) {s_cmd = COMMAND_TEMPERATURE;}
		else if (!strcmp(command, "hum")) {s_cmd = COMMAND_HUMIDITY;}
		else if (!strcmp(command, "rbt")) {s_cmd = COMMAND_REBOOT;}

		switch(s_cmd) {
			case COMMAND_TEMPERATURE:
				gpio_put(LED_PIN, 1);
				break;
			case COMMAND_HUMIDITY:
				gpio_put(LED_PIN, 0);
				break;
			case COMMAND_REBOOT:
				reset_usb_boot(0, 0);
				break;
			default:
				printf("Unknown command\n");
				break;
		}
	}
}