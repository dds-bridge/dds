"C++ toolchain for Clang installed by Homebrew."

load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
    "with_feature_set",
)

all_compile_actions = [
    ACTION_NAMES.c_compile,
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.linkstamp_compile,
    ACTION_NAMES.assemble,
    ACTION_NAMES.preprocess_assemble,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.cpp_module_compile,
    ACTION_NAMES.cpp_module_codegen,
    ACTION_NAMES.clif_match,
    ACTION_NAMES.lto_backend,
]

all_cpp_compile_actions = [
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.linkstamp_compile,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.cpp_module_compile,
    ACTION_NAMES.cpp_module_codegen,
    ACTION_NAMES.clif_match,
]

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

def _impl(ctx):
    tool_paths = [
        tool_path(
            name = "gcc",
            path = "/opt/homebrew/opt/llvm/bin/clang",
        ),
        tool_path(
            name = "ld",
            path = "/opt/homebrew/opt/llvm/bin/llvm-link",
        ),
        tool_path(
            name = "ar",
            path = "/opt/homebrew/opt/llvm/bin/llvm-ar",
        ),
        tool_path(
            name = "cpp",
            path = "/opt/homebrew/opt/llvm/bin/clang++",
        ),
        tool_path(
            name = "gcov",
            path = "/opt/homebrew/opt/llvm/bin/llvm-cov",
        ),
        tool_path(
            name = "nm",
            path = "/usr/bin/nm",
        ),
        tool_path(
            name = "objdump",
            path = "/opt/homebrew/opt/llvm/bin/llvm-objdump",
        ),
        tool_path(
            name = "strip",
            path = "/opt/homebrew/opt/llvm/bin/llvm-strip",
        ),
    ]

    opt_feature = feature(name = "opt")

    dbg_feature = feature(name = "dbg")

    default_link_flags_feature = feature(   
        name = "default_link_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = all_link_actions,
                flag_groups = ([
                    flag_group(
                        flags = [
                            "-lc++",
                            "-L/opt/homebrew/lib",
                            "-L/opt/homebrew/opt/openssl/lib"
                        ],
                    ),
                ]),
            ),
            flag_set(
                actions = all_link_actions,
                flag_groups = [flag_group(flags = ["-lm"])],
                with_features = [with_feature_set(features = ["dbg"])],
            ),
        ],
    )

    default_compile_flags_feature = feature(
        name = "default_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = all_compile_actions,
                flag_groups = [
                    flag_group(
                        flags = [
                            "-stdlib=libc++",
                        ],
                    ),
                ],
            ),
            flag_set(
                actions = all_compile_actions,
                flag_groups = [flag_group(flags = ["-g"])],
                with_features = [with_feature_set(features = ["dbg"])],
            ),
            flag_set(
                actions = all_compile_actions,
                flag_groups = [
                    flag_group(
                        flags = [
                            "-g0",
                            "-O2",
                            "-DNDEBUG",
                        ],
                    ),
                ],
                with_features = [with_feature_set(features = ["opt"])],
            ),
            flag_set(
                actions = all_cpp_compile_actions + [ACTION_NAMES.lto_backend],
                flag_groups = [flag_group(flags = ["-std=c++20"])],
            ),
        ],
    )

    features = [
        opt_feature,
        dbg_feature,
        default_link_flags_feature,
        default_compile_flags_feature,
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = [
            "/opt/homebrew/opt/llvm/include/c++/v1/",
            "/opt/homebrew/Cellar/llvm@20/20.1.8/include/",
            "/opt/homebrew/Cellar/llvm@20/20.1.8/lib/clang/20/include/",
            "/Library/Developer/CommandLineTools/SDKs/MacOSX14.sdk/usr/include/",
            "/Library/Developer/CommandLineTools/SDKs/MacOSX15.sdk/usr/include/",
        ],
        toolchain_identifier = "brew_clang_toolchain",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "k8",
        target_libc = "unknown",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

brew_clang_toolchain_config = rule(
    implementation = _impl,
    attrs = {
        "library_search_directories": attr.string_list(),
    },
    provides = [CcToolchainConfigInfo],
)