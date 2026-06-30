cmake_minimum_required(VERSION 3.20)

project(microassert VERSION 0.0.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

option(MICROASSERT_BUILD_TESTS "Build microassert tests" ON)
set(MICROASSERT_C_STANDARD 99 CACHE STRING "C language standard for microassert targets")
set_property(CACHE MICROASSERT_C_STANDARD PROPERTY STRINGS 99 11)

function(microassert_enable_c_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /Zc:preprocessor)
    elseif (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wstrict-prototypes
            -Wmissing-prototypes
            -Wcast-qual
            -Wwrite-strings
            -Wformat=2
            -Werror)
    endif()
endfunction()

function(microassert_enable_cxx_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /Zc:preprocessor)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wcast-qual
            -Wwrite-strings
            -Wformat=2
            -Werror)
    endif()
endfunction()

add_library(microassert src/massert.c)
add_library(microassert::microassert ALIAS microassert)

target_include_directories(microassert
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(microassert PROPERTIES
    C_STANDARD ${MICROASSERT_C_STANDARD}
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)

microassert_enable_c_warnings(microassert)

if (MICROASSERT_BUILD_TESTS AND BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS microassert
    EXPORT microassertTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES include/massert.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT microassertTargets
    NAMESPACE microassert::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microassert)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/microassertConfig.cmake.in
    ${PROJECT_BINARY_DIR}/microassertConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microassert
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO)

install(FILES ${PROJECT_BINARY_DIR}/microassertConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microassert)
