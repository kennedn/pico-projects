add_executable(shift
        shift.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(shift pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(shift)
