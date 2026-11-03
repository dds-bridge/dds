#!/usr/bin/env python3
"""Guard that dds3_wheel carries correct CPython/platform tags.

_dds3 is a pybind11 extension built against the full CPython API (no
Py_LIMITED_API/abi3), for the CPython version MODULE.bazel pins
(python_version = "3.14"). py_wheel defaults to python_tag="py3",
abi="none", platform="any" -- a universal pure-Python tag -- which would
silently mislabel the compiled extension as installable anywhere.
"""

from __future__ import annotations

import ast
import unittest
from pathlib import Path


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() -- under `bazel test` this file is often a
    # runfiles symlink into the execroot/source tree, and resolving leaves
    # the runfiles tree where //python:BUILD.bazel lives.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        if (parent / "python" / "BUILD.bazel").is_file():
            return parent
    raise AssertionError("could not locate repository root from test file path")


_UNRESOLVED = object()


def _literal(node: ast.expr) -> object:
    """The node's value if it is a string literal, else _UNRESOLVED."""
    if isinstance(node, ast.Constant) and isinstance(node.value, str):
        return node.value
    return _UNRESOLVED


def _py_wheel_call(build_bazel: str, name: str) -> ast.Call:
    """The py_wheel(name = "<name>", ...) call node, by its literal name."""
    for node in ast.walk(ast.parse(build_bazel)):
        if not (
            isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id == "py_wheel"
        ):
            continue
        for kw in node.keywords:
            if kw.arg == "name" and _literal(kw.value) == name:
                return node
    raise AssertionError(f'expected py_wheel(name = "{name}", ...)')


def _keyword_value(call: ast.Call, keyword: str) -> ast.expr | None:
    for kw in call.keywords:
        if kw.arg == keyword:
            return kw.value
    return None


def _select_arms(node: ast.expr) -> dict[str, object]:
    """The {condition_label: literal_value} arms of a select({...}) call.

    Fails closed (AssertionError) if node is not a select() call over a
    dict literal, since any other shape can't be read back from the source.
    """
    if not (
        isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id == "select"
        and node.args
        and isinstance(node.args[0], ast.Dict)
    ):
        raise AssertionError("expected select({...}) over a dict literal")
    arms: dict[str, object] = {}
    for key, value in zip(node.args[0].keys, node.args[0].values):
        if key is None:
            raise AssertionError("select() arm key must not be **-unpacked")
        label = _literal(key)
        if label is _UNRESOLVED:
            raise AssertionError("select() arm key must be a string literal")
        arms[label] = _literal(value)
    return arms


class TestDds3WheelCpythonTags(unittest.TestCase):
    def test_sets_cpython_314_python_tag_and_abi(self) -> None:
        """A py3-none wheel would claim to be a universal pure-Python build."""
        text = (_repo_root() / "python" / "BUILD.bazel").read_text(encoding="utf-8")
        call = _py_wheel_call(text, "dds3_wheel")

        python_tag = _keyword_value(call, "python_tag")
        abi = _keyword_value(call, "abi")
        self.assertIsNotNone(python_tag, "expected dds3_wheel to set python_tag")
        self.assertIsNotNone(abi, "expected dds3_wheel to set abi")
        assert python_tag is not None and abi is not None

        self.assertEqual(
            _literal(python_tag),
            "cp314",
            "dds3_wheel's _dds3 extension targets CPython 3.14 specifically",
        )
        self.assertEqual(
            _literal(abi),
            "cp314",
            "_dds3 links against the full CPython API, not the stable abi3 ABI",
        )

    def test_platform_select_covers_all_three_os_with_platform_specific_tags(
        self,
    ) -> None:
        """A bare platform="any" would mislabel the compiled extension.

        //:build_linux matches any Linux CPU (see specs/build-system.md), but
        there is no cpu-generic Linux wheel tag, so the select must key on
        the more specific //:build_linux_x86_64 -- keying on //:build_linux
        would silently mistag an aarch64 Linux build as x86_64 (pip would
        then reject the wheel on its real architecture and accept it on an
        incompatible one). "linux_x86_64" itself is the honest (non-portable)
        tag for how CI actually builds this wheel: ubuntu-latest + this
        repo's own hermetic LLVM, with no manylinux sysroot/auditwheel step,
        so claiming a manylinux2014 floor would be a portability lie.

        //:build_macos matches any macOS CPU (see specs/build-system.md), but
        there is no cpu-generic macOS wheel tag, so the select must key on
        the more specific //:build_macos_arm64 -- keying on //:build_macos
        would silently mistag an x86_64 mac build as arm64.
        """
        text = (_repo_root() / "python" / "BUILD.bazel").read_text(encoding="utf-8")
        call = _py_wheel_call(text, "dds3_wheel")

        platform = _keyword_value(call, "platform")
        self.assertIsNotNone(platform, "expected dds3_wheel to set platform")
        assert platform is not None

        arms = _select_arms(platform)
        self.assertEqual(
            arms,
            {
                "//:build_linux_x86_64": "linux_x86_64",
                "//:build_macos_arm64": "macosx_11_0_arm64",
                "//:build_windows": "win_amd64",
            },
            "expected one platform tag per supported CI host OS",
        )


class TestPyWheelCallHelperTolerance(unittest.TestCase):
    """Boundary cases for the AST helpers themselves, not the real file."""

    def test_py_wheel_call_tolerates_reordered_keywords(self) -> None:
        sample = (
            'py_wheel(\n'
            '    python_tag = "cp314",\n'
            '    name = "dds3_wheel",\n'
            '    abi = "cp314",\n'
            ')\n'
        )
        call = _py_wheel_call(sample, "dds3_wheel")
        self.assertEqual(_literal(_keyword_value(call, "python_tag")), "cp314")

    def test_py_wheel_call_raises_when_name_is_missing(self) -> None:
        with self.assertRaises(AssertionError):
            _py_wheel_call('py_wheel(python_tag = "cp314")', "dds3_wheel")

    def test_py_wheel_call_ignores_other_named_wheels(self) -> None:
        sample = (
            'py_wheel(name = "other_wheel", python_tag = "py3")\n'
            'py_wheel(name = "dds3_wheel", python_tag = "cp314")\n'
        )
        call = _py_wheel_call(sample, "dds3_wheel")
        self.assertEqual(_literal(_keyword_value(call, "python_tag")), "cp314")

    def test_select_arms_reads_dict_literal(self) -> None:
        node = ast.parse(
            'select({"//:build_linux": "manylinux2014_x86_64"})'
        ).body[0].value
        self.assertEqual(
            _select_arms(node),
            {"//:build_linux": "manylinux2014_x86_64"},
        )

    def test_select_arms_rejects_non_select_call(self) -> None:
        node = ast.parse('"win_amd64"').body[0].value
        with self.assertRaises(AssertionError):
            _select_arms(node)


if __name__ == "__main__":
    unittest.main()
