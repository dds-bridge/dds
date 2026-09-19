#!/usr/bin/env python3
"""Guard C++ sources use 4-space indentation and no hard tabs.

Matches .github/instructions/cpp.instructions.md (Indentation):
- 4 spaces per indentation level
- No hard tabs

Copyright block-comment lines that use a decorative 3-space indent are allowed
(leading length % 4 == 3). Continuation alignments that leave a residual
1-space indent (leading length % 4 == 1) are also allowed. What is not allowed
is a 2-space indent level (leading length % 4 == 2).

Hermetic ``bazel test //python:cpp_indentation_test`` covers the helpers and
the CI wiring check. The full-tree scan runs against a real checkout via
``python3 python/tests/cpp_indentation_test.py`` (local or Linux CI), so we
do not need per-package Bazel filegroups of every C++ source.
"""

from __future__ import annotations

import re
import sys
import tempfile
import unittest
from pathlib import Path


_CPP_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"}
_SCAN_ROOTS = (
    "benchmarks",
    "examples",
    "include",
    "jni",
    "library",
    "python",
    "utilities",
    "wasm",
    "web",
)
_SKIP_DIR_NAMES = {
    ".git",
    "bazel-bin",
    "bazel-out",
    "bazel-testlogs",
    "node_modules",
    "third_party",
    "external",
}
_MIN_REPO_CPP_FILES = 50
_WORKSPACE_INDENT_CHECK = "python3 python/tests/cpp_indentation_test.py"


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() — under `bazel test` this file is often a runfiles
    # symlink into the execroot/source tree, and resolving leaves the runfiles
    # tree where data deps live.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        if (parent / ".bazelrc").is_file() and (parent / "CPPVARIABLES.bzl").is_file():
            return parent
    raise AssertionError("could not locate repository root from test file path")


def iter_cpp_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for name in _SCAN_ROOTS:
        base = root / name
        if not base.is_dir():
            continue
        for path in base.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in _CPP_SUFFIXES:
                continue
            try:
                relative_parts = path.relative_to(root).parts
            except ValueError:
                continue
            if any(part in _SKIP_DIR_NAMES for part in relative_parts):
                continue
            files.append(path)
    return sorted(files)


def expand_tabs(line: str, tab_width: int = 4) -> str:
    """Expand tabs to spaces using column-aware tab stops."""
    out: list[str] = []
    col = 0
    for ch in line:
        if ch == "\t":
            spaces = tab_width - (col % tab_width)
            out.append(" " * spaces)
            col += spaces
        else:
            out.append(ch)
            col += 1
    return "".join(out)


def leading_spaces(line: str) -> tuple[int, str]:
    """Return (leading_space_count, remainder) after tab expansion."""
    expanded = expand_tabs(line)
    match = re.match(r"^( *)", expanded)
    assert match is not None
    n = len(match.group(1))
    return n, expanded[n:]


def find_indent_violations(text: str, path: str = "<memory>") -> list[str]:
    """Return human-readable violations for one file's contents."""
    violations: list[str] = []
    for lineno, raw in enumerate(text.splitlines(), 1):
        if "\t" in raw:
            violations.append(f"{path}:{lineno}: hard tab")
            continue
        if not raw.strip():
            continue
        n, _ = leading_spaces(raw)
        if n % 4 == 2:
            violations.append(
                f"{path}:{lineno}: leading indent {n} is not a multiple of 4 "
                f"(2-space indent level)"
            )
    return violations


def _count_exact_two_space_indents(text: str) -> int:
    count = 0
    for raw in text.splitlines():
        if "\t" in raw or not raw.strip():
            continue
        n, _ = leading_spaces(raw)
        if n == 2:
            count += 1
    return count


def reindent_text(text: str) -> str:
    """Normalize a C++ source string to 4-space indentation and no tabs.

    Files that still use 2-space indent levels (many lines with exactly two
    leading spaces) have every leading run of spaces doubled. Remaining lines
    whose leading length is 2 mod 4 (for example half-indented ``public:``)
    are padded up by two spaces to the next multiple of 4.
    """
    newline = "\r\n" if "\r\n" in text else "\n"
    body = text.splitlines()

    # Decide doubling from the tab-expanded view of the original text.
    expanded_view = "\n".join(expand_tabs(line) for line in body)
    double = _count_exact_two_space_indents(expanded_view) >= 3

    out_lines: list[str] = []
    for raw in body:
        if not raw.strip():
            out_lines.append("")
            continue
        expanded = expand_tabs(raw)
        n, rest = leading_spaces(expanded)
        if double and n % 2 == 0:
            # Double even indents only. Odd decorative indents (copyright
            # banners at 3 spaces, some aligned continuations) stay put.
            n *= 2
        elif n % 4 == 2:
            n += 2
        out_lines.append(" " * n + rest)

    result = newline.join(out_lines)
    if text.endswith(("\n", "\r\n")):
        result += newline
    return result


class TestExpandTabs(unittest.TestCase):
    def test_leading_tab_becomes_four_spaces(self) -> None:
        self.assertEqual(expand_tabs("\tfoo"), "    foo")

    def test_tab_after_spaces_aligns_to_stop(self) -> None:
        self.assertEqual(expand_tabs("  \tfoo"), "    foo")


