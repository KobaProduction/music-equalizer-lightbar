include("${CMAKE_CURRENT_LIST_DIR}/phyplus_external.cmake")

function(melb_add_phyplus_lotus_firmware)
    melb_resolve_phyplus_sdk(_phyplus_root)

    if(NOT MELB_THB2_SOURCE_DIR)
        message(FATAL_ERROR "THB2 source directory was not resolved")
    endif()

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    find_program(MELB_MAKE_EXECUTABLE NAMES make gmake REQUIRED)

    get_filename_component(_gcc_dir "${CMAKE_C_COMPILER}" DIRECTORY)

    set(_overlay "${CMAKE_CURRENT_BINARY_DIR}/thb2-lotus")
    set(_elf "${_overlay}/build/melb_lotus.elf")
    set(_hex "${_overlay}/build/melb_lotus.hex")
    set(_bin "${_overlay}/build/melb_lotus.bin")
    set(_map "${_overlay}/build/melb_lotus.map")

    add_custom_command(
        OUTPUT "${_elf}" "${_hex}" "${_bin}" "${_map}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/prepare_thb2_lotus_overlay.py"
            --thb2 "${MELB_THB2_SOURCE_DIR}"
            --project "${CMAKE_CURRENT_SOURCE_DIR}"
            --output "${_overlay}"
        COMMAND "${CMAKE_COMMAND}" -E env
            "PATH=${_gcc_dir}:$ENV{PATH}"
            "${MELB_MAKE_EXECUTABLE}"
            -C "${_overlay}"
            -j2
            PROJECT_NAME=melb_lotus
            PROJECT_DEF=-DDEVICE=DEVICE_KEY2
            all
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/pvvx_lotus_sbp_profile.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/ble_control.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/lotus_lantern.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/local_controls.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/st17h66b_spi1.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/ws2812b.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/ws2812b_spi.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/prepare_thb2_lotus_overlay.py"
        VERBATIM
        USES_TERMINAL
    )

    add_custom_target(melb-lotus-ble
        DEPENDS "${_elf}" "${_hex}" "${_bin}" "${_map}")

    message(STATUS "Lotus Lantern BLE firmware uses pinned THB2 build substrate")
    message(STATUS "Lotus Lantern BLE artifacts: ${_overlay}/build")
endfunction()
