add_executable(uart
        uart.c
        )
target_compile_definitions(uart PRIVATE 
PICO_DEFAULT_UART_TX_PIN=8
PICO_DEFAULT_UART_RX_PIN=9
PICO_DEFAULT_UART=1
)
# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(uart pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(uart)
