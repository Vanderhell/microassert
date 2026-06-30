set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

# Bare-metal toolchains usually cannot link CMake's hosted probe executables
# without board-specific startup code and syscalls.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
