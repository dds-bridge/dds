#!/usr/bin/env python3
"""Guard that MemorySanitizer is wired into Bazel config and Linux CI."""

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


class TestMsanBazelConfig(unittest.TestCase):
    def test_bazelrc_defines_msan_config(self) -> None:
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        self.assertRegex(
            bazelrc,
            r"(?m)^build:msan\s+--features=msan\b",
            "expected build:msan to enable the toolchains_llvm msan feature",
        )
        self.assertRegex(
            bazelrc,
            r"(?m)^build:msan\s+--extra_toolchains=@llvm_toolchain_msan//:all\b",
            "expected build:msan to select the instrumented-libc++ toolchain",
        )
        self.assertRegex(
            bazelrc,
            r"(?m)^test:msan\s+--test_timeout=",
            "expected test:msan timeouts like the other sanitizer configs",
        )

    def test_module_defines_msan_toolchain_with_instrumented_libcxx(self) -> None:
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        self.assertIn(
            'name = "llvm_toolchain_msan"',
            module,
            "expected a dedicated llvm_toolchain_msan for instrumented libc++",
        )
        self.assertRegex(
            module,
            r"libcxx_url\s*=",
            "msan requires an instrumented libc++ archive via libcxx_url",
        )
        self.assertRegex(
            module,
            r"libcxx_sha256\s*=",
            "msan libcxx_url must be pinned with libcxx_sha256",
        )


class TestMsanLinuxCi(unittest.TestCase):
    def test_ci_linux_runs_msan_job(self) -> None:
        text = (_repo_root() / ".github" / "workflows" / "ci_linux.yml").read_text(
            encoding="utf-8"
        )
        self.assertRegex(
            text,
            r"(?m)^  msan:\s*$",
            "expected a dedicated msan job in ci_linux.yml",
        )
        self.assertRegex(
            text,
            r"bazelisk\s+test\s+--config=msan\b",
            "expected Linux CI to run tests under --config=msan",
        )
        # MSAN is Linux-only; keep the job scoped to library tests like ASAN/UBSAN.
        self.assertRegex(
            text,
            r"bazelisk\s+test\s+--config=msan\b[^\n]*//library/tests/",
            "expected msan CI to exercise //library/tests/...",
        )


class TestMsanNotOnMacosCi(unittest.TestCase):
    def test_ci_macos_does_not_run_msan(self) -> None:
        text = (_repo_root() / ".github" / "workflows" / "ci_macos.yml").read_text(
            encoding="utf-8"
        )
        self.assertIsNone(
            re.search(r"--config=msan\b", text),
            "MSAN is Linux-only; macOS CI must not enable --config=msan",
        )


if __name__ == "__main__":
    unittest.main()
