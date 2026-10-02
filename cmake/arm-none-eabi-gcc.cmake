set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# The bootstrap currently compiles objects only. Avoid CMake linker probes until
# the ST17H66B startup/linker contract is established and validated.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_NONE_EABI_GCC arm-none-eabi-gcc REQUIRED)
find_program(ARM_NONE_EABI_AR arm-none-eabi-ar REQUIRED)
find_program(ARM_NONE_EABI_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(ARM_NONE_EABI_SIZE arm-none-eabi-size REQUIRED)

set(CMAKE_C_COMPILER "${ARM_NONE_EABI_GCC}")
set(CMAKE_ASM_COMPILER "${ARM_NONE_EABI_GCC}")
set(CMAKE_AR "${ARM_NONE_EABI_AR}")

set(CMAKE_OBJCOPY "${ARM_NONE_EABI_OBJCOPY}" CACHE FILEPATH "GNU Arm objcopy")
set(CMAKE_SIZE "${ARM_NONE_EABI_SIZE}" CACHE FILEPATH "GNU Arm size")
