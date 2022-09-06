add_executable(uart
        uart.c
        )
target_compile_definitions(uart PRIVATE 
PICO_DEFAULT_UART_TX_PIN=0
PICO_DEFAULT_UART_RX_PIN=1
PICO_DEFAULT_UART=0
)
# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(uart pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(uart)
