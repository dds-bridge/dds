#!/usr/bin/env python3
"""Guard that native Darwin/Linux links route through the host linker.

The macOS 27 SDK lists `arm64e.x1` in system `.tbd` stubs. LLVM 21's
`ld64.lld` rejects those files ("unknown architecture arm64e.x1") and then
reports a cascade of missing libc++ / libSystem symbols. `toolchains_llvm`'s
`llvm.toolchain` exposes `extra_link_flags` precisely to override the
toolchain-default `--ld-path` per exec/target pair without patching
`toolchains_llvm` itself; the last `--ld-path` wins, so pointing it at
`/usr/bin/ld` (Apple's own linker, which always understands its own SDK's
`.tbd` format) fixes the new SDK without breaking the previous one. The
upstream LLVM Linux archive's `ld.lld` has an unrelated but same-shaped
problem (it needs `libxml2.so.2`, missing on newer distros), fixed the same
way. Apple's/GNU's native `ld` does not understand lld's
`--start-lib`/`--end-lib`, so `.bazelrc` disables the `supports_start_end_lib`
feature on both platforms.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() — under `bazel test` this file is often a runfiles
    # symlink into the execroot/source tree, and resolving leaves the runfiles
    # tree where //MODULE.bazel and other data deps live.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        if (parent / "MODULE.bazel").is_file() and (parent / ".bazelrc").is_file():
            return parent
    raise AssertionError("could not locate repository root from test file path")


class TestNativeLinkerOverride(unittest.TestCase):
    def test_module_overrides_ld_path_for_darwin_and_linux(self) -> None:
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        toolchain = re.search(
            r'llvm\.toolchain\(\s*\n\s*name\s*=\s*"llvm_toolchain"[\s\S]*?\n\)',
            module,
        )
        self.assertIsNotNone(
            toolchain, "could not locate the llvm_toolchain llvm.toolchain() call"
        )
        block = toolchain.group(0)
        self.assertRegex(
            block,
            r'extra_link_flags\s*=\s*\{[\s\S]*?"darwin-aarch64"\s*:\s*\[\s*"--ld-path=/usr/bin/ld"',
            "expected darwin-aarch64 to override --ld-path to Apple's native linker",
        )
        self.assertRegex(
            block,
            r'extra_link_flags\s*=\s*\{[\s\S]*?"darwin-x86_64"\s*:\s*\[\s*"--ld-path=/usr/bin/ld"',
            "expected darwin-x86_64 to override --ld-path to Apple's native linker",
        )
        self.assertRegex(
            block,
            r'extra_link_flags\s*=\s*\{[\s\S]*?"linux-x86_64"\s*:\s*\[\s*"--ld-path=/usr/bin/ld"',
            "expected linux-x86_64 to override --ld-path to the host GNU linker",
        )

    def test_module_relinks_runtime_archives_in_order_for_linux(self) -> None:
        """GNU ld resolves static archives in command-line order; lld does not.

        The toolchain places -l:libunwind.a among the early link flags, before
        the libc++abi.a that needs _Unwind_*, so extra_link_libs must repeat
        libc++.a, libc++abi.a and libunwind.a in dependency order at the end.
        """
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        self.assertRegex(
            module,
            r'extra_link_libs\s*=\s*\{\s*\n\s*"linux-x86_64"\s*:\s*\[\s*\n'
            r'\s*"-l:libc\+\+\.a"\s*,\s*\n'
            r'\s*"-l:libc\+\+abi\.a"\s*,\s*\n'
            r'\s*"-l:libunwind\.a"\s*,',
            "expected libc++, libc++abi, libunwind relinked in that order for Linux",
        )

    def test_bazelrc_disables_start_end_lib_on_macos_and_linux(self) -> None:
        """Apple's/GNU's native ld rejects lld's --start-lib/--end-lib."""
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        self.assertRegex(
            bazelrc,
            r"(?m)^build:macos\s+--features=-supports_start_end_lib\b",
            "expected build:macos to disable supports_start_end_lib",
        )
        self.assertRegex(
            bazelrc,
            r"(?m)^build:linux\s+--features=-supports_start_end_lib\b",
            "expected build:linux to disable supports_start_end_lib",
        )

    def test_macos_keeps_thin_lto_at_compile_and_link(self) -> None:
        """Swapping ld64.lld for Apple's native ld risks -flto=thin silently

        becoming a no-op: classic ld64 needed an explicit -lto_library plugin
        flag to do LTO, which this change does not supply. Verified on Xcode
        27.0 / macOS 26.6.2 that Apple's current linker has LTO support built
        in (see docs/BUILD_SYSTEM.md), but nothing else catches a future
        regression, so guard that the flag itself is still requested at both
        compile and link time for macOS.
        """
        cppvariables = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        cppopts = re.search(
            r'DDS_CPPOPTS\s*=\s*select\(\{\s*\n\s*"//:build_macos"\s*:\s*\[[\s\S]*?\]',
            cppvariables,
        )
        self.assertIsNotNone(cppopts, "could not locate DDS_CPPOPTS //:build_macos")
        self.assertIn(
            '"-flto=thin"',
            cppopts.group(0),
            "expected DDS_CPPOPTS to keep requesting ThinLTO at compile time for macOS",
        )
        linkopts = re.search(
            r'DDS_LINKOPTS\s*=\s*select\(\{\s*\n\s*"//:build_macos"\s*:\s*\[[\s\S]*?\]',
            cppvariables,
        )
        self.assertIsNotNone(linkopts, "could not locate DDS_LINKOPTS //:build_macos")
        self.assertIn(
            '"-flto=thin"',
            linkopts.group(0),
            "expected DDS_LINKOPTS to keep requesting ThinLTO at link time for macOS",
        )


if __name__ == "__main__":
    unittest.main()
