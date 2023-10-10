add_executable(sid_pico
        sid_pico.c
        )

# pull in common dependencies and additional pwm hardware support
target_link_libraries(sid_pico pico_stdlib hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(sid_pico)

