"Global C++ compilation and link flags"

DDS_CPPOPTS = select({
    "//:build_macos": [
        "-std=c++20",
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
        "-std=c++20",
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
        "-fPIC",
        "-std=c++20",
        "-Wpedantic",
        "-Wall",
        "-Werror",
    ],
    "//:debug_build_linux": [
        "-g",
        "-O2",
        "-fPIC",
        "-std=c++20",
        "-Wpedantic",
        "-Wall",
        "-Werror"
    ],
    "//conditions:default": [
        "-std=c++20"
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
}) + select({
    "//:skip_heuristic": ["DDS_SKIP_HEURISTIC"],
    "//conditions:default": [],
}) + select({
    "//:debug_all": ["DDS_DEBUG_ALL"],
    "//conditions:default": [],
})

DDS_LINKOPTS = select({
    "//:build_macos": [],
    "//:debug_build_macos": [],
    "//:build_linux": [],
    "//:debug_build_linux": [],
    "//conditions:default": [],
})

# Per-target define to enable scheduler timing when desired.
# This variable returns ["DDS_SCHEDULER"] for macOS builds by default
# and empty on other platforms. BUILD files may add it to their
# local_defines to enable scheduler timing only for selected targets.
DDS_SCHEDULER_DEFINE = select({
    "//:build_macos": ["DDS_SCHEDULER"],
    "//:debug_build_macos": ["DDS_SCHEDULER"],
    "//:build_linux": [],
    "//:debug_build_linux": [],
    "//conditions:default": [],
})
