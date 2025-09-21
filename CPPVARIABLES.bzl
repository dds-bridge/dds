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
# Controlled with: --define=scheduler=true
# Usage in BUILD files: local_defines = DDS_LOCAL_DEFINES + DDS_SCHEDULER_DEFINE
DDS_SCHEDULER_DEFINE = select({
    "//:scheduler": ["DDS_SCHEDULER"],
    "//conditions:default": [],
})
