cmake_minimum_required(VERSION 3.13)
set(NAME sid_pico)

include(pico_sdk_import.cmake)

project(sid_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(${PROJECT_NAME}
    src/sid_pico.c
    src/sid_server.c
)



if (DEFINED ENV{WIFI_SSID} AND (NOT WIFI_SSID))
    set(WIFI_SSID $ENV{WIFI_SSID})
    message("Using WIFI_SSID from environment ('${WIFI_SSID}')")
endif()

if (DEFINED ENV{WIFI_PASSWORD} AND (NOT WIFI_PASSWORD))
    set(WIFI_PASSWORD $ENV{WIFI_PASSWORD})
    message("Using WIFI_PASSWORD from environment")
endif()

set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID")
        
set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi password")

target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/..
        )
target_link_libraries(${PROJECT_NAME}   pico_stdlib
                                        pico_multicore
                                        pico_cyw43_arch_lwip_poll
                                        hardware_pio
                                        hardware_pwm)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/sidpio.pio)
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_add_extra_outputs(${PROJECT_NAME})


install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)