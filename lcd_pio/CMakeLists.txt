
add_executable(lcd_pio)

pico_generate_pio_header(lcd_pio ${CMAKE_CURRENT_LIST_DIR}/lcd.pio)

target_sources(lcd_pio PRIVATE lcd.c)

target_link_libraries(lcd_pio PRIVATE
        pico_stdlib
        hardware_pio
        )

pico_add_extra_outputs(lcd_pio)