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

function(add_compile_fail_test name source regex)
    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DCC=${CMAKE_C_COMPILER}
            -DINCLUDE_DIR=${PROJECT_SOURCE_DIR}/include
            -DSOURCE_FILE=${PROJECT_SOURCE_DIR}/tests/fixtures/compile_fail/${source}
            -DREGEX=${regex}
            -P ${PROJECT_SOURCE_DIR}/tests/compile_fail.cmake)
endfunction()

add_compile_fail_test(bad_clock bad_clock.c "bad_clock\\.c:[0-9]+:[0-9]+:.*clock")
add_compile_fail_test(bad_hook bad_hook.c "bad_hook\\.c:[0-9]+:[0-9]+:.*hook")
add_compile_fail_test(bad_reset bad_reset.c "bad_reset\\.c:[0-9]+:[0-9]+:.*reset")
add_compile_fail_test(bad_halt bad_halt.c "bad_halt\\.c:[0-9]+:[0-9]+:.*halt")
add_compile_fail_test(bad_format bad_format.c "bad_format\\.c:[0-9]+:[0-9]+:.*format")

add_test(NAME install_consumer
    COMMAND ${CMAKE_COMMAND}
        -DBUILD_DIR=${PROJECT_BINARY_DIR}
        -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -P ${PROJECT_SOURCE_DIR}/tests/install_consumer.cmake)
