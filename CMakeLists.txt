cmake_minimum_required(VERSION 3.16)

project(dds
    VERSION 2.9.0
    DESCRIPTION "Double Dummy Solver for Contract Bridge"
    HOMEPAGE_URL "https://github.com/dds-bridge/dds"
    LANGUAGES CXX
)

# ============================================================
# Build options
# ============================================================

option(BUILD_SHARED_LIBS    "Build shared library instead of static"  ON)

# Threading backends — multiple can be enabled simultaneously.
# The library selects the highest-priority available one at run time
# (priority order: WinAPI > OpenMP > GCD > Boost > STL; see System.cpp).
option(DDS_THREADS_BOOST    "Use Boost threading backend"              OFF)
option(DDS_THREADS_OPENMP   "Use OpenMP threading backend"             OFF)
option(DDS_THREADS_STL      "Use C++ STL threading backend"            OFF)
option(DDS_THREADS_GCD      "Use Grand Central Dispatch (macOS only)"  OFF)
option(DDS_THREADS_WINAPI   "Use Windows API threading (Windows only)" OFF)

# Optional build features
option(DDS_WERROR           "Treat compiler warnings as errors"        ON)
option(DDS_SMALL_MEMORY     "Reduce memory use at cost of speed"       OFF)
option(DDS_ENABLE_DEBUG     "Enable all debug output"                  OFF)
option(DDS_ENABLE_TIMING    "Enable timing statistics"                 OFF)
option(DDS_ENABLE_SCHEDULER "Enable scheduler debug output"            OFF)

# Sub-projects
option(DDS_BUILD_TESTS      "Build test program (dtest)"               OFF)
option(DDS_BUILD_EXAMPLES   "Build example programs"                   OFF)

# ============================================================
# Platform defaults for threading
# Applied only when the user hasn't explicitly requested any backend.
# ============================================================

if(NOT DDS_THREADS_BOOST  AND
   NOT DDS_THREADS_OPENMP AND
   NOT DDS_THREADS_STL    AND
   NOT DDS_THREADS_GCD    AND
   NOT DDS_THREADS_WINAPI)
    if(WIN32)
        set(DDS_THREADS_WINAPI ON)
        set(DDS_THREADS_OPENMP ON)
        set(DDS_THREADS_STL    ON)
    elseif(APPLE)
        set(DDS_THREADS_GCD ON)
        set(DDS_THREADS_STL ON)
    else()
        # Linux / other Unix
        set(DDS_THREADS_OPENMP ON)
        set(DDS_THREADS_STL    ON)
    endif()
endif()

# ============================================================
# Windows resource file (version info embedded in the DLL)
# ============================================================

if(WIN32 AND BUILD_SHARED_LIBS)
    enable_language(RC)
endif()

# ============================================================
# Library source files
# ============================================================

set(DDS_SOURCES
    src/dds.cpp
    src/ABsearch.cpp
    src/ABstats.cpp
    src/CalcTables.cpp
    src/DealerPar.cpp
    src/dump.cpp
    src/File.cpp
    src/Init.cpp
    src/LaterTricks.cpp
    src/Memory.cpp
    src/Moves.cpp
    src/Par.cpp
    src/PlayAnalyser.cpp
    src/PBN.cpp
    src/QuickTricks.cpp
    src/Scheduler.cpp
    src/SolveBoard.cpp
    src/SolverIF.cpp
    src/System.cpp
    src/ThreadMgr.cpp
    src/Timer.cpp
    src/TimerGroup.cpp
    src/TimerList.cpp
    src/TimeStat.cpp
    src/TimeStatList.cpp
    src/TransTableS.cpp
    src/TransTableL.cpp
)

if(WIN32 AND BUILD_SHARED_LIBS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/dds.rc")
    list(APPEND DDS_SOURCES src/dds.rc)
endif()

# ============================================================
# Library target
# ============================================================

add_library(dds ${DDS_SOURCES})
add_library(dds::dds ALIAS dds)

target_include_directories(dds
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# MSVC: use the module-definition file to control DLL exports
if(WIN32 AND BUILD_SHARED_LIBS AND MSVC)
    target_link_options(dds PRIVATE
        "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/Exports.def"
    )
endif()

# ============================================================
# C++ standard
# ============================================================

if(MSVC)
    # C++17 needed for STLIMPL / parallel STL threading backend
    target_compile_features(dds PUBLIC cxx_std_17)
else()
    target_compile_features(dds PUBLIC cxx_std_11)
endif()
set_target_properties(dds PROPERTIES CXX_EXTENSIONS OFF)

# ============================================================
# Link-time optimisation
# ============================================================

include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output)
if(_ipo_supported)
    set_target_properties(dds PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE         TRUE
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO  TRUE
    )
else()
    message(STATUS "IPO/LTO not supported: ${_ipo_output}")
endif()

# ============================================================
# Compiler flags
# ============================================================

if(MSVC)
    target_compile_options(dds PRIVATE
        /O2 /Oi /Ot /Oy /Ox /EHs /Qspectre
        # Aggressive warnings, suppressing those triggered by Windows / STL headers
        /Wall
        /wd4365   # signed/unsigned mismatch
        /wd4464   # relative include path contains '..'
        /wd4514   # unreferenced inline function removed
        /wd4555   # expression has no effect
        /wd4571   # catch(...) semantics changed
        /wd4623   # default constructor implicitly deleted
        /wd4625   # copy constructor implicitly deleted
        /wd4626   # copy assignment operator implicitly deleted
        /wd4668   # symbol not defined as preprocessor macro
        /wd4710   # function not inlined
        /wd4711   # function selected for automatic inlining
        /wd4774   # format string not a string literal
        /wd4820   # padding added after data member
        /wd4996   # deprecated function
        /wd5026   # move constructor implicitly deleted
        /wd5027   # move assignment operator implicitly deleted
        /wd5045   # Spectre mitigation insertion
        $<$<BOOL:${DDS_WERROR}>:/WX>
    )
else()
    # Flags common to GCC and Clang
    target_compile_options(dds PRIVATE
        -O3
        -Wall -Wextra $<$<BOOL:${DDS_WERROR}>:-Werror> -pedantic
        -Wshadow
        -Wsign-conversion
        -Wcast-align
        -Wcast-qual
        -Wctor-dtor-privacy
        -Wdisabled-optimization
        -Winit-self
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wold-style-cast
        -Woverloaded-virtual
        -Wredundant-decls
        -Wsign-promo
        -Wstrict-overflow=1
        -Wswitch-default
        -Wundef
        -Wno-unused
        -Wno-unknown-pragmas
        -Wno-long-long
        -Wno-format
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(dds PRIVATE
            -Wlogical-op
            -Wnoexcept
            -Wstrict-null-sentinel
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(dds PRIVATE
            -Wcomment
        )
    endif()

    # Linker hardening for shared libraries on Linux
    if(UNIX AND NOT APPLE AND BUILD_SHARED_LIBS)
        target_link_options(dds PRIVATE
            -Wl,--sort-common
            -Wl,--as-needed
            -Wl,-z,relro
        )
    endif()
endif()

# ============================================================
# Threading backends
# ============================================================

if(DDS_THREADS_OPENMP)
    find_package(OpenMP REQUIRED)
    target_compile_definitions(dds PRIVATE DDS_THREADS_OPENMP)
    target_link_libraries(dds PRIVATE OpenMP::OpenMP_CXX)
endif()

if(DDS_THREADS_BOOST)
    find_package(Boost REQUIRED COMPONENTS system thread)
    target_compile_definitions(dds PRIVATE DDS_THREADS_BOOST)
    target_link_libraries(dds PRIVATE Boost::system Boost::thread)
endif()

if(DDS_THREADS_STL)
    find_package(Threads REQUIRED)
    target_compile_definitions(dds PRIVATE DDS_THREADS_STL)
    target_link_libraries(dds PRIVATE Threads::Threads)
endif()

if(DDS_THREADS_GCD)
    if(NOT APPLE)
        message(FATAL_ERROR "DDS_THREADS_GCD is available on macOS only")
    endif()
    target_compile_definitions(dds PRIVATE DDS_THREADS_GCD)
endif()

if(DDS_THREADS_WINAPI)
    if(NOT WIN32)
        message(FATAL_ERROR "DDS_THREADS_WINAPI is available on Windows only")
    endif()
    target_compile_definitions(dds PRIVATE DDS_THREADS_WINAPI)
endif()

# ============================================================
# Optional behaviour flags
# ============================================================

if(DDS_SMALL_MEMORY)
    target_compile_definitions(dds PRIVATE SMALL_MEMORY_OPTION)
endif()

if(DDS_ENABLE_DEBUG)
    target_compile_definitions(dds PRIVATE DDS_DEBUG_ALL)
endif()

if(DDS_ENABLE_TIMING)
    target_compile_definitions(dds PRIVATE DDS_TIMING)
endif()

if(DDS_ENABLE_SCHEDULER)
    target_compile_definitions(dds PRIVATE DDS_SCHEDULER)
endif()

# ============================================================
# Sub-projects
# ============================================================

if(DDS_BUILD_TESTS OR DDS_BUILD_EXAMPLES)
    enable_testing()
endif()

if(DDS_BUILD_TESTS)
    add_subdirectory(test)
endif()

if(DDS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================
# Installation
# ============================================================

include(GNUInstallDirs)

install(TARGETS dds
    EXPORT   ddsTargets
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}   # .dll on Windows
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT ddsTargets
    FILE      ddsTargets.cmake
    NAMESPACE dds::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dds
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ddsConfigVersion.cmake"
    VERSION     ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ddsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ddsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dds
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ddsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ddsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dds
)
