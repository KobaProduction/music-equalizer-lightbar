cmake_minimum_required(VERSION 3.24)

project(music_equalizer_lightbar LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MELB_BOARD "music-light-v3-221101" CACHE STRING "Board target")
set_property(CACHE MELB_BOARD PROPERTY STRINGS music-light-v3-221101)

if(MELB_BOARD STREQUAL "music-light-v3-221101")
    set(MELB_MCU "st17h66b")
    set(MELB_BOARD_MARKING "Music-Light-V3-221101")
else()
    message(FATAL_ERROR "Unsupported MELB_BOARD: ${MELB_BOARD}")
endif()

set(MELB_OUTPUT_BASENAME
    "music-equalizer-lightbar-${MELB_BOARD_MARKING}-${MELB_MCU}")

add_executable(melb_firmware
    src/main.c
    src/board_music_light_v3_221101.c
    src/ws2812b.c
    src/startup_st17h66b.S
)

set_target_properties(melb_firmware PROPERTIES
    OUTPUT_NAME "${MELB_OUTPUT_BASENAME}"
    SUFFIX ".elf"
)

target_compile_features(melb_firmware PRIVATE c_std_11)

target_compile_definitions(melb_firmware PRIVATE
    MELB_BOARD_MUSIC_LIGHT_V3_221101=1
    MELB_MCU_ST17H66B=1
)

set(MELB_ARCH_FLAGS
    -mcpu=cortex-m0
    -mthumb
)

target_compile_options(melb_firmware PRIVATE
    ${MELB_ARCH_FLAGS}
    -ffreestanding
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wextra
    -Wpedantic
)

set(MELB_MAP_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/${MELB_OUTPUT_BASENAME}.map")
set(MELB_HEX_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/${MELB_OUTPUT_BASENAME}.hex")
set(MELB_BIN_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/${MELB_OUTPUT_BASENAME}.bin")

target_link_options(melb_firmware PRIVATE
    ${MELB_ARCH_FLAGS}
    -nostartfiles
    -nostdlib
    -Wl,--gc-sections
    -Wl,--build-id=none
    -Wl,-Map,${MELB_MAP_FILE}
    -Wl,--script=${CMAKE_CURRENT_SOURCE_DIR}/src/st17h66b.ld
)

add_custom_command(TARGET melb_firmware POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex
            "$<TARGET_FILE:melb_firmware>" "${MELB_HEX_FILE}"
    COMMAND "${CMAKE_OBJCOPY}" -O binary
            "$<TARGET_FILE:melb_firmware>" "${MELB_BIN_FILE}"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:melb_firmware>"
    BYPRODUCTS
        "${MELB_HEX_FILE}"
        "${MELB_BIN_FILE}"
        "${MELB_MAP_FILE}"
    VERBATIM
)

message(STATUS "MELB board: ${MELB_BOARD_MARKING}")
message(STATUS "MELB MCU: ${MELB_MCU}")
message(STATUS "Linked ST17H66B image is experimental until hardware validation")
