if (NOT DEFINED CC OR NOT DEFINED INCLUDE_DIR OR NOT DEFINED SOURCE_FILE OR NOT DEFINED REGEX OR NOT DEFINED COMPILER_ID)
    message(FATAL_ERROR "missing compile_fail inputs")
endif()

get_filename_component(source_name "${SOURCE_FILE}" NAME_WE)
if (COMPILER_ID STREQUAL "MSVC")
    set(output_file "${CMAKE_BINARY_DIR}/compile_fail_${source_name}.obj")
    execute_process(
        COMMAND "${CC}"
            /nologo
            /TC
            /std:c11
            /W4
            /WX
            "/I${INCLUDE_DIR}"
            /c "${SOURCE_FILE}"
            /Fo"${output_file}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output)
else()
    set(output_file "${CMAKE_BINARY_DIR}/compile_fail_${source_name}.o")
    execute_process(
        COMMAND "${CC}"
            -std=c99
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
            -Werror
            -I
            "${INCLUDE_DIR}"
            -c "${SOURCE_FILE}"
            -o "${output_file}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output)
endif()

if (result EQUAL 0)
    message(FATAL_ERROR "compile unexpectedly succeeded for ${SOURCE_FILE}")
endif()

if (NOT output MATCHES "${REGEX}")
    message(FATAL_ERROR "compile output did not match ${REGEX}:\n${output}")
endif()
