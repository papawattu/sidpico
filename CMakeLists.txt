cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)

# Initialize the SDK
pico_sdk_init()

project(sid_pico)
add_subdirectory(./src)

