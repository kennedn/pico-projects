
add_executable(lcd_animation)

pico_generate_pio_header(lcd_animation ${CMAKE_CURRENT_LIST_DIR}/lcd.pio)

target_sources(lcd_animation PRIVATE lcd_animation.c)


target_link_libraries(lcd_animation PRIVATE pico_stdlib hardware_pio)

pico_add_extra_outputs(lcd_animation)