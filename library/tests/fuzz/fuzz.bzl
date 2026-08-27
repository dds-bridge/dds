"""Macro pairing each fuzz harness with a corpus-replay test and a libFuzzer binary."""

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("//:CPPVARIABLES.bzl", "DDS_CPPOPTS", "DDS_LINKOPTS", "DDS_LOCAL_DEFINES")

_HARNESS_DEPS = [
    "//library/src:testable_dds",
    "//library/src/api:api_definitions",
]

def dds_fuzz_harness(name, src, corpus_dir):
    """Define a fuzz harness, its corpus-replay test, and its libFuzzer binary.

    Args:
      name: base name; targets are <name>_harness, <name>_fuzz_corpus_test
        and <name>_fuzz.
      src: the .cpp defining LLVMFuzzerTestOneInput().
      corpus_dir: workspace-relative seed corpus directory, passed to the
        replay driver as a plain path (it walks directories recursively).
    """
    cc_library(
        name = name + "_harness",
        srcs = [src],
        copts = DDS_CPPOPTS,
        linkopts = DDS_LINKOPTS,
        local_defines = DDS_LOCAL_DEFINES,
        deps = _HARNESS_DEPS,
        # The harness exports no symbol the driver references directly.
        alwayslink = True,
    )

    cc_test(
        name = name + "_fuzz_corpus_test",
        size = "small",
        # Runfiles-relative: a cc_test runs with its cwd at the runfiles root.
        args = [native.package_name() + "/" + corpus_dir],
        data = native.glob([corpus_dir + "/**"]),
        copts = DDS_CPPOPTS,
        linkopts = DDS_LINKOPTS,
        local_defines = DDS_LOCAL_DEFINES,
        deps = [
            ":" + name + "_harness",
            ":fuzz_corpus_main",
        ],
    )

    cc_binary(
        name = name + "_fuzz",
        copts = DDS_CPPOPTS,
        linkopts = DDS_LINKOPTS,
        local_defines = DDS_LOCAL_DEFINES,
        # libFuzzer supplies main(); requires --config=fuzz.
        tags = ["manual"],
        deps = [":" + name + "_harness"],
    )
