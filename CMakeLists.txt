cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

# Add blink example
add_subdirectory(shift_register)
add_subdirectory(shift_register_pio)
add_subdirectory(multi_pio)
add_subdirectory(uart)
add_subdirectory(uart_scanf)
add_subdirectory(hello_pio)
add_subdirectory(rgb_pio)
add_subdirectory(lcd_scroll)
add_subdirectory(lcd_animation)
add_subdirectory(dht22)
add_subdirectory(infrared_nec)
add_subdirectory(asm_experiments)

cmake_minimum_required(VERSION 3.12)

if (PICO_CYW43_SUPPORTED) # set by BOARD=pico-w
    if (NOT TARGET pico_cyw43_arch)
        message("Skipping Pico W examples as support is not available")
    else()
        set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID for examples")
        set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi password for examples")


        if ("${WIFI_SSID}" STREQUAL "")
            message("Skipping some Pico W examples as WIFI_SSID is not defined")
        elseif ("${WIFI_PASSWORD}" STREQUAL "")
            message("Skipping some Pico W examples as WIFI_PASSWORD is not defined")
        else()
            add_subdirectory(infrared_nec_wifi)
        endif()
    endif()
endif()
