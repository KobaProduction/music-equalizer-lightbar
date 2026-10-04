cmake_minimum_required(VERSION 3.24)

project(music_equalizer_lightbar LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MELB_HOST_TESTS "Build host-side protocol tests instead of firmware" OFF)
option(MELB_PHYPLUS_PROFILE_CHECK "Compile project BLE profile against fetched Phyplus SDK" OFF)
option(MELB_PHYPLUS_HAPPYLIGHTING_FIRMWARE "Build full HappyLighting/Triones BLE firmware using fetched THB2" OFF)

if(MELB_PHYPLUS_PROFILE_CHECK)
    enable_language(ASM)
    include(cmake/phyplus_external.cmake)
    melb_add_phyplus_profile_check()
    return()
endif()

if(MELB_PHYPLUS_HAPPYLIGHTING_FIRMWARE)
    enable_language(ASM)
    include(cmake/phyplus_happylighting_firmware.cmake)
    melb_add_phyplus_happylighting_firmware()
    return()
endif()

if(MELB_HOST_TESTS)
    enable_testing()

    add_executable(melb_host_tests
        tests/test_protocol.c
        src/ble_control.c
        src/board_music_light_v3_221101.c
        src/happylighting.c
        src/local_controls.c
        src/ws2812b.c
        src/ws2812b_spi.c
    )

    target_include_directories(melb_host_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    target_compile_features(melb_host_tests PRIVATE c_std_11)
    target_compile_options(melb_host_tests PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    add_test(NAME protocol COMMAND melb_host_tests)
    return()
endif()

enable_language(ASM)

set(MELB_BOARD "music-light-v3-221101" CACHE STRING "Board target")
set_property(CACHE MELB_BOARD PROPERTY STRINGS music-light-v3-221101)

set(MELB_VARIANT "base" CACHE STRING "Firmware variant")
set_property(CACHE MELB_VARIANT PROPERTY STRINGS base ws2812b-basic)

if(MELB_BOARD STREQUAL "music-light-v3-221101")
    set(MELB_MCU "st17h66b")
    set(MELB_BOARD_MARKING "Music-Light-V3-221101")
else()
    message(FATAL_ERROR "Unsupported MELB_BOARD: ${MELB_BOARD}")
endif()

set(MELB_FIRMWARE_SOURCES
    src/ble_control.c
    src/board_music_light_v3_221101.c
    src/happylighting.c
    src/ws2812b.c
    src/startup_st17h66b.S
)

if(MELB_VARIANT STREQUAL "base")
    list(APPEND MELB_FIRMWARE_SOURCES
        src/main.c
    )
    set(MELB_OUTPUT_BASENAME
        "music-equalizer-lightbar-${MELB_BOARD_MARKING}-${MELB_MCU}")
elseif(MELB_VARIANT STREQUAL "ws2812b-basic")
    list(APPEND MELB_FIRMWARE_SOURCES
        examples/ws2812b_basic_main.c
        src/ws2812b_spi.c
        examples/st17h66b_spi1_standalone.c
    )
    set(MELB_OUTPUT_BASENAME
        "music-equalizer-lightbar-${MELB_BOARD_MARKING}-${MELB_MCU}-ws2812b-basic")
else()
    message(FATAL_ERROR "Unsupported MELB_VARIANT: ${MELB_VARIANT}")
endif()

add_executable(melb_firmware
    ${MELB_FIRMWARE_SOURCES}
)

target_include_directories(melb_firmware PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
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

if(MELB_VARIANT STREQUAL "ws2812b-basic")
    target_compile_definitions(melb_firmware PRIVATE
        MELB_VARIANT_WS2812B_BASIC=1
    )
endif()

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
set(MELB_MANIFEST_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/${MELB_OUTPUT_BASENAME}.manifest.json")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/firmware-manifest.json.in"
    "${MELB_MANIFEST_FILE}"
    @ONLY
)

target_link_libraries(melb_firmware PRIVATE
    gcc
)

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
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/verify_st17h66b_image.py"
            --bin "${MELB_BIN_FILE}"
            --hex "${MELB_HEX_FILE}"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:melb_firmware>"
    BYPRODUCTS
        "${MELB_HEX_FILE}"
        "${MELB_BIN_FILE}"
        "${MELB_MAP_FILE}"
        "${MELB_MANIFEST_FILE}"
    VERBATIM
)

message(STATUS "MELB board: ${MELB_BOARD_MARKING}")
message(STATUS "MELB MCU: ${MELB_MCU}")
message(STATUS "MELB variant: ${MELB_VARIANT}")
message(STATUS "Linked ST17H66B image is experimental until hardware validation")
