#!/usr/bin/env python3
"""Guard that macOS ASAN crosstool helpers do not trip Xcode 27 libc++ warnings."""

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
        if (parent / "MODULE.bazel").is_file() and (
            parent / "patches" / "apple_support_macos_min_11.patch"
        ).is_file():
            return parent
    raise AssertionError("could not locate repository root from test file path")


class TestAppleSupportAsanCrosstoolWarnings(unittest.TestCase):
    def test_module_patches_apple_support_for_xcode27_crosstool(self) -> None:
        """ASAN uses apple_support wrapped_clang/libtool.

        Xcode 27's libc++ emits -W#warnings for -mmacosx-version-min < 11.0, and
        apple_support 1.24.2's libtool.cc has a dead nodiscard hasher call. Keep
        the single_version_override patches until we bump past those fixes
        (upstream >= 2.8.4 for the min-OS bump).
        """
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        self.assertRegex(
            module,
            r'bazel_dep\(\s*name\s*=\s*"apple_support"',
            "expected a direct apple_support bazel_dep for ASAN",
        )
        self.assertRegex(
            module,
            r'single_version_override\(\s*\n\s*module_name\s*=\s*"apple_support"',
            "expected single_version_override for apple_support crosstool patches",
        )
        override = re.search(
            r'single_version_override\(\s*\n\s*module_name\s*=\s*"apple_support"'
            r".*?\)",
            module,
            flags=re.DOTALL,
        )
        self.assertIsNotNone(override, "could not locate apple_support override block")
        block = override.group(0)
        self.assertIn(
            "apple_support_macos_min_11.patch",
            block,
            "override must raise crosstool -mmacosx-version-min to 11.0",
        )
        self.assertIn(
            "apple_support_libtool_nodiscard.patch",
            block,
            "override must drop the dead libtool hasher(file) nodiscard call",
        )

    def test_apple_support_patches_raise_min_os_and_drop_dead_hasher(self) -> None:
        root = _repo_root()
        min_os = (root / "patches" / "apple_support_macos_min_11.patch").read_text(
            encoding="utf-8"
        )
        libtool = (
            root / "patches" / "apple_support_libtool_nodiscard.patch"
        ).read_text(encoding="utf-8")
        self.assertRegex(
            min_os,
            r"(?m)^-\s*-mmacosx-version-min=10\.15\s*\\?\s*$",
            "min-OS patch must remove the pre-Big-Sur deployment target",
        )
        self.assertRegex(
            min_os,
            r"(?m)^\+\s*-mmacosx-version-min=11\.0\s*\\?\s*$",
            "min-OS patch must set -mmacosx-version-min=11.0 for Xcode 27 libc++",
        )
        self.assertIn(
            "-    hasher(file);",
            libtool,
            "libtool patch must remove the dead nodiscard hasher(file) call",
        )


if __name__ == "__main__":
    unittest.main()
