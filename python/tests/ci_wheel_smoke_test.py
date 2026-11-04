#!/usr/bin/env python3
"""Guard that CI builds the Python wheel and smoke-tests a real `pip install`.

//python:dds3_wheel_dist is tagged manual (its platform select has no arm
for Linux ARM64/Intel macOS, see python/BUILD.bazel), so `bazelisk build
//...` does not produce it and each CI workflow must build it explicitly by
name. `bazelisk test //...` already exercises _dds3 through Bazel's own
py_library layout, but neither that nor an explicit wheel build ever runs
`pip install` on the packaged .whl, so a mistag (see ci_wheel_tags_test.py)
or a missing/renamed file inside the archive would go unnoticed. This guards
that each of the three CI workflows pins a CPython 3.14 interpreter (to
match the wheel's cp314 tag) and actually installs + imports the built
wheel in a throwaway venv.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path

_STEP_NAME = "Build and smoke-test Python wheel"


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() -- under `bazel test` this file is often a
    # runfiles symlink into the execroot/source tree, and resolving leaves
    # the runfiles tree where //.github/workflows data deps live.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        workflows = parent / ".github" / "workflows"
        if workflows.is_dir() and any(workflows.glob("ci_*.yml")):
            return parent
    raise AssertionError("could not locate repository root from test file path")


def _workflow_text(name: str) -> str:
    return (_repo_root() / ".github" / "workflows" / name).read_text(encoding="utf-8")


def _step_body(workflow_text: str, step_name: str) -> str | None:
    """Return the body of a `- name: <step_name>` step, or None if absent."""
    lines = workflow_text.splitlines()
    name_re = re.compile(rf"^(\s*)-\s*name:\s*{re.escape(step_name)}\s*$")
    start: int | None = None
    step_indent: int | None = None
    for index, line in enumerate(lines):
        match = name_re.match(line)
        if match:
            start, step_indent = index, len(match.group(1))
            break
    if start is None or step_indent is None:
        return None
    collected = [lines[start]]
    for line in lines[start + 1 :]:
        if not line.strip():
            collected.append(line)
            continue
        indent = len(line) - len(line.lstrip(" "))
        if indent <= step_indent and line[indent:].startswith("-"):
            break
        if indent < step_indent:
            break
        collected.append(line)
    return "\n".join(collected)


class TestWheelSmokeTestStepHelpers(unittest.TestCase):
    """Boundary cases for _step_body itself, not the real workflow files."""

    def test_returns_none_when_step_is_absent(self) -> None:
        self.assertIsNone(_step_body("steps:\n  - name: Unrelated\n    run: echo hi\n", _STEP_NAME))

    def test_isolates_body_between_sibling_steps(self) -> None:
        sample = (
            "steps:\n"
            "  - name: Before\n"
            "    run: echo before\n"
            f"  - name: {_STEP_NAME}\n"
            "    run: |\n"
            "      echo inside\n"
            "  - name: After\n"
            "    run: echo after\n"
        )
        body = _step_body(sample, _STEP_NAME)
        self.assertIsNotNone(body)
        assert body is not None
        self.assertIn("echo inside", body)
        self.assertNotIn("echo before", body)
        self.assertNotIn("echo after", body)


class TestNextNonblankLine(unittest.TestCase):
    """Boundary cases for _next_nonblank_line itself."""

    def test_returns_first_nonblank_line_after_marker(self) -> None:
        body = "bazelisk build //...\n\nif ($LASTEXITCODE -ne 0) { exit 1 }\n"
        line = _next_nonblank_line(body, re.compile(r"bazelisk\s+build"))
        self.assertEqual(line, "if ($LASTEXITCODE -ne 0) { exit 1 }")

    def test_returns_none_when_marker_is_absent(self) -> None:
        self.assertIsNone(
            _next_nonblank_line("echo hi\n", re.compile(r"bazelisk\s+build"))
        )

    def test_returns_none_when_marker_is_the_last_line(self) -> None:
        self.assertIsNone(
            _next_nonblank_line("bazelisk build //...\n", re.compile(r"bazelisk\s+build"))
        )


def _next_nonblank_line(body: str, marker: re.Pattern[str]) -> str | None:
    """The first non-blank line after the first line matching `marker`."""
    lines = body.splitlines()
    for index, line in enumerate(lines):
        if marker.search(line):
            for later in lines[index + 1 :]:
                if later.strip():
                    return later
            return None
    return None


_EXIT_CODE_CHECK = re.compile(r"\$LASTEXITCODE\s*-ne\s*0")
_WHEEL_NULL_CHECK = re.compile(r"if\s*\(\s*-not\s*\$wheel\s*\)")


def _assert_wheel_smoke_step(
    case: unittest.TestCase, workflow_text: str, *, is_windows: bool
) -> None:
    case.assertRegex(
        workflow_text,
        r'actions/setup-python@[0-9a-f]{40}[^\n]*\n\s+with:\s*\n\s+python-version:\s*"3\.14"',
        "expected a pinned actions/setup-python step requesting 3.14, to match "
        "dds3_wheel's cp314 tag",
    )

    body = _step_body(workflow_text, _STEP_NAME)
    case.assertIsNotNone(body, f'expected a step named "{_STEP_NAME}"')
    assert body is not None

    case.assertRegex(
        body,
        r"bazelisk\s+build\b[^\n]*//python:dds3_wheel_dist\b",
        "expected the step to build //python:dds3_wheel_dist",
    )
    if is_windows:
        case.assertRegex(
            body,
            r"bazelisk\s+build\b[^\n]*--config=opt\b[^\n]*//python:dds3_wheel_dist\b"
            r"|bazelisk\s+build\b[^\n]*//python:dds3_wheel_dist\b[^\n]*--config=opt\b",
            "Windows CI must use --config=opt on every bazelisk build invocation",
        )
        # pwsh does not abort a step on a failing native command the way the
        # default bash shell does (see the "Bazel build and test" step's own
        # $LASTEXITCODE checks two steps above) -- without an explicit check
        # after each command, a failure here can be silently swallowed.
        for label, marker in (
            ("bazelisk build", re.compile(r"bazelisk\s+build\b")),
            ("venv creation", re.compile(r"-m\s+venv\b")),
            ("pip install", re.compile(r"pip(\.exe)?[^\n]*install")),
        ):
            next_line = _next_nonblank_line(body, marker)
            case.assertIsNotNone(next_line, f"expected a line after {label}")
            assert next_line is not None
            case.assertRegex(
                next_line,
                _EXIT_CODE_CHECK,
                f"expected a $LASTEXITCODE check immediately after {label}",
            )

        # Get-ChildItem | Select-Object -First 1 silently yields $null if no
        # .whl matched, and $null.FullName then silently evaluates to $null
        # rather than failing -- unlike the external-command checks above,
        # this needs a $wheel null-check, not a $LASTEXITCODE check.
        wheel_discovery_line = _next_nonblank_line(
            body, re.compile(r"Get-ChildItem\b")
        )
        case.assertIsNotNone(
            wheel_discovery_line, "expected a line after the wheel Get-ChildItem"
        )
        assert wheel_discovery_line is not None
        case.assertRegex(
            wheel_discovery_line,
            _WHEEL_NULL_CHECK,
            "expected a $wheel null-check immediately after Get-ChildItem, "
            "before $wheel.FullName is used",
        )

    case.assertRegex(
        body,
        r"\.whl\b",
        "expected the step to reference the packaged .whl file",
    )
    case.assertRegex(
        body,
        r"pip[^\n]*install[^\n]*--no-index",
        "expected an offline `pip install --no-index` of the packaged wheel",
    )
    case.assertRegex(
        body,
        r"import dds3",
        "expected the smoke test to actually import the installed dds3 package",
    )
    case.assertRegex(
        body,
        r"dds3\.api_root\(\)",
        "expected the smoke test to call into the compiled extension, not just "
        "import the pure-Python package shim",
    )


class TestLinuxCiSmokeTestsWheel(unittest.TestCase):
    def test_build_and_test_job_installs_and_imports_the_wheel(self) -> None:
        _assert_wheel_smoke_step(
            self, _workflow_text("ci_linux.yml"), is_windows=False
        )


class TestMacosCiSmokeTestsWheel(unittest.TestCase):
    def test_build_and_test_job_installs_and_imports_the_wheel(self) -> None:
        _assert_wheel_smoke_step(
            self, _workflow_text("ci_macos.yml"), is_windows=False
        )


class TestWindowsCiSmokeTestsWheel(unittest.TestCase):
    def test_build_and_test_job_installs_and_imports_the_wheel(self) -> None:
        _assert_wheel_smoke_step(
            self, _workflow_text("ci_windows.yml"), is_windows=True
        )


if __name__ == "__main__":
    unittest.main()
