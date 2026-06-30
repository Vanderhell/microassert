cmake_minimum_required(VERSION 3.20)
project(microassert_install_consumer C)

find_package(microassert CONFIG REQUIRED)

add_executable(install_consumer main.c)
target_link_libraries(install_consumer PRIVATE microassert::microassert)
set_target_properties(install_consumer PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
