add_executable(uart
        uart.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(uart pico_stdlib pico_stdio_uart)

# create map/bin/hex file etc.
pico_add_extra_outputs(uart)
