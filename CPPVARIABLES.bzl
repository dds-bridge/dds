"Global C++ compilation and link flags"

DDS_CPPOPTS = select({
    "//:build_macos": [
        "-std=c++17",
        "-stdlib=libc++",
        "-O3",
        "-mtune=generic",
        "-fPIC",
        "-DDDS_THREADS_GCD",
        "-Wpedantic",
        "-Wall",
        "-Werror",
    ],
    "//:debug_build_macos": [
        "-std=c++17",
        "-stdlib=libc++",
        "-O2",
        "-g",
        "-mtune=generic",
        "-fPIC",
        "-DDDS_THREADS_GCD",
        "-Wpedantic",
        "-Wall",
        "-Werror",
    ],
    "//:build_linux": [
        "-O3",
        "-fopenmp",
        "-std=c++17",
        "-Wpedantic",
        "-Wall",
        "-Werror",
    ],
    "//:debug_build_linux": [
        "-g",
        "-O2",
        "-fopenmp",
        "-std=c++17",
        "-Wpedantic",
        "-Wall",
        "-Werror"
    ],
    "//conditions:default": [
        "-std=c++17"
    ],
})

DDS_LOCAL_DEFINES = select({
    "//:build_macos": ["DDS_THREADS_GCD"],
    "//:debug_build_macos": ["DDS_THREADS_GCD"],
    "//:build_linux": [],
    "//:debug_build_linux": [],
    "//conditions:default": [],
}) + select({
    "//:new_heuristic": ["DDS_USE_NEW_HEURISTIC"],
    "//conditions:default": [],
})

DDS_LINKOPTS = select({
    "//:build_macos": [],
    "//:debug_build_macos": [],
    "//:build_linux": [],
    "//:debug_build_linux": [],
    "//conditions:default": [],
})
