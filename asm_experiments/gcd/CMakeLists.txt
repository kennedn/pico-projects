add_executable(gcd)


# UART0 TX pin is broken, use UART1
target_compile_definitions(gcd PRIVATE 
PICO_DEFAULT_UART_TX_PIN=7
PICO_DEFAULT_UART_RX_PIN=8
PICO_DEFAULT_UART=1
)
target_sources(gcd PRIVATE gcd.c gcd.S)

target_link_libraries(gcd PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(gcd)
