
add_executable(shift_register_pio)

pico_generate_pio_header(shift_register_pio ${CMAKE_CURRENT_LIST_DIR}/shift_register.pio)

target_sources(shift_register_pio PRIVATE shift_register.c)

target_link_libraries(shift_register_pio PRIVATE
        pico_stdlib
        hardware_pio
        )

pico_add_extra_outputs(shift_register_pio)