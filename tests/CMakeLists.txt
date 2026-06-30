add_executable(microassert_tests test_all.c)
target_link_libraries(microassert_tests PRIVATE microassert::microassert)
set_target_properties(microassert_tests PROPERTIES
    C_STANDARD ${MICROASSERT_C_STANDARD}
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
microassert_enable_c_warnings(microassert_tests)
add_test(NAME runtime COMMAND microassert_tests)

add_executable(microassert_multi_tu
    fixtures/multi_tu/main.c
    fixtures/multi_tu/a.c
    fixtures/multi_tu/b.c)
target_link_libraries(microassert_multi_tu PRIVATE microassert::microassert)
set_target_properties(microassert_multi_tu PROPERTIES
    C_STANDARD ${MICROASSERT_C_STANDARD}
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
microassert_enable_c_warnings(microassert_multi_tu)
add_test(NAME multi_tu COMMAND microassert_multi_tu)

add_executable(microassert_cpp_consumer
    fixtures/cxx_consumer/main.cpp)
target_link_libraries(microassert_cpp_consumer PRIVATE microassert::microassert)
set_target_properties(microassert_cpp_consumer PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
microassert_enable_cxx_warnings(microassert_cpp_consumer)
add_test(NAME cxx_consumer COMMAND microassert_cpp_consumer)

add_executable(microassert_readme_example
    fixtures/readme_example/main.c)
target_link_libraries(microassert_readme_example PRIVATE microassert::microassert)
set_target_properties(microassert_readme_example PROPERTIES
    C_STANDARD ${MICROASSERT_C_STANDARD}
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
microassert_enable_c_warnings(microassert_readme_example)

function(add_compile_fail_test name source keyword)
    if (MSVC)
        set(regex "${source}\\([0-9]+\\):.*${keyword}")
    else()
        set(regex "${source}:[0-9]+:[0-9]+:.*${keyword}")
    endif()

    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCC=${CMAKE_C_COMPILER}
            -DCOMPILER_ID=${CMAKE_C_COMPILER_ID}
            -DINCLUDE_DIR=${PROJECT_SOURCE_DIR}/include
            -DSOURCE_FILE=${PROJECT_SOURCE_DIR}/tests/fixtures/compile_fail/${source}
            -DREGEX=${regex}
            -P ${PROJECT_SOURCE_DIR}/tests/compile_fail.cmake)
endfunction()

add_compile_fail_test(bad_clock bad_clock.c "clock")
add_compile_fail_test(bad_hook bad_hook.c "hook")
add_compile_fail_test(bad_reset bad_reset.c "reset")
add_compile_fail_test(bad_halt bad_halt.c "halt")
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_fail_test(bad_format bad_format.c "format")
endif()

add_test(NAME install_consumer
    COMMAND ${CMAKE_COMMAND}
        -DBUILD_DIR=${PROJECT_BINARY_DIR}
        -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DGENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -P ${PROJECT_SOURCE_DIR}/tests/install_consumer.cmake)
