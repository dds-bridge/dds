"Global C++ compilation and link flags"

DDS_CPPOPTS = select({
    "//:build_macos": ["-std=c++17", "-stdlib=libc++", "-O3", "-mtune=generic", "-fPIC", "-DDDS_THREADS_GCD"],
    "//:debug_build_macos": ["-std=c++17", "-stdlib=libc++", "-g", "-mtune=generic", "-fPIC", "-DDDS_THREADS_GCD"],
    "//:build_linux": ["-O3", "-fopenmp", "-std=c++17"],
    "//:debug_build_linux": ["-g", "-fopenmp", "-std=c++17"],
    "//conditions:default": ["-std=c++17"],
})

DDS_LOCAL_DEFINES = select({
    "//:build_macos": ["DDS_THREADS_GCD"],
    "//:debug_build_macos": ["DDS_THREADS_GCD"],
    "//:build_linux": [],
    "//:debug_build_linux": [],
    "//conditions:default": [],
})

DDS_LINKOPTS = select({
    "//:build_macos": [],
    "//:debug_build_macos": [],
    "//:build_linux": ["-lbsd"],
    "//:debug_build_linux": ["-lbsd"],
    "//conditions:default": [],
})