class TestFindIndentViolations(unittest.TestCase):
    def test_reports_hard_tab(self) -> None:
        violations = find_indent_violations("int main() {\n\treturn 0;\n}\n")
        self.assertEqual(len(violations), 1)
        self.assertIn("hard tab", violations[0])

    def test_reports_two_space_indent_level(self) -> None:
        sample = "void f()\n{\n  int x = 1;\n  if (x) {\n    return;\n  }\n}\n"
        violations = find_indent_violations(sample)
        self.assertTrue(any("indent 2" in v for v in violations))
        self.assertTrue(any("indent 6" in v for v in violations) or any("indent 2" in v for v in violations))

    def test_allows_four_space_indent_and_copyright_three_space(self) -> None:
        sample = (
            "/*\n"
            "   Copyright\n"
            "*/\n"
            "void f()\n"
            "{\n"
            "    int x = 1;\n"
            "    if (x) {\n"
            "        return;\n"
            "    }\n"
            "}\n"
        )
        self.assertEqual(find_indent_violations(sample), [])


class TestReindentText(unittest.TestCase):
    def test_doubles_two_space_indent_levels(self) -> None:
        sample = "void f()\n{\n  int x = 1;\n  if (x) {\n    nested();\n  }\n}\n"
        fixed = reindent_text(sample)
        self.assertEqual(find_indent_violations(fixed), [])
        self.assertIn("    int x = 1;", fixed)
        self.assertIn("        nested();", fixed)

    def test_expands_tabs_to_spaces(self) -> None:
        sample = "void f()\n{\n\treturn;\n}\n"
        fixed = reindent_text(sample)
        self.assertEqual(find_indent_violations(fixed), [])
        self.assertIn("    return;", fixed)
        self.assertNotIn("\t", fixed)

    def test_pads_half_indented_access_specifier_in_four_space_file(self) -> None:
        sample = (
            "class File\n"
            "{\n"
            "  private:\n"
            "\n"
            "    std::string fname_;\n"
            "};\n"
        )
        fixed = reindent_text(sample)
        self.assertEqual(find_indent_violations(fixed), [])
        self.assertIn("    private:", fixed)
        self.assertIn("    std::string fname_;", fixed)

    def test_preserves_three_space_copyright_when_doubling(self) -> None:
        sample = (
            "/*\n"
            "   Copyright\n"
            "*/\n"
            "void f()\n"
            "{\n"
            "  int a;\n"
            "  int b;\n"
            "  int c;\n"
            "}\n"
        )
        fixed = reindent_text(sample)
        self.assertEqual(find_indent_violations(fixed), [])
        self.assertIn("   Copyright", fixed)
        self.assertIn("    int a;", fixed)


class TestRepoCppIndentation(unittest.TestCase):
    def test_all_cpp_sources_use_four_space_indent_without_tabs(self) -> None:
        root = _repo_root()
        files = iter_cpp_files(root)
        if len(files) < _MIN_REPO_CPP_FILES:
            self.skipTest(
                "full-tree scan requires a workspace checkout "
                f"(found {len(files)} C++ files; run "
                f"`{_WORKSPACE_INDENT_CHECK}`)"
            )

        violations: list[str] = []
        for path in files:
            text = path.read_text(encoding="utf-8", errors="replace")
            rel = path.relative_to(root).as_posix()
            violations.extend(find_indent_violations(text, rel))

        if violations:
            preview = "\n".join(violations[:40])
            more = "" if len(violations) <= 40 else f"\n... and {len(violations) - 40} more"
            self.fail(
                f"{len(violations)} C++ indentation violation(s); "
                f"expected 4-space indent and no tabs:\n{preview}{more}"
            )


class TestCiWiresCppIndentationScan(unittest.TestCase):
    def test_linux_ci_runs_workspace_indentation_check(self) -> None:
        """Avoid reintroducing per-package filegroups: CI scans the checkout."""
        workflow = _repo_root() / ".github" / "workflows" / "ci_linux.yml"
        self.assertTrue(workflow.is_file(), f"missing {workflow}")
        text = workflow.read_text(encoding="utf-8")
        self.assertIn(
            _WORKSPACE_INDENT_CHECK,
            text,
            "ci_linux.yml must run the workspace C++ indentation check "
            f"(`{_WORKSPACE_INDENT_CHECK}`) so the full tree is scanned "
            "without Bazel filegroups of every source",
        )


class TestRepoRoot(unittest.TestCase):
    def test_does_not_follow_symlink_out_of_runfiles_tree(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            source = tmp_path / "source" / "python" / "tests"
            source.mkdir(parents=True)
            (source / "cpp_indentation_test.py").write_text("# source\n", encoding="utf-8")
            (tmp_path / "source" / ".bazelrc").write_text("#\n", encoding="utf-8")
            (tmp_path / "source" / "CPPVARIABLES.bzl").write_text("#\n", encoding="utf-8")

            runfiles = tmp_path / "runfiles" / "_main"
            tests_dir = runfiles / "python" / "tests"
            tests_dir.mkdir(parents=True)
            (tests_dir / "cpp_indentation_test.py").symlink_to(
                source / "cpp_indentation_test.py"
            )
            (runfiles / ".bazelrc").write_text("#\n", encoding="utf-8")
            (runfiles / "CPPVARIABLES.bzl").write_text("#\n", encoding="utf-8")

            found = _repo_root(tests_dir / "cpp_indentation_test.py")
            self.assertEqual(found, runfiles)


def _fix_repo(root: Path) -> int:
    changed = 0
    for path in iter_cpp_files(root):
        original = path.read_text(encoding="utf-8", errors="replace")
        fixed = reindent_text(original)
        if fixed != original:
            path.write_text(fixed, encoding="utf-8")
            changed += 1
    print(f"reindented {changed} file(s)")
    return changed


if __name__ == "__main__":
    if "--fix" in sys.argv:
        _fix_repo(_repo_root())
        sys.exit(0)
    unittest.main()
