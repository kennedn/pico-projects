add_executable(shift
        shift.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(shift pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(shift)

add_custom_command(TARGET shift
        POST_BUILD
        COMMAND /bin/bash -c 'cp *.uf2 /media/kennedn/RPI-RP2/ || (exit 0)'
        )