#!/usr/bin/env python3
"""Guard that Windows MSVC flags do not fight Bazel's defaults (D9025)."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() — under `bazel test` this file is often a runfiles
    # symlink into the execroot/source tree, and resolving leaves the runfiles
    # tree where //.bazelrc and other data deps live.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        if (parent / ".bazelrc").is_file() and (parent / "CPPVARIABLES.bzl").is_file():
            return parent
    raise AssertionError("could not locate repository root from test file path")


def _windows_cppopts_block(cppvariables: str) -> str:
    """Return the string list body for the //:build_windows select arm."""
    match = re.search(
        r'"//:build_windows":\s*\[(.*?)\]',
        cppvariables,
        flags=re.DOTALL,
    )
    if match is None:
        raise AssertionError("expected //:build_windows arm in DDS_CPPOPTS")
    return match.group(1)


def _debug_windows_cppopts_block(cppvariables: str) -> str:
    match = re.search(
        r'"//:debug_build_windows":\s*\[(.*?)\]',
        cppvariables,
        flags=re.DOTALL,
    )
    if match is None:
        raise AssertionError("expected //:debug_build_windows arm in DDS_CPPOPTS")
    return match.group(1)


class TestWindowsMsvcCppoptsAvoidD9025(unittest.TestCase):
    def test_windows_cppopts_do_not_override_bazel_optimization(self) -> None:
        """Bazel already sets /Od (fastbuild/dbg) or /O2 (opt); re-stating them
        in DDS_CPPOPTS produces cl D9025 ('overriding /Od with /O2').
        """
        text = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        for label, block in (
            ("build_windows", _windows_cppopts_block(text)),
            ("debug_build_windows", _debug_windows_cppopts_block(text)),
        ):
            self.assertNotRegex(
                block,
                r'"/O[d2]"',
                f"{label} must not set /Od or /O2; use compilation_mode instead",
            )

    def test_windows_cppopts_still_request_cxx20(self) -> None:
        text = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        for label, block in (
            ("build_windows", _windows_cppopts_block(text)),
            ("debug_build_windows", _debug_windows_cppopts_block(text)),
        ):
            self.assertIn(
                '"/std:c++20"',
                block,
                f"{label} must keep /std:c++20 (MSVC-only; not via build:windows "
                "cxxopt, which would leak into wasm transitions on Windows hosts)",
            )

    def test_windows_bazelrc_disables_default_cpp_std_feature(self) -> None:
        """rules_cc injects /std:c++17 via default_cpp_std; without disabling it,
        Windows DDS_CPPOPTS /std:c++20 yields cl D9025.
        """
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        self.assertRegex(
            bazelrc,
            r"(?m)^build:windows\s+--features=-default_cpp_std\b",
            "expected build:windows to disable default_cpp_std so /std:c++20 "
            "in DDS_CPPOPTS is the only language standard flag",
        )

    def test_windows_bazelrc_sets_host_cxx20_without_target_cxxopt(self) -> None:
        """Host tools still need C++20 after -default_cpp_std; target /std must
        stay out of build:windows --cxxopt so emscripten wasm builds are safe.
        """
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        self.assertRegex(
            bazelrc,
            r"(?m)^build:windows\s+--host_cxxopt=/std:c\+\+20\b",
            "expected build:windows host_cxxopt=/std:c++20 for host tools",
        )
        self.assertIsNone(
            re.search(r"(?m)^build:windows\s+--cxxopt=/std:", bazelrc),
            "build:windows must not set --cxxopt=/std:... (leaks into wasm)",
        )


class TestWindowsCiUsesOpt(unittest.TestCase):
    def test_windows_ci_passes_config_opt(self) -> None:
        """Without /O2 in DDS_CPPOPTS, CI must opt in via --config=opt."""
        text = (
            _repo_root() / ".github" / "workflows" / "ci_windows.yml"
        ).read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r"bazelisk\s+build\s+--config=opt\b",
            "expected Windows CI build to use --config=opt",
        )
        self.assertRegex(
            text,
            r"bazelisk\s+test\s+--config=opt\b",
            "expected Windows CI test to use --config=opt",
        )


if __name__ == "__main__":
    unittest.main()
