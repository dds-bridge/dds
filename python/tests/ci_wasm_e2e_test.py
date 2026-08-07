#!/usr/bin/env python3
"""Guard that WASM CI overrides .bazelrc's -e2e filter so Playwright runs."""

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


class TestBazelrcSkipsE2eByDefault(unittest.TestCase):
    def test_bazelrc_excludes_e2e_tag(self) -> None:
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        self.assertRegex(
            bazelrc,
            r"(?m)^test\s+--test_tag_filters=-e2e\b",
            "expected default test config to skip e2e (Playwright) targets",
        )


class TestWasmCiRunsE2e(unittest.TestCase):
    def test_ci_wasm_clears_e2e_tag_filter(self) -> None:
        """web_system_tests includes dds_mvp_e2e_test; CI must not inherit -e2e."""
        text = (_repo_root() / ".github" / "workflows" / "ci_wasm.yml").read_text(
            encoding="utf-8"
        )
        # Empty --test_tag_filters= overrides .bazelrc's -e2e so tagged tests run.
        self.assertRegex(
            text,
            r"bazelisk\s+test\b[^\n]*--test_tag_filters=(?:\s|$)",
            "ci_wasm.yml must pass empty --test_tag_filters= to run e2e targets",
        )
        self.assertRegex(
            text,
            r"bazelisk\s+test\b[^\n]*//web:web_tests\b",
            "expected WASM CI to keep running //web:web_tests",
        )
        self.assertRegex(
            text,
            r"bazelisk\s+test\b[^\n]*//web:web_system_tests\b",
            "expected WASM CI to keep running //web:web_system_tests (includes e2e)",
        )
        # Ensure we did not reintroduce an -e2e exclusion on the same invocation.
        for match in re.finditer(r"bazelisk\s+test\b[^\n]+", text):
            line = match.group(0)
            if "--test_tag_filters=" in line:
                self.assertNotRegex(
                    line,
                    r"--test_tag_filters=[^\n]*-e2e\b",
                    "WASM CI test invocation must not exclude the e2e tag",
                )


if __name__ == "__main__":
    unittest.main()
