cmake_minimum_required(VERSION 3.24)

project(music_equalizer_lightbar LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MELB_TARGET "st17h66b" CACHE STRING "Firmware target")
set_property(CACHE MELB_TARGET PROPERTY STRINGS st17h66b)

if(NOT MELB_TARGET STREQUAL "st17h66b")
    message(FATAL_ERROR "Unsupported MELB_TARGET: ${MELB_TARGET}")
endif()

add_library(melb_firmware OBJECT
    src/main.c
)

target_compile_features(melb_firmware PRIVATE c_std_11)

target_compile_definitions(melb_firmware PRIVATE
    MELB_TARGET_ST17H66B=1
)

target_compile_options(melb_firmware PRIVATE
    -mcpu=cortex-m0
    -mthumb
    -ffreestanding
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wextra
    -Wpedantic
)

message(STATUS "MELB target: ${MELB_TARGET}")
message(STATUS "Bootstrap mode: compile-only; no flashable image is linked yet")
