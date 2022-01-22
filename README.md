# pico-projects

A collection of my experiments and projects on the Raspberry Pi Pico microcontroller.

# Notable Projects

- [DHT22](dht22) - Temperature / humidity sensor project. Implements a  PIOASM driver for the DHT22 module.
- [LCD Animation](lcd_animation) / [LCD Scroll](lcd_scroll) - Implements a PIOASM driver for a generic 16x2 LCD module in 8 bit mode. Explores text scrolling and driving simple animations on the module.
- [Infrared NEC](infrared_nec) -  Implements the NEC infrared protocol as a PIOASM driver. The project lets the pico mimic an IR remote control. IR codes are accepted via serial and transmitted to a connected IR LED.