
add_executable(lcd_scroll)

pico_generate_pio_header(lcd_scroll ${CMAKE_CURRENT_LIST_DIR}/lcd.pio)

target_sources(lcd_scroll PRIVATE lcd_scroll.c)


target_link_libraries(lcd_scroll PRIVATE pico_stdlib hardware_pio)

pico_add_extra_outputs(lcd_scroll)