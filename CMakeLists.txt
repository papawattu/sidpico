cmake_minimum_required(VERSION 3.13)
set(NAME sid_pico)

include(pico_sdk_import.cmake)

project(sid_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_TARGET pico)

# Initialize the SDK
pico_sdk_init()


add_executable(${PROJECT_NAME}
    src/sid_pico.c
)

target_link_libraries(${PROJECT_NAME}   pico_stdlib
                                        pico_multicore
                                        hardware_pwm)
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_add_extra_outputs(${PROJECT_NAME})


install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)