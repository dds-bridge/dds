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


def _toolchains_llvm_bazel_dep_version(module_bazel: str) -> str:
    for match in re.finditer(r"bazel_dep\(\s*([^)]*)\)", module_bazel):
        body = match.group(1)
        name = re.search(r'name\s*=\s*"toolchains_llvm"', body)
        version = re.search(r'version\s*=\s*"([^"]+)"', body)
        if name and version:
            return version.group(1)
    raise AssertionError('expected bazel_dep(... name = "toolchains_llvm" ...)')


def _has_toolchains_llvm_override(module_bazel: str) -> bool:
    """True if any override pins toolchains_llvm to a specific commit/archive.

    Matches each override call's full argument body regardless of argument
    order or formatting, rather than assuming module_name is the first line
    after the opening "(".
    """
    for call in re.finditer(
        r"(?:archive_override|git_override|local_path_override"
        r"|multiple_version_override|single_version_override)\(\s*([^)]*)\)",
        module_bazel,
    ):
        if re.search(r'module_name\s*=\s*"toolchains_llvm"', call.group(1)):
            return True
    return False


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

    def test_sanitizer_test_timeouts_are_strictly_increasing(self) -> None:
        """--test_timeout is (short, moderate, long, eternal); values must escalate.

        Small solver tests need short=300 under sanitizers (default moderate
        budget) without timeout="moderate", which warns on fast local runs.
        Duplicate short/moderate values collapse the hierarchy.
        """
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        configs = ("asan", "tsan", "ubsan", "msan")
        pattern = re.compile(
            r"(?m)^test:(%s)\s+--test_timeout=(\d+),(\d+),(\d+),(\d+)\s*$"
            % "|".join(configs)
        )
        found = {
            match.group(1): tuple(int(match.group(i)) for i in range(2, 6))
            for match in pattern.finditer(bazelrc)
        }
        self.assertEqual(
            set(found),
            set(configs),
            "expected --test_timeout for asan, tsan, ubsan, and msan",
        )
        timeout_values = set(found.values())
        self.assertEqual(
            len(timeout_values),
            1,
            f"expected identical four-value --test_timeout across sanitizers, got {found}",
        )
        for name, timeouts in found.items():
            short, moderate, long, eternal = timeouts
            self.assertGreaterEqual(
                short,
                300,
                f"test:{name} short timeout must keep sanitizer headroom for size=small",
            )
            self.assertLess(
                short,
                moderate,
                f"test:{name} short must be < moderate ({timeouts})",
            )
            self.assertLess(
                moderate,
                long,
                f"test:{name} moderate must be < long ({timeouts})",
            )
            self.assertLess(
                long,
                eternal,
                f"test:{name} long must be < eternal ({timeouts})",
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

    def test_module_pins_toolchains_llvm_past_unused_stdlib_fix(self) -> None:
        """BCR 1.8.0 emitted unused -stdlib=libc++, breaking Linux -Werror builds.

        toolchains_llvm#791 dropped the redundant flag; it shipped in the 1.9.0
        release, so a plain bazel_dep replaces the archive_override this
        project used to carry to pin a specific pre-release commit.
        """
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        version = tuple(
            int(p) for p in _toolchains_llvm_bazel_dep_version(module).split(".")
        )
        self.assertGreaterEqual(
            version,
            (1, 9, 0),
            "toolchains_llvm must stay >= 1.9.0 (drops unused -stdlib=libc++, #791)",
        )
        self.assertFalse(
            _has_toolchains_llvm_override(module),
            "toolchains_llvm no longer needs an archive_override pin",
        )

    def test_toolchains_llvm_bazel_dep_version_tolerates_reordered_args(
        self,
    ) -> None:
        sample = 'bazel_dep(version = "1.11.2", name = "toolchains_llvm")'
        self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.11.2")

    def test_has_toolchains_llvm_override_detects_same_line_args(self) -> None:
        sample = (
            'archive_override(module_name = "toolchains_llvm", '
            'strip_prefix = "toolchains_llvm-abc123")'
        )
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_reordered_args(self) -> None:
        sample = """
archive_override(
    strip_prefix = "toolchains_llvm-abc123",
    module_name = "toolchains_llvm",
)
"""
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_other_modules(self) -> None:
        sample = """
single_version_override(
    module_name = "apple_support",
    version = "2.10.1",
)
"""
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_local_path_override(self) -> None:
        sample = """
local_path_override(
    module_name = "toolchains_llvm",
    path = "../toolchains_llvm",
)
"""
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_multiple_version_override(
        self,
    ) -> None:
        sample = """
multiple_version_override(
    module_name = "toolchains_llvm",
    versions = ["1.9.0", "1.11.2"],
)
"""
        self.assertTrue(_has_toolchains_llvm_override(sample))


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
        # MODULE.bazel pins the MSAN LLVM + instrumented libc++ to the Ubuntu
        # 22.04 distribution; keep the runner on that OS so the overlay matches.
        self.assertRegex(
            text,
            r"(?m)^  msan:\s*\n(?:[ \t]+[^\n]*\n)*?[ \t]+runs-on:\s*ubuntu-22\.04\s*$",
            "msan CI must pin ubuntu-22.04 to match the MSAN toolchain overlay",
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


class TestMsanCompatibleParallelBoardsAllocHooks(unittest.TestCase):
    def test_custom_new_delete_are_disabled_under_memory_sanitizer(self) -> None:
        """MSAN's runtime owns operator new/delete; custom defs duplicate-symbol.

        parallel_boards_test.cpp overrides global new/delete for allocation
        counting. Those symbols must not be compiled under MemorySanitizer.
        """
        path = (
            _repo_root()
            / "library"
            / "tests"
            / "system"
            / "parallel_boards_test.cpp"
        )
        text = path.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r"void\*\s+operator\s+new\s*\(",
            "expected a custom operator new for allocation tracking",
        )
        self.assertRegex(
            text,
            r"__has_feature\s*\(\s*memory_sanitizer\s*\)",
            "expected MSAN detection via __has_feature(memory_sanitizer)",
        )
        # Both Clang feature-test and GCC-style __SANITIZE_MEMORY__ may be set;
        # defining DDS_TEST_MEMORY_SANITIZER twice triggers -Wmacro-redefined.
        self.assertRegex(
            text,
            r"#\s*ifndef\s+DDS_TEST_MEMORY_SANITIZER[\s\S]*?"
            r"#\s*if\s+defined\s*\(\s*__SANITIZE_MEMORY__\s*\)",
            "second MSAN detect must be guarded to avoid macro redefinition",
        )
        self.assertEqual(
            len(re.findall(r"#\s*define\s+DDS_TEST_MEMORY_SANITIZER\s+1\b", text)),
            2,
            "expected two paths that set DDS_TEST_MEMORY_SANITIZER to 1",
        )
        # Custom replacements must not be linked under MSAN (duplicate symbols).
        self.assertRegex(
            text,
            r"#\s*if\s*!DDS_TEST_MEMORY_SANITIZER[\s\S]*?"
            r"void\*\s+operator\s+new\s*\(",
            "custom operator new must be compiled only when MSAN is off",
        )


if __name__ == "__main__":
    unittest.main()
