"Global C++ compilation and link flags"

CPPOPTS = select({
    "//:build_macos": ["-std=c++17", "-stdlib=libc++", "-O3", "-flto", "-mtune=generic", "-fPIC", "-DDDS_THREADS_GCD"],
    "//:debug_build_macos": ["-std=c++17", "-stdlib=libc++", "-g", "-flto", "-mtune=generic", "-fPIC", "-DDDS_THREADS_GCD"],
    "//:build_linux": ["-O3", "-flto", "-fopenmp", "-std=c++17"],
    "//:debug_build_linux": ["-g", "-flto", "-fopenmp", "-std=c++17"],
    "//conditions:default": ["-std=c++11"],
})

LINKOPTS = select({
    "//:build_macos": [],
    "//:debug_build_macos": [],
    "//:build_linux": ["-lbsd"],
    "//:debug_build_linux": ["-lbsd"],
    "//conditions:default": [],
})
