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


_IDENT_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def _skip_ws(text: str, i: int, n: int) -> int:
    while i < n and text[i] in " \t\r\n":
        i += 1
    return i


def _iter_call_kwargs(call_names: tuple[str, ...], text: str) -> list[dict[str, str]]:
    """Yield the {keyword: string value} args of each top-level call.

    A single linear scan tracks string-literal and "#" comment state across
    the whole text, so a call name — and each "identifier = value" pair
    inside its body — is only recognized outside both. This matters beyond
    just truncation: a plain regex search over body *text* (even with
    comments stripped) still matches an unrelated argument's *string value*
    that happens to contain assignment-shaped text, e.g.
    `patch_cmds = ['historically name = "toolchains_llvm", ...']` — a
    bazel_dep for a different module would be misread as the toolchains_llvm
    one. Parsing real "identifier = value" pairs only at the call's
    top-level parenthesis depth, with identifier-matching paused for every
    character inside a string or comment, makes that impossible: nothing
    inside a string literal is ever offered to the identifier matcher.
    Only simple `identifier = "string"` / `identifier = 'string'`
    assignments are captured (list/nested-call values are skipped over for
    paren balancing but not parsed), which is everything every caller here
    needs. Whitespace between a call name and "(" is tolerated
    ("archive_override (..." is valid Starlark).
    """
    sorted_names = sorted(call_names, key=len, reverse=True)
    results: list[dict[str, str]] = []
    i = 0
    n = len(text)
    in_string: str | None = None
    in_comment = False
    while i < n:
        ch = text[i]
        if in_comment:
            if ch == "\n":
                in_comment = False
            i += 1
            continue
        if in_string:
            if ch == "\\":
                i += 2
                continue
            if ch == in_string:
                in_string = None
            i += 1
            continue
        if ch == "#":
            in_comment = True
            i += 1
            continue
        if ch in "\"'":
            in_string = ch
            i += 1
            continue
        matched_name = None
        paren_index = None
        for name in sorted_names:
            if text.startswith(name, i):
                j = _skip_ws(text, i + len(name), n)
                if j < n and text[j] == "(":
                    matched_name = name
                    paren_index = j
                    break
        if matched_name is None:
            i += 1
            continue
        i = paren_index + 1
        depth = 1
        kwargs: dict[str, str] = {}
        while i < n and depth > 0:
            ch = text[i]
            if in_comment:
                if ch == "\n":
                    in_comment = False
                i += 1
                continue
            if in_string:
                if ch == "\\":
                    i += 2
                    continue
                if ch == in_string:
                    in_string = None
                i += 1
                continue
            if ch == "#":
                in_comment = True
                i += 1
                continue
            if ch in "\"'":
                in_string = ch
                i += 1
                continue
            if ch == "(":
                depth += 1
                i += 1
                continue
            if ch == ")":
                depth -= 1
                i += 1
                continue
            if depth == 1:
                m = _IDENT_RE.match(text, i)
                if m:
                    ident = m.group(0)
                    j = _skip_ws(text, m.end(), n)
                    if j < n and text[j] == "=" and text[j : j + 2] != "==":
                        j = _skip_ws(text, j + 1, n)
                        if j < n and text[j] in "\"'":
                            quote = text[j]
                            k = j + 1
                            value_chars = []
                            while k < n and text[k] != quote:
                                if text[k] == "\\" and k + 1 < n:
                                    value_chars.append(text[k + 1])
                                    k += 2
                                    continue
                                value_chars.append(text[k])
                                k += 1
                            kwargs[ident] = "".join(value_chars)
                            i = k + 1
                            continue
                    i = m.end()
                    continue
            i += 1
        results.append(kwargs)
    return results


def _toolchains_llvm_bazel_dep_version(module_bazel: str) -> str:
    for kwargs in _iter_call_kwargs(("bazel_dep",), module_bazel):
        if kwargs.get("name") == "toolchains_llvm" and "version" in kwargs:
            return kwargs["version"]
    raise AssertionError('expected bazel_dep(... name = "toolchains_llvm" ...)')


def _has_toolchains_llvm_override(module_bazel: str) -> bool:
    """True if any override pins toolchains_llvm to a specific commit/archive.

    Matches each override call's keyword args regardless of argument order
    or formatting, rather than assuming module_name is the first line after
    the opening "(".
    """
    override_names = (
        "archive_override",
        "git_override",
        "local_path_override",
        "multiple_version_override",
        "single_version_override",
    )
    for kwargs in _iter_call_kwargs(override_names, module_bazel):
        if kwargs.get("module_name") == "toolchains_llvm":
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

    def test_toolchains_llvm_bazel_dep_version_survives_an_earlier_paren(
        self,
    ) -> None:
        """A ")" inside a leading comment must not truncate the call body."""
        sample = """bazel_dep(
    # see upstream fix (closes #791)
    version = "1.11.2",
    name = "toolchains_llvm",
)"""
        self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.11.2")

    def test_toolchains_llvm_bazel_dep_version_ignores_commented_out_call(
        self,
    ) -> None:
        sample = """# bazel_dep(name = "toolchains_llvm", version = "9.9.9")
bazel_dep(name = "toolchains_llvm", version = "1.11.2")
"""
        self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.11.2")

    def test_toolchains_llvm_bazel_dep_version_ignores_commented_out_version(
        self,
    ) -> None:
        """A stale version in a comment inside the call must not win."""
        sample = """bazel_dep(
    name = "toolchains_llvm",
    # version = "9.9.9" (stale)
    version = "1.11.2",
)"""
        self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.11.2")

    def test_toolchains_llvm_bazel_dep_version_ignores_fake_assignment_in_string(
        self,
    ) -> None:
        """Assignment-shaped text inside an unrelated string value for a

        different module's bazel_dep must not be read as real args.
        """
        sample = """bazel_dep(
    name = "rules_cc",
    version = "0.2.26",
    patch_cmds = ['historically name = "toolchains_llvm", version = "9.9.9"'],
)"""
        with self.assertRaises(AssertionError):
            _toolchains_llvm_bazel_dep_version(sample)

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

    def test_has_toolchains_llvm_override_survives_an_earlier_paren(self) -> None:
        """A ")" inside a leading comment must not truncate the call body."""
        sample = """archive_override(
    # pin past #791 (unreleased fix)
    module_name = "toolchains_llvm",
    strip_prefix = "toolchains_llvm-abc123",
)"""
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_commented_out_call(
        self,
    ) -> None:
        sample = (
            '# archive_override(module_name = "toolchains_llvm", '
            'strip_prefix = "x")\n'
        )
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_other_modules(self) -> None:
        sample = """
single_version_override(
    module_name = "apple_support",
    version = "2.10.1",
)
"""
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_commented_module_name(
        self,
    ) -> None:
        """A comment mentioning toolchains_llvm in an apple_support override

        is not itself a toolchains_llvm override.
        """
        sample = """single_version_override(
    # module_name = "toolchains_llvm" (old pin, no longer used)
    module_name = "apple_support",
    version = "2.10.1",
)"""
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_fake_assignment_in_string(
        self,
    ) -> None:
        """Assignment-shaped text inside an unrelated string value for a

        different module's override must not be read as real args.
        """
        sample = """single_version_override(
    module_name = "apple_support",
    version = "2.10.1",
    patch_cmds = ['the module_name = "toolchains_llvm" override is legacy'],
)"""
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_tolerates_whitespace_before_paren(
        self,
    ) -> None:
        sample = (
            'archive_override (module_name = "toolchains_llvm", '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_toolchains_llvm_override(sample))

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
