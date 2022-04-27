# Infrared NEC

The project lets the pico mimic an IR remote control. IR codes are accepted via serial and transmitted to a connected IR LED.

The PIOASM driver implements both the transmission of data and a 38kHz modulation to produce the carrier wave on a single state machine. This project was my own delve into the protocol and was pieced together from skimming the data sheet and observing oscilloscope output. 

As a result is is not the most efficient way of achieving the NEC protocol on the pico (coming in a whopping 30 instructions!) and I would suggest taking a look at the code found in the official [pico-examples repository](https://github.com/raspberrypi/pico-examples/tree/master/pio/ir_nec/nec_transmit_library). This example implements the same protocol across two state machines using IRQ, something I long thought possible but had insufficient examples of state machine IRQ to attempt it when I was originally writing the driver.