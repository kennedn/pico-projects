add_executable(uart_scanf
        uart_scanf.c
        )
        
# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(uart_scanf pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(uart_scanf)
