add_executable(dtest
    args.cpp
    compare.cpp
    loop.cpp
    parse.cpp
    print.cpp
    testcommon.cpp
    TestTimer.cpp
    dtest.cpp
)

target_include_directories(dtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dtest PRIVATE dds::dds)

# ============================================================
# CTest definitions
# ============================================================

# dtest always exits 0; detect failures via output patterns.
set(_fail_re "Difference;read_file failed;loop_[a-z]+:.*return")
set(_hands "${CMAKE_SOURCE_DIR}/hands")

foreach(_mode IN ITEMS solve calc par dealerpar play)
    add_test(
        NAME    dtest_${_mode}
        COMMAND dtest -f "${_hands}/list10.txt" -s ${_mode}
    )
    set_tests_properties(dtest_${_mode} PROPERTIES
        FAIL_REGULAR_EXPRESSION "${_fail_re}"
    )
endforeach()

# Extra: GIB-format input (sol10.txt) exercising the calc solver path
add_test(
    NAME    dtest_calc_gib
    COMMAND dtest -f "${_hands}/sol10.txt" -s calc
)
set_tests_properties(dtest_calc_gib PROPERTIES
    FAIL_REGULAR_EXPRESSION "${_fail_re}"
)
