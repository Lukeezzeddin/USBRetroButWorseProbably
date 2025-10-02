# pico-arm-toolchain.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# Target the RP2040 Cortex-M0+ (Thumb)
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mcpu=cortex-m0plus -mthumb")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-m0plus -mthumb")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mcpu=cortex-m0plus -mthumb")

# When CMake tests the compiler it must not expect to run executables on host:
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
