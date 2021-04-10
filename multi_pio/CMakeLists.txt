
add_executable(multi_pio)

pico_generate_pio_header(multi_pio ${CMAKE_CURRENT_LIST_DIR}/shift.pio)
pico_generate_pio_header(multi_pio ${CMAKE_CURRENT_LIST_DIR}/hello.pio)

target_sources(multi_pio PRIVATE multi.c)

target_link_libraries(multi_pio PRIVATE
        pico_stdlib
        hardware_pio
        )

pico_add_extra_outputs(multi_pio)