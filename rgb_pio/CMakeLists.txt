
add_executable(rgb_pio)

pico_generate_pio_header(rgb_pio ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

target_sources(rgb_pio PRIVATE rgb.c)

target_link_libraries(rgb_pio PRIVATE
        pico_stdlib
        hardware_pio
        )

pico_add_extra_outputs(rgb_pio)