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
    # A sibling job header is indented 2 spaces, the same as "  <job>:"
    # itself; only a line indented 3+ spaces (job bodies start at 4) or a
    # blank/whitespace-only line continues this job's block.
    match = re.search(
        rf"(?m)^  {re.escape(job)}:\s*\n((?:[ \t]{{3,}}[^\n]*\n|[ \t]*\n)*)",
        text,
    )
    assert match is not None, f"expected a {job!r} job in ci_linux.yml"
    return match.group(1)


class TestJobBlockScoping(unittest.TestCase):
    """_job_block must stop at the next sibling job, not swallow it.

    Job bodies are indented 4+ spaces; a sibling job header is indented 2
    spaces, the same as the "  <job>:" line _job_block starts from. Matching
    "[ \\t]+" for continuation lines accepts that 2-space indent too, so a
    job added below gcc_arm64 would silently join its block -- and could
    then satisfy (or fail) these guards on another job's commands entirely.
    """

    def test_job_block_stops_at_the_next_sibling_job(self) -> None:
        sample = (
            "jobs:\n"
            "  gcc_arm64:\n"
            "    runs-on: ubuntu-24.04-arm\n"
            "    steps:\n"
            "      - run: bazelisk build //library/...\n"
            "\n"
            "  zzz_added_later:\n"
            "    runs-on: ubuntu-latest\n"
            "    steps:\n"
            "      - run: bazelisk build --extra_toolchains=@llvm_toolchain//:all //...\n"
        )
        block = _job_block(sample, "gcc_arm64")
        self.assertNotIn("zzz_added_later", block)
        self.assertNotIn("--extra_toolchains", block)


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

    def test_gcc_arm64_job_disables_the_disk_cache(self) -> None:
        """Host headers/compiler are undeclared inputs; a cache hit after a

        runner image update (new GCC, new headers) could hide exactly the
        compatibility failure this job exists to catch -- same reasoning
        as the Linux distro CI job's disk-cache opt-out.
        """
        block = _job_block(_workflow(), "gcc_arm64")
        self.assertRegex(
            block,
            r'uses:\s*\./\.github/actions/setup-bazelisk\s*\n\s+with:\s*\n\s+disk-cache:\s*"false"',
        )

    def test_gcc_arm64_job_uploads_test_logs_on_failure(self) -> None:
        block = _job_block(_workflow(), "gcc_arm64")
        self.assertRegex(
            block,
            r"(?m)upload-artifact@[^\n]*\n(?:[ \t]+[^\n]*\n)*?[ \t]+name:\s*bazel-test-logs-linux-gcc-arm64\s*$",
        )


if __name__ == "__main__":
    unittest.main()
