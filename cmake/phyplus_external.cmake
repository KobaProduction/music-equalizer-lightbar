function(melb_add_phyplus_profile_check)
    if(NOT MELB_PHYPLUS_SDK_ROOT)
        message(FATAL_ERROR
            "MELB_PHYPLUS_SDK_ROOT is required for the Phyplus BLE profile check")
    endif()

    get_filename_component(_phyplus_root
        "${MELB_PHYPLUS_SDK_ROOT}" ABSOLUTE)

    set(_required_files
        "${_phyplus_root}/components/ble/include/att.h"
        "${_phyplus_root}/components/ble/host/gatt.h"
        "${_phyplus_root}/components/ble/host/gattservapp.h"
        "${_phyplus_root}/components/inc/mcu_phy_bumbee.h"
        "${_phyplus_root}/misc/bb_rom_sym_m0.gcc"
    )

    foreach(_required IN LISTS _required_files)
        if(NOT EXISTS "${_required}")
            message(FATAL_ERROR
                "MELB_PHYPLUS_SDK_ROOT does not look like the PHY62x2 SDK: missing ${_required}")
        endif()
    endforeach()

    add_library(melb_phyplus_lotus_profile OBJECT
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ble_control.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lotus_lantern.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/phyplus_lotus_gatt.c"
    )

    target_include_directories(melb_phyplus_lotus_profile PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${_phyplus_root}/misc"
        "${_phyplus_root}/misc/CMSIS/include"
        "${_phyplus_root}/misc/CMSIS/device/phyplus"
        "${_phyplus_root}/components/arch/cm0"
        "${_phyplus_root}/components/inc"
        "${_phyplus_root}/components/ble/include"
        "${_phyplus_root}/components/ble/host"
        "${_phyplus_root}/components/ble/hci"
        "${_phyplus_root}/components/profiles/GATT"
        "${_phyplus_root}/components/profiles/Roles"
        "${_phyplus_root}/components/osal/include"
        "${_phyplus_root}/components/driver/log"
        "${_phyplus_root}/components/driver/uart"
        "${_phyplus_root}/components/driver/gpio"
    )

    target_compile_definitions(melb_phyplus_lotus_profile PRIVATE
        __GCC
        ARMCM0
        PHY_MCU_TYPE=MCU_BUMBEE_M0
        HOST_CONFIG=0x04
        MAX_NUM_LL_CONN=1
        DEF_GAPBOND_MGR_ENABLE=0
        DEBUG_INFO=0
    )

    target_compile_options(melb_phyplus_lotus_profile PRIVATE
        -mcpu=cortex-m0
        -mthumb
        -ffreestanding
        -fdata-sections
        -ffunction-sections
        -Wall
        -Wextra
        -Wpedantic
    )

    add_custom_target(phyplus-ble-profile-check
        DEPENDS melb_phyplus_lotus_profile)

    message(STATUS
        "Phyplus BLE profile compile-check enabled with external SDK: ${_phyplus_root}")
endfunction()
