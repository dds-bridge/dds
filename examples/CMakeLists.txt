set(DDS_EXAMPLES
    AnalyseAllPlaysBin
    AnalyseAllPlaysPBN
    AnalysePlayBin
    AnalysePlayPBN
    CalcAllTables
    CalcAllTablesPBN
    CalcDDtable
    CalcDDtablePBN
    DealerPar
    Par
    SolveAllBoards
    SolveBoard
    SolveBoardPBN
)

foreach(example IN LISTS DDS_EXAMPLES)
    add_executable(${example} ${example}.cpp hands.cpp)
    target_include_directories(${example} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${example} PRIVATE dds::dds)
endforeach()

# ============================================================
# CTest definitions
# ============================================================

# Examples always exit 0; detect failures via output patterns.
foreach(example IN LISTS DDS_EXAMPLES)
    add_test(NAME example_${example} COMMAND ${example})
    set_tests_properties(example_${example} PROPERTIES
        FAIL_REGULAR_EXPRESSION "ERROR;DDS error"
    )
endforeach()
