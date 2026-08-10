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


def _bazelisk_invocation_has_config_opt(text: str, subcommand: str) -> bool:
    """True if any bazelisk <subcommand> invocation includes --config=opt.

    Flag order after the subcommand is not significant, and a YAML `run:`
    prefix on the same line is allowed.
    """
    pattern = rf"bazelisk\s+{re.escape(subcommand)}\b[^\n]*--config=opt\b"
    return re.search(pattern, text) is not None


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

    def test_windows_bazelrc_keeps_default_cpp_std_feature(self) -> None:
        """rules_cc's default_cpp_std supplies /std:c++17 for every MSVC compile,
        including @googletest and cc_* targets that do not use DDS_CPPOPTS.

        Disabling it leaves those TUs with no language standard; googletest then
        fails with C1189 (C++17 required). DDS_CPPOPTS may still add /std:c++20
        (a benign D9025 override on those targets). Do not put /std in
        build:windows --cxxopt — that leaks into wasm transitions on Windows.
        """
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        self.assertIsNone(
            re.search(
                r"(?m)^build:windows\s+--features=-default_cpp_std\b",
                bazelrc,
            ),
            "build:windows must not disable default_cpp_std (breaks googletest "
            "and targets without DDS_CPPOPTS)",
        )
        self.assertIsNone(
            re.search(r"(?m)^build:windows\s+--cxxopt=/std:", bazelrc),
            "build:windows must not set --cxxopt=/std:... (leaks into wasm)",
        )
        self.assertIsNone(
            re.search(r"(?m)^build:windows\s+--host_cxxopt=/std:", bazelrc),
            "build:windows must not set --host_cxxopt=/std:...; host tools use "
            "default_cpp_std",
        )


class TestBazeliskConfigOptMatching(unittest.TestCase):
    def test_matches_when_config_opt_is_first_flag(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "bazelisk build --config=opt //...",
                "build",
            )
        )

    def test_matches_when_other_flags_precede_config_opt(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "bazelisk build --verbose_failures --config=opt //...",
                "build",
            )
        )

    def test_matches_when_config_opt_is_not_adjacent_to_subcommand(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "    bazelisk test --test_output=errors --config=opt //...",
                "test",
            )
        )

    def test_matches_yaml_run_prefix_on_same_line(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "        run: bazelisk test --verbose_failures --config=opt //...",
                "test",
            )
        )

    def test_rejects_missing_config_opt(self) -> None:
        self.assertFalse(
            _bazelisk_invocation_has_config_opt(
                "bazelisk build --verbose_failures //...",
                "build",
            )
        )

    def test_rejects_config_opt_on_unrelated_subcommand(self) -> None:
        self.assertFalse(
            _bazelisk_invocation_has_config_opt(
                "bazelisk fetch --config=opt //...\nbazelisk build //...",
                "build",
            )
        )


class TestWindowsCiUsesOpt(unittest.TestCase):
    def test_windows_ci_passes_config_opt(self) -> None:
        """Without /O2 in DDS_CPPOPTS, CI must opt in via --config=opt."""
        text = (
            _repo_root() / ".github" / "workflows" / "ci_windows.yml"
        ).read_text(encoding="utf-8")
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(text, "build"),
            "expected Windows CI build to use --config=opt",
        )
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(text, "test"),
            "expected Windows CI test to use --config=opt",
        )


class TestWindowsCppoptsConfigExportsVisibility(unittest.TestCase):
    def test_exports_files_visibility_matches_filegroup(self) -> None:
        """exports_files defaults to public; keep it aligned with the restricted
        windows_cppopts_config_files filegroup so these roots are not world-readable
        labels.
        """
        text = (_repo_root() / "BUILD.bazel").read_text(encoding="utf-8")
        match = re.search(
            r"exports_files\(\s*"
            r"\[\s*"
            r'"\.bazelrc",\s*'
            r'"CPPVARIABLES\.bzl",\s*'
            r'"MODULE\.bazel",\s*'
            r"\]\s*,\s*"
            r"visibility\s*=\s*\[\s*\"//python:__pkg__\"\s*\]\s*,?\s*"
            r"\)",
            text,
            flags=re.DOTALL,
        )
        self.assertIsNotNone(
            match,
            "expected exports_files([.bazelrc, CPPVARIABLES.bzl, MODULE.bazel], "
            'visibility = ["//python:__pkg__"])',
        )


if __name__ == "__main__":
    unittest.main()
