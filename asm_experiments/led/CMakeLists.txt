add_executable(led)


# UART0 TX pin is broken, use UART1
target_compile_definitions(led PRIVATE 
PICO_DEFAULT_UART_TX_PIN=7
PICO_DEFAULT_UART_RX_PIN=8
PICO_DEFAULT_UART=1
)
target_sources(led PRIVATE led.c led.S)

target_link_libraries(led PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(led)
