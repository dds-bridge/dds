#!/usr/bin/env python3
"""Guard that Linux ARM64 CI builds and tests with the host's GCC toolchain.

MODULE.bazel's llvm_versions has no "linux-aarch64" entry (see
docs/BUILD_SYSTEM.md), so on an arm64 Linux host Bazel falls back to the
autodetected host toolchain -- GCC, not the hermetic clang every other Linux
job uses. That is the only place this project's GCC support is exercised, so
a job on a native arm64 runner is the only way to catch a GCC regression
(e.g. #450) before a contributor hits it by hand on real ARM64 hardware.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() — under `bazel test` this file is often a runfiles
    # symlink into the execroot/source tree, and resolving leaves the runfiles
    # tree where //.github/workflows and other data deps live.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        workflows = parent / ".github" / "workflows"
        if workflows.is_dir() and any(workflows.glob("ci_*.yml")):
            return parent
    raise AssertionError("could not locate repository root from test file path")


def _workflow() -> str:
    return (_repo_root() / ".github" / "workflows" / "ci_linux.yml").read_text(
        encoding="utf-8"
    )


def _job_block(text: str, job: str) -> str:
    match = re.search(
        rf"(?m)^  {re.escape(job)}:\s*\n((?:[ \t]+[^\n]*\n|\n)*)",
        text,
    )
    assert match is not None, f"expected a {job!r} job in ci_linux.yml"
    return match.group(1)


class TestGccArm64Job(unittest.TestCase):
    def test_ci_linux_runs_a_gcc_arm64_job(self) -> None:
        self.assertRegex(
            _workflow(),
            r"(?m)^  gcc_arm64:\s*$",
            "expected a dedicated gcc_arm64 job in ci_linux.yml",
        )

    def test_gcc_arm64_job_runs_on_a_native_arm64_runner(self) -> None:
        block = _job_block(_workflow(), "gcc_arm64")
        self.assertRegex(
            block,
            r"(?m)^\s+runs-on:\s*ubuntu-24\.04-arm\s*$",
            "gcc_arm64 must run on a native arm64 runner, not emulation",
        )

    def test_gcc_arm64_job_does_not_register_a_hermetic_toolchain(self) -> None:
        """The point of this job is the host-GCC fallback path, not clang."""
        block = _job_block(_workflow(), "gcc_arm64")
        self.assertNotRegex(
            block,
            r"--extra_toolchains",
            "gcc_arm64 must exercise the autodetected host GCC toolchain",
        )

    def test_gcc_arm64_job_builds_the_host_toolchain_sensitive_packages(self) -> None:
        """Same scope PR #450 fixed: library, python and utilities sources."""
        block = _job_block(_workflow(), "gcc_arm64")
        self.assertRegex(
            block,
            r"bazelisk\s+build\b[^\n]*//library/\.\.\.[^\n]*//python/\.\.\.[^\n]*//utilities/\.\.\.",
        )

    def test_gcc_arm64_job_tests_the_host_toolchain_sensitive_packages(self) -> None:
        block = _job_block(_workflow(), "gcc_arm64")
        self.assertRegex(
            block,
            r"bazelisk\s+test\b[^\n]*//library/\.\.\.[^\n]*//python/\.\.\.[^\n]*//utilities/\.\.\.",
        )

    def test_gcc_arm64_job_uploads_test_logs_on_failure(self) -> None:
        block = _job_block(_workflow(), "gcc_arm64")
        self.assertRegex(
            block,
            r"(?m)upload-artifact@[^\n]*\n(?:[ \t]+[^\n]*\n)*?[ \t]+name:\s*bazel-test-logs-linux-gcc-arm64\s*$",
        )


if __name__ == "__main__":
    unittest.main()
