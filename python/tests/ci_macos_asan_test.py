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


def _iter_call_bodies(call_names: tuple[str, ...], text: str) -> list[str]:
    """Yield the argument-list text of each top-level call to call_names.

    A single linear scan tracks string-literal and "#" comment state across
    the whole text, so a call name is only recognized outside both — a
    commented-out "bazel_dep(...)" line is correctly ignored, rather than a
    plain regex search finding the name text inside the comment and reading
    its (ignored-by-Bazel) args as if they were live. The same running state
    continues through each matched call's body, so a ")" inside a string,
    comment, or nested call (e.g. select(...)) never ends the body early —
    it always spans to its real matching ")".
    """
    sorted_names = sorted(call_names, key=len, reverse=True)
    bodies: list[str] = []
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
        matched_name = next(
            (
                name
                for name in sorted_names
                if text.startswith(name, i) and text[i + len(name) : i + len(name) + 1] == "("
            ),
            None,
        )
        if matched_name is None:
            i += 1
            continue
        i += len(matched_name) + 1
        depth = 1
        body_start = i
        while i < n and depth > 0:
            bch = text[i]
            if in_comment:
                if bch == "\n":
                    in_comment = False
            elif in_string:
                if bch == "\\":
                    i += 1
                elif bch == in_string:
                    in_string = None
            elif bch == "#":
                in_comment = True
            elif bch in "\"'":
                in_string = bch
            elif bch == "(":
                depth += 1
            elif bch == ")":
                depth -= 1
            i += 1
        bodies.append(text[body_start : i - 1])
    return bodies


def _apple_support_bazel_dep_version(module_bazel: str) -> str:
    for body in _iter_call_bodies(("bazel_dep",), module_bazel):
        name = re.search(r'name\s*=\s*"apple_support"', body)
        version = re.search(r'version\s*=\s*"([^"]+)"', body)
        if name and version:
            return version.group(1)
    raise AssertionError('expected bazel_dep(... name = "apple_support" ...)')


def _has_apple_support_override(module_bazel: str) -> bool:
    """True if any Bazel module override directive targets apple_support.

    Covers all five override directives (archive_override, git_override,
    local_path_override, multiple_version_override, single_version_override)
    — not just single_version_override — and matches each call's full
    argument body regardless of argument order or formatting, rather than
    assuming module_name is the first line after the opening "(".
    """
    override_names = (
        "archive_override",
        "git_override",
        "local_path_override",
        "multiple_version_override",
        "single_version_override",
    )
    for body in _iter_call_bodies(override_names, module_bazel):
        if re.search(r'module_name\s*=\s*"apple_support"', body):
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

    def test_apple_support_bazel_dep_version_survives_an_earlier_paren(
        self,
    ) -> None:
        """A ")" inside a leading comment must not truncate the call body."""
        sample = """bazel_dep(
    # fixes Xcode 27 crosstool warnings (see docs)
    version = "2.10.1",
    name = "apple_support",
)"""
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_apple_support_bazel_dep_version_ignores_commented_out_call(
        self,
    ) -> None:
        sample = """# bazel_dep(name = "apple_support", version = "9.9.9")
bazel_dep(name = "apple_support", version = "2.10.1")
"""
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

    def test_has_apple_support_override_survives_an_earlier_paren(self) -> None:
        """A ")" inside a leading comment must not truncate the call body."""
        sample = """single_version_override(
    # pin past the Xcode 27 fixes (see docs)
    module_name = "apple_support",
    version = "1.24.2",
)"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_commented_out_call(self) -> None:
        sample = (
            '# archive_override(module_name = "apple_support", '
            'strip_prefix = "x")\n'
        )
        self.assertFalse(_has_apple_support_override(sample))

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

    def test_has_apple_support_override_detects_git_override(self) -> None:
        sample = """
git_override(
    module_name = "apple_support",
    remote = "https://github.com/bazelbuild/apple_support",
    commit = "abc123",
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
