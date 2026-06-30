if (NOT DEFINED BUILD_DIR OR NOT DEFINED PROJECT_SOURCE_DIR)
    message(FATAL_ERROR "missing install_consumer inputs")
endif()

set(prefix "${BUILD_DIR}/_microassert_install")
set(consumer_source "${PROJECT_SOURCE_DIR}/tests/fixtures/install_consumer")
set(consumer_build "${BUILD_DIR}/_microassert_install_consumer")
set(configure_args)

file(REMOVE_RECURSE "${consumer_build}")

if (DEFINED GENERATOR AND NOT GENERATOR STREQUAL "")
    list(APPEND configure_args -G "${GENERATOR}")
endif()
if (DEFINED CMAKE_C_COMPILER AND NOT CMAKE_C_COMPILER STREQUAL "")
    list(APPEND configure_args -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER})
endif()
if (DEFINED CMAKE_CXX_COMPILER AND NOT CMAKE_CXX_COMPILER STREQUAL "")
    list(APPEND configure_args -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
endif()
if (DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
    list(APPEND configure_args -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
endif()
if (DEFINED CMAKE_C_FLAGS AND NOT CMAKE_C_FLAGS STREQUAL "")
    list(APPEND configure_args -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS})
endif()
if (DEFINED CMAKE_CXX_FLAGS AND NOT CMAKE_CXX_FLAGS STREQUAL "")
    list(APPEND configure_args -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS})
endif()
if (DEFINED CMAKE_EXE_LINKER_FLAGS AND NOT CMAKE_EXE_LINKER_FLAGS STREQUAL "")
    list(APPEND configure_args -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS})
endif()
if (DEFINED CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_TOOLCHAIN_FILE STREQUAL "")
    list(APPEND configure_args -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_output)
if (NOT install_result EQUAL 0)
    message(FATAL_ERROR "install failed:\n${install_output}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${consumer_source}"
        -B "${consumer_build}"
        -DCMAKE_PREFIX_PATH=${prefix}
        ${configure_args}
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE configure_output)
if (NOT configure_result EQUAL 0)
    message(FATAL_ERROR "consumer configure failed:\n${configure_output}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${consumer_build}"
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_output
    ERROR_VARIABLE build_output)
if (NOT build_result EQUAL 0)
    message(FATAL_ERROR "consumer build failed:\n${build_output}")
endif()

execute_process(
    COMMAND "${consumer_build}/install_consumer"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_output
    ERROR_VARIABLE run_output)
if (NOT run_result EQUAL 0)
    message(FATAL_ERROR "consumer run failed:\n${run_output}")
endif()
