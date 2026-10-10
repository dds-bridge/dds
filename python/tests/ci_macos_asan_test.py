#!/usr/bin/env python3
"""Guard that apple_support stays past the release that fixed Xcode 27 ASAN
crosstool warnings, without re-adding the patches that release made obsolete.
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


def _apple_support_bazel_dep_version(module_bazel: str) -> str:
    for match in re.finditer(r"bazel_dep\(\s*([^)]*)\)", module_bazel):
        body = match.group(1)
        name = re.search(r'name\s*=\s*"apple_support"', body)
        version = re.search(r'version\s*=\s*"([^"]+)"', body)
        if name and version:
            return version.group(1)
    raise AssertionError('expected bazel_dep(... name = "apple_support" ...)')


def _has_apple_support_override(module_bazel: str) -> bool:
    """True if any override pins apple_support to a specific commit/version set.

    Matches each override call's full argument body regardless of argument
    order or formatting, rather than assuming module_name is the first line
    after the opening "(", and covers all five Bazel module override
    directives, not just single_version_override.
    """
    for call in re.finditer(
        r"(?:archive_override|git_override|local_path_override"
        r"|multiple_version_override|single_version_override)\(\s*([^)]*)\)",
        module_bazel,
    ):
        if re.search(r'module_name\s*=\s*"apple_support"', call.group(1)):
            return True
    return False


class TestAppleSupportAsanCrosstoolWarnings(unittest.TestCase):
    def test_module_pins_apple_support_past_xcode27_crosstool_fixes(self) -> None:
        """apple_support < 2.8.4 needs -mmacosx-version-min=11.0 and a dead
        nodiscard hasher(file) call patched out of its wrapped_clang/libtool
        crosstool helpers (used by --config=asan). Both are fixed upstream as
        of 2.8.4, so a plain bazel_dep replaces the single_version_override
        this project used to carry.
        """
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        version = tuple(
            int(p) for p in _apple_support_bazel_dep_version(module).split(".")
        )
        self.assertGreaterEqual(
            version,
            (2, 8, 4),
            "apple_support must stay >= 2.8.4 (Xcode 27 ASAN crosstool fixes)",
        )
        self.assertFalse(
            _has_apple_support_override(module),
            "apple_support no longer needs a single_version_override/patches",
        )

    def test_apple_support_patches_were_removed(self) -> None:
        root = _repo_root()
        self.assertFalse(
            (root / "patches" / "apple_support_macos_min_11.patch").exists(),
            "min-OS patch is obsolete once apple_support >= 2.8.4 is pinned",
        )
        self.assertFalse(
            (root / "patches" / "apple_support_libtool_nodiscard.patch").exists(),
            "libtool nodiscard patch is obsolete once apple_support >= 2.8.4 is pinned",
        )

    def test_apple_support_bazel_dep_version_tolerates_reordered_args(
        self,
    ) -> None:
        sample = 'bazel_dep(version = "2.10.1", name = "apple_support")'
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_has_apple_support_override_detects_same_line_args(self) -> None:
        sample = (
            'single_version_override(module_name = "apple_support", version = "1.24.2")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_reordered_args(self) -> None:
        sample = """
single_version_override(
    patches = ["//:patches/apple_support_macos_min_11.patch"],
    version = "1.24.2",
    module_name = "apple_support",
)
"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_other_modules(self) -> None:
        sample = """
single_version_override(
    module_name = "rules_cc",
    version = "0.2.26",
)
"""
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_archive_override(self) -> None:
        sample = """
archive_override(
    module_name = "apple_support",
    urls = ["https://example.com/apple_support.tar.gz"],
)
"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_local_path_override(self) -> None:
        sample = """
local_path_override(
    module_name = "apple_support",
    path = "../apple_support",
)
"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_multiple_version_override(
        self,
    ) -> None:
        sample = """
multiple_version_override(
    module_name = "apple_support",
    versions = ["1.24.2", "2.10.1"],
)
"""
        self.assertTrue(_has_apple_support_override(sample))


if __name__ == "__main__":
    unittest.main()
