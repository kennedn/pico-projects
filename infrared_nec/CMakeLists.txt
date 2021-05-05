add_executable(infrared)

pico_generate_pio_header(infrared ${CMAKE_CURRENT_LIST_DIR}/nec.pio)


target_sources(infrared PRIVATE infrared.c)

target_link_libraries(infrared PRIVATE pico_stdlib pico_stdio_uart hardware_pio)
pico_add_extra_outputs(infrared)
