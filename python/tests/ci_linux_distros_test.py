#!/usr/bin/env python3
"""Guard that the Linux distro container CI stays wired to the host-dependent build."""

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


def _workflow() -> str:
    return (_repo_root() / ".github" / "workflows" / "ci_linux_distros.yml").read_text(
        encoding="utf-8"
    )


def _install_script() -> str:
    return (_repo_root() / ".github" / "scripts" / "install_distro_deps.sh").read_text(
        encoding="utf-8"
    )


class TestDistroMatrix(unittest.TestCase):
    def test_matrix_covers_newest_glibc_and_oldest_supported_rhel(self) -> None:
        text = _workflow()
        self.assertRegex(text, r'(?m)^\s+image:\s*\[[^\]]*"fedora:latest"')
        self.assertRegex(text, r'(?m)^\s+image:\s*\[[^\]]*"almalinux:10"')
        self.assertRegex(text, r"(?m)^\s+container:\s*\$\{\{\s*matrix\.image\s*\}\}")

    def test_one_distro_failure_does_not_cancel_the_others(self) -> None:
        self.assertRegex(_workflow(), r"(?m)^\s+fail-fast:\s*false\s*$")

    def test_tests_the_targets_compiled_against_host_headers(self) -> None:
        self.assertRegex(
            _workflow(),
            r"bazelisk\s+test\b[^\n]*//library/\.\.\.[^\n]*//python/\.\.\.",
        )


class TestDistroCaching(unittest.TestCase):
    def test_disk_cache_is_disabled(self) -> None:
        """Host glibc headers are undeclared inputs; a cache hit would mask drift."""
        self.assertRegex(
            _workflow(),
            r'uses:\s*\./\.github/actions/setup-bazelisk\s*\n\s+with:\s*\n\s+disk-cache:\s*"false"',
        )

    def test_setup_bazelisk_honours_disk_cache_false(self) -> None:
        action = (
            _repo_root() / ".github" / "actions" / "setup-bazelisk" / "action.yml"
        ).read_text(encoding="utf-8")
        self.assertRegex(action, r"(?m)^  disk-cache:\s*$")
        self.assertEqual(
            len(re.findall(r"disk-cache:\s*\$\{\{\s*inputs\.disk-cache\s*==\s*'false'", action)),
            2,
            "both setup-bazel attempts must respect the disk-cache input",
        )


class TestDistroPackages(unittest.TestCase):
    def test_git_is_installed_before_checkout(self) -> None:
        text = _workflow()
        install_git = text.find("name: Install git")
        checkout = text.find("uses: actions/checkout@")
        self.assertNotEqual(install_git, -1)
        self.assertNotEqual(checkout, -1)
        self.assertLess(install_git, checkout)

    def test_script_installs_gnu_ld_because_linux_links_use_it(self) -> None:
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        self.assertIn('"linux-x86_64": ["--ld-path=/usr/bin/ld"]', module)
        self.assertRegex(_install_script(), r"(?m)^common=\([^)]*\bbinutils\b")

    def test_script_installs_glibc_headers_for_each_family(self) -> None:
        script = _install_script()
        for family, headers in (
            ("dnf", "glibc-devel"),
            ("apt-get", "libc6-dev"),
            ("zypper", "glibc-devel"),
        ):
            with self.subTest(family=family):
                self.assertRegex(script, rf"(?m)^\s+{re.escape(family)}\b[^\n]*\b{headers}\b")


if __name__ == "__main__":
    unittest.main()
