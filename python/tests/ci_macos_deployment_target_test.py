#!/usr/bin/env python3
"""Guard that the macOS deployment target backs the dds3 wheel's platform tag.

python/BUILD.bazel tags dds3_wheel "macosx_11_0_arm64", but until this guard
existed nothing actually constrained the compiled extension's minimum macOS
version -- the real floor baked into the .so was whatever the runner's
default Xcode/SDK toolchain picked, which pip's tag would then misrepresent
if it ever drifted from 11.0. CPPVARIABLES.bzl must set
-mmacosx-version-min=11.0 for both compile and link on every macOS
config_setting, and that floor must match the wheel tag's own major.minor.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() -- under `bazel test` this file is often a
    # runfiles symlink into the execroot/source tree, and resolving leaves
    # the runfiles tree where //CPPVARIABLES.bzl and //python:BUILD.bazel
    # live.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        if (parent / "CPPVARIABLES.bzl").is_file() and (
            parent / "python" / "BUILD.bazel"
        ).is_file():
            return parent
    raise AssertionError("could not locate repository root from test file path")


def _select_arm_block(text: str, variable: str, label: str) -> str:
    """The string-list body of the `"//:<label>": [...]` arm inside the
    first `select({...})` found after `<variable> = `.
    """
    var_match = re.search(rf"^{re.escape(variable)}\s*=", text, flags=re.MULTILINE)
    if var_match is None:
        raise AssertionError(f"expected a top-level `{variable} = ` assignment")
    arm_match = re.search(
        rf'"//:{re.escape(label)}":\s*\[(.*?)\]',
        text[var_match.start() :],
        flags=re.DOTALL,
    )
    if arm_match is None:
        raise AssertionError(f'expected a "//:{label}" arm in {variable}')
    return arm_match.group(1)


_DEPLOYMENT_TARGET = re.compile(r'"-mmacosx-version-min=(\d+)\.(\d+)"')


class TestMacosDeploymentTargetMatchesWheelTag(unittest.TestCase):
    def test_cppopts_set_deployment_target_on_release_and_debug_macos(self) -> None:
        text = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        for label in ("build_macos", "debug_build_macos"):
            block = _select_arm_block(text, "DDS_CPPOPTS", label)
            self.assertRegex(
                block,
                _DEPLOYMENT_TARGET,
                f"expected {label}'s DDS_CPPOPTS arm to set -mmacosx-version-min",
            )

    def test_linkopts_set_deployment_target_on_release_and_debug_macos(self) -> None:
        text = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        for label in ("build_macos", "debug_build_macos"):
            block = _select_arm_block(text, "DDS_LINKOPTS", label)
            self.assertRegex(
                block,
                _DEPLOYMENT_TARGET,
                f"expected {label}'s DDS_LINKOPTS arm to set -mmacosx-version-min",
            )

    def test_deployment_target_matches_wheel_platform_tag_floor(self) -> None:
        cppvariables = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        cppopts_block = _select_arm_block(cppvariables, "DDS_CPPOPTS", "build_macos")
        flag_match = _DEPLOYMENT_TARGET.search(cppopts_block)
        self.assertIsNotNone(
            flag_match, "expected build_macos DDS_CPPOPTS to set -mmacosx-version-min"
        )
        assert flag_match is not None
        flag_floor = f"{flag_match.group(1)}_{flag_match.group(2)}"

        build_bazel = (_repo_root() / "python" / "BUILD.bazel").read_text(
            encoding="utf-8"
        )
        tag_match = re.search(r'"macosx_(\d+)_(\d+)_arm64"', build_bazel)
        self.assertIsNotNone(
            tag_match, 'expected a "macosx_<major>_<minor>_arm64" wheel platform tag'
        )
        assert tag_match is not None
        tag_floor = f"{tag_match.group(1)}_{tag_match.group(2)}"

        self.assertEqual(
            flag_floor,
            tag_floor,
            "the enforced -mmacosx-version-min floor must match the wheel's "
            "advertised macosx_<floor>_arm64 tag, or the tag would overclaim "
            "(or underclaim) real compatibility",
        )


class TestAsanMacosAvoidsConflictingDeploymentTargetFlag(unittest.TestCase):
    """--config=asan on macOS (build:asan_macos in .bazelrc) swaps in
    local_config_apple_cc_toolchains, whose generated compile command
    already carries its own `-target arm64-apple-macosX.Y`. Also setting
    -mmacosx-version-min=11.0 there makes clang error under -Werror:
    "overriding '-mmacosx-version-min=11.0' option with '-target
    arm64-apple-macosX.Y' [-Werror,-Woverriding-option]". A more specific
    config_setting (debug_build_macos_asan, matched via a --define only
    build:asan_macos sets) must win that select() arm over debug_build_macos
    and must not carry the flag.
    """

    def test_bazelrc_sets_the_asan_macos_define(self) -> None:
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        self.assertRegex(
            bazelrc,
            r"build:asan_macos\s+--define=macos_asan=true",
            "expected build:asan_macos to set --define=macos_asan=true so "
            "debug_build_macos_asan can match only under asan",
        )

    def test_build_bazel_defines_asan_specific_config_setting(self) -> None:
        build_bazel = (_repo_root() / "BUILD.bazel").read_text(encoding="utf-8")
        self.assertIn(
            'name = "debug_build_macos_asan"',
            build_bazel,
            "expected a debug_build_macos_asan config_setting, more "
            "specific than debug_build_macos, to win under --config=asan",
        )

    def test_cppopts_asan_arm_has_no_deployment_target_flag(self) -> None:
        text = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        block = _select_arm_block(text, "DDS_CPPOPTS", "debug_build_macos_asan")
        self.assertNotRegex(
            block,
            _DEPLOYMENT_TARGET,
            "debug_build_macos_asan must not set -mmacosx-version-min; it "
            "conflicts with local_config_apple_cc_toolchains' own -target",
        )

    def test_linkopts_asan_arm_has_no_deployment_target_flag(self) -> None:
        text = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        block = _select_arm_block(text, "DDS_LINKOPTS", "debug_build_macos_asan")
        self.assertNotRegex(
            block,
            _DEPLOYMENT_TARGET,
            "debug_build_macos_asan must not set -mmacosx-version-min; it "
            "conflicts with local_config_apple_cc_toolchains' own -target",
        )


class TestSelectArmBlockHelperTolerance(unittest.TestCase):
    """Boundary cases for _select_arm_block itself, not the real files."""

    def test_raises_when_variable_is_missing(self) -> None:
        with self.assertRaises(AssertionError):
            _select_arm_block('OTHER = select({"//:build_macos": []})', "DDS_CPPOPTS", "build_macos")

    def test_raises_when_arm_is_missing(self) -> None:
        with self.assertRaises(AssertionError):
            _select_arm_block(
                'DDS_CPPOPTS = select({"//:build_linux": []})', "DDS_CPPOPTS", "build_macos"
            )

    def test_finds_arm_after_the_named_variable_only(self) -> None:
        sample = (
            'DDS_LINKOPTS = select({"//:build_macos": ["-flto=thin"]})\n'
            'DDS_CPPOPTS = select({"//:build_macos": ["-O3"]})\n'
        )
        block = _select_arm_block(sample, "DDS_CPPOPTS", "build_macos")
        self.assertIn("-O3", block)
        self.assertNotIn("-flto=thin", block)


if __name__ == "__main__":
    unittest.main()
