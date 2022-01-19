
add_executable(dht22)

pico_generate_pio_header(dht22 ${CMAKE_CURRENT_LIST_DIR}/dht22.pio)
pico_generate_pio_header(dht22 ${CMAKE_CURRENT_LIST_DIR}/lcd.pio)

# target_compile_definitions(dht22 PRIVATE 
# PICO_DEFAULT_UART_TX_PIN=4
# PICO_DEFAULT_UART_RX_PIN=5
# PICO_DEFAULT_UART=1
# )

target_sources(dht22 PRIVATE main.c dht22.c lcd.c)
target_link_libraries(dht22 PRIVATE pico_stdlib hardware_pio pico_multicore)

pico_add_extra_outputs(dht22)