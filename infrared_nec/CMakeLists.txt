add_executable(infrared)

pico_generate_pio_header(infrared ${CMAKE_CURRENT_LIST_DIR}/nec.pio)

# UART0 TX pin is broken, use UART1
target_compile_definitions(infrared PRIVATE 
PICO_DEFAULT_UART_TX_PIN=0
PICO_DEFAULT_UART_RX_PIN=1
PICO_DEFAULT_UART=0
)
target_sources(infrared PRIVATE infrared.c)

target_link_libraries(infrared PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(infrared)
