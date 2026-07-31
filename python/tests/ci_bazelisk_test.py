#!/usr/bin/env python3
"""Guard that CI invokes bazelisk, not bare bazel, for Bazel CLI commands."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


# Matches a bare `bazel` launcher token followed by a Bazel subcommand.
# Intentionally does not match paths (bazel-bin), artifact names, or
# bazelisk / bazelisk-version / bazel-contrib.
_BARE_BAZEL_CMD = re.compile(
    r"(?<![\w.-])bazel\s+(build|test|fetch|run|clean|shutdown|coverage|info|mod|query)\b"
)


def _repo_root() -> Path:
    here = Path(__file__).resolve()
    for parent in here.parents:
        workflows = parent / ".github" / "workflows"
        if workflows.is_dir() and any(workflows.glob("ci_*.yml")):
            return parent
    raise AssertionError("could not locate repository root from test file path")


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
