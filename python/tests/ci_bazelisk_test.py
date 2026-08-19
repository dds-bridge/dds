#!/usr/bin/env python3
"""Guard that CI invokes bazelisk, not bare bazel, for Bazel CLI commands."""

from __future__ import annotations

import re
import tempfile
import unittest
from pathlib import Path


# Matches a bare `bazel` launcher token followed by a Bazel subcommand.
# Lookbehind excludes word chars / `.` / `-` so `bazelisk`, `bazel-bin`, and
# `bazel-contrib` do not match. Absolute or drive-qualified paths such as
# `/usr/bin/bazel build` still match — CI should use bazelisk, not bare bazel.
_BARE_BAZEL_CMD = re.compile(
    r"(?<![\w.-])bazel\s+(build|test|fetch|run|clean|shutdown|coverage|info|mod|query)\b"
)


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() — under `bazel test` this file is often a runfiles
    # symlink into the execroot/source tree, and resolving leaves the runfiles
    # tree where //.github/workflows data deps live.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        workflows = parent / ".github" / "workflows"
        if workflows.is_dir() and any(workflows.glob("ci_*.yml")):
            return parent
    raise AssertionError("could not locate repository root from test file path")


class TestBareBazelCmd(unittest.TestCase):
    def test_matches_bare_bazel_subcommand(self) -> None:
        self.assertIsNotNone(_BARE_BAZEL_CMD.search("bazel build //..."))
        self.assertIsNotNone(_BARE_BAZEL_CMD.search("  bazel test --verbose_failures //..."))

    def test_matches_path_qualified_bare_bazel(self) -> None:
        """CI must not sneak past the guard via an absolute bazel path."""
        self.assertIsNotNone(_BARE_BAZEL_CMD.search("/usr/bin/bazel build //..."))
        self.assertIsNotNone(_BARE_BAZEL_CMD.search(r"C:\tools\bazel test //..."))

    def test_does_not_match_bazelisk_or_artifact_names(self) -> None:
        self.assertIsNone(_BARE_BAZEL_CMD.search("bazelisk build //..."))
        self.assertIsNone(_BARE_BAZEL_CMD.search("bazelisk-version: 1.0"))
        self.assertIsNone(_BARE_BAZEL_CMD.search("path: bazel-bin/library/tests/dtest"))
        self.assertIsNone(_BARE_BAZEL_CMD.search("https://github.com/bazel-contrib/rules_python"))


class TestRepoRoot(unittest.TestCase):
    def test_does_not_follow_symlink_out_of_runfiles_tree(self) -> None:
        """Runfiles often symlink sources; resolve() would miss sibling data deps."""
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            source = tmp_path / "source" / "python" / "tests"
            source.mkdir(parents=True)
            (source / "ci_bazelisk_test.py").write_text("# source copy\n", encoding="utf-8")

            runfiles = tmp_path / "runfiles" / "_main"
            tests_dir = runfiles / "python" / "tests"
            tests_dir.mkdir(parents=True)
            workflows = runfiles / ".github" / "workflows"
            workflows.mkdir(parents=True)
            (workflows / "ci_linux.yml").write_text("name: ci\n", encoding="utf-8")

            link = tests_dir / "ci_bazelisk_test.py"
            link.symlink_to(source / "ci_bazelisk_test.py")

            root = _repo_root(start=link)
            self.assertEqual(root, runfiles)
            self.assertTrue((root / ".github" / "workflows" / "ci_linux.yml").is_file())


class TestCiUsesBazelisk(unittest.TestCase):
    def test_workflow_run_steps_use_bazelisk_launcher(self) -> None:
        workflows = sorted((_repo_root() / ".github" / "workflows").glob("ci_*.yml"))
        self.assertTrue(workflows, "expected CI workflow YAML files")
        offenders: list[str] = []
        for path in workflows:
            text = path.read_text(encoding="utf-8")
            for i, line in enumerate(text.splitlines(), start=1):
                if line.lstrip().startswith("#"):
                    continue
                if _BARE_BAZEL_CMD.search(line):
                    offenders.append(f"{path.name}:{i}: {line.strip()}")
        self.assertEqual(
            offenders,
            [],
            "CI should invoke bazelisk (not bare bazel) for Bazel commands:\n"
            + "\n".join(offenders),
        )


if __name__ == "__main__":
    unittest.main()
