add_executable(infrared_wifi)

pico_generate_pio_header(infrared_wifi ${CMAKE_CURRENT_LIST_DIR}/nec.pio)

# UART0 TX pin is broken, use UART1
target_compile_definitions(infrared_wifi PRIVATE 
    PICO_DEFAULT_UART_TX_PIN=0
    PICO_DEFAULT_UART_RX_PIN=1
    PICO_DEFAULT_UART=0
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)
target_sources(infrared_wifi PRIVATE infrared_wifi.c)

target_include_directories(infrared_wifi PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(infrared_wifi PRIVATE 
    pico_stdlib 
    hardware_pio
    pico_cyw43_arch_lwip_threadsafe_background
)
pico_add_extra_outputs(infrared_wifi)