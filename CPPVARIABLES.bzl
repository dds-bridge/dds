"Global C++ compilation and link flags"

DDS_CPPOPTS = select({
    "//:build_macos": [
        "-O3",
        "-flto=thin",
        "-mtune=generic",
        "-fPIC",
        "-Wpedantic",
        "-Wall",
        "-Wno-character-conversion",
        "-Werror",
    ],
    "//:debug_build_macos": [
        "-g",
        "-mtune=generic",
        "-fPIC",
        "-Wpedantic",
        "-Wall",
        "-Wno-character-conversion",
        "-Werror",
    ],
    "//:build_linux": [
        "-O3",
        "-fPIC",
        "-Wpedantic",
        "-Wall",
        "-Wno-character-conversion",
        "-Werror",
    ],
    "//:debug_build_linux": [
        "-g",
        "-O2",
        "-fPIC",
        "-Wpedantic",
        "-Wall",
        "-Wno-character-conversion",
        "-Werror",
    ],
    # Optimisation (/O2, /Od) comes from Bazel's compilation_mode — restating
    # them here overrides the toolchain defaults and triggers MSVC D9025.
    "//:build_windows": [
        "/std:c++20",
        "/W4",
        "/WX",
        "/permissive-",
    ],
    "//:debug_build_windows": [
        "/Zi",
        "/std:c++20",
        "/W4",
        "/WX",
        "/permissive-",
    ],
    "//:build_wasm": [
        "-O3",
        "-flto",
        "-std=c++20",
        "-Wpedantic",
        "-Wall",
        "-Werror",
        # -fexceptions must precede -fwasm-exceptions: the toolchain's default
        # -fno-exceptions is otherwise not overridden at the clang frontend
        # level by -fwasm-exceptions alone (it only selects the EH lowering
        # mechanism, not the "exceptions enabled" toggle, in this LLVM build).
        "-fexceptions",
        "-fwasm-exceptions",
    ],
    "//conditions:default": [
        "-std=c++20"
    ],
})

DDS_LOCAL_DEFINES = select({
    "//:build_macos": [],
    "//:debug_build_macos": [],
    "//:build_linux": [],
    "//:debug_build_linux": [],
    "//:build_wasm": [],
    "//conditions:default": [],
}) + select({
    "//:debug_all": ["DDS_DEBUG_ALL"],
    "//conditions:default": [],
}) + select({
    "//:tt_context_ownership": ["DDS_TT_CONTEXT_OWNERSHIP"],
    "//conditions:default": [],
}) + select({
    "//:tt_reset_debug": ["DDS_DEBUG_TT_RESET"],
    "//conditions:default": [],
}) + select({
    "//:ab_stats": ["DDS_AB_STATS"],
    "//conditions:default": [],
})

DDS_LINKOPTS = select({
    "//:build_macos": ["-flto=thin"],
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
