include(FetchContent)

set(MELB_THB2_GIT_REPOSITORY
    "https://github.com/pvvx/THB2.git"
    CACHE STRING "THB2 repository used for the external Phyplus SDK")
set(MELB_THB2_GIT_TAG
    "48db5245d235aef57cfdf5fcc58ec74fa753d89c"
    CACHE STRING "Pinned THB2 commit used for the external Phyplus SDK")

function(melb_resolve_phyplus_sdk out_var)
    FetchContent_Declare(
        thb2
        GIT_REPOSITORY "${MELB_THB2_GIT_REPOSITORY}"
        GIT_TAG "${MELB_THB2_GIT_TAG}"
        GIT_SHALLOW FALSE
        GIT_PROGRESS TRUE
    )

    FetchContent_GetProperties(thb2)
    if(NOT thb2_POPULATED)
        message(STATUS "Fetching pvvx/THB2 at ${MELB_THB2_GIT_TAG}")
        FetchContent_Populate(thb2)
    endif()

    set(_phyplus_root "${thb2_SOURCE_DIR}/bthome_phy6222/SDK")

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
                "Fetched THB2 tree does not contain the expected PHY62x2 SDK file: ${_required}")
        endif()
    endforeach()

    set(${out_var} "${_phyplus_root}" PARENT_SCOPE)
endfunction()

function(melb_add_phyplus_profile_check)
    melb_resolve_phyplus_sdk(_phyplus_root)

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
        "Phyplus BLE profile compile-check uses fetched SDK: ${_phyplus_root}")
endfunction()
