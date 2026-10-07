#!/usr/bin/env python3
"""Guard that the Linux distro container CI stays wired to the host-dependent build."""

from __future__ import annotations

import json
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


def _rolling_workflow() -> str:
    return (
        _repo_root() / ".github" / "workflows" / "ci_linux_distros_rolling.yml"
    ).read_text(encoding="utf-8")


def _install_script() -> str:
    return (_repo_root() / ".github" / "scripts" / "install_distro_deps.sh").read_text(
        encoding="utf-8"
    )


class TestDistroMatrix(unittest.TestCase):
    def test_matrix_covers_newest_glibc_and_oldest_supported_rhel(self) -> None:
        """The PR matrix is the fallback when no images input is given."""
        text = _workflow()
        default = re.search(
            r"(?m)^\s+image:\s*\$\{\{\s*fromJSON\(inputs\.images\s*\|\|\s*'(\[[^']*\])'\)\s*\}\}",
            text,
        )
        self.assertIsNotNone(default, "matrix must default to the PR images")
        assert default is not None
        self.assertEqual(json.loads(default.group(1)), ["fedora:latest", "almalinux:10"])
        self.assertRegex(text, r"(?m)^\s+container:\s*\$\{\{\s*matrix\.image\s*\}\}")

    def test_one_distro_failure_does_not_cancel_the_others(self) -> None:
        self.assertRegex(_workflow(), r"(?m)^\s+fail-fast:\s*false\s*$")

    def test_tests_the_targets_compiled_against_host_headers(self) -> None:
        self.assertRegex(
            _workflow(),
            r"bazelisk\s+test\b[^\n]*//library/\.\.\.[^\n]*//python/\.\.\.",
        )


class TestRollingDistros(unittest.TestCase):
    """Rolling distros run weekly through the same job, never on pull requests."""

    def test_runs_weekly_and_on_demand_only(self) -> None:
        text = _rolling_workflow()
        self.assertRegex(text, r'(?m)^\s+- cron:\s*"0 6 \* \* 1"')
        self.assertRegex(text, r"(?m)^  workflow_dispatch:")
        self.assertNotRegex(text, r"(?m)^  (pull_request\w*|push):")
        self.assertNotRegex(text, r"(?m)^on:[ \t]*[^\s#]")

    def test_reuses_the_distro_job(self) -> None:
        self.assertRegex(
            _rolling_workflow(),
            r"(?m)^\s+uses:\s*\./\.github/workflows/ci_linux_distros\.yml\s*$",
        )
        self.assertRegex(_workflow(), r"(?m)^  workflow_call:\s*\n\s+inputs:\s*\n\s+images:")

    def test_rolling_images_cover_each_package_manager(self) -> None:
        """Tier 1 only runs the dnf branch; the weekly run exercises the others."""
        package_managers = {
            "fedora:rawhide": "dnf",
            "archlinux:latest": "pacman",
            "opensuse/tumbleweed": "zypper",
            "debian:testing": "apt-get",
        }
        images = re.search(r"(?m)^\s+images:\s*'(\[[^']*\])'\s*$", _rolling_workflow())
        self.assertIsNotNone(images)
        assert images is not None
        self.assertEqual(json.loads(images.group(1)), list(package_managers))
        script = _install_script()
        for image, manager in package_managers.items():
            with self.subTest(image=image):
                self.assertRegex(script, rf"(?m)^\s+{re.escape(manager)}\s")


class TestDistroCaching(unittest.TestCase):
    def test_disk_cache_is_disabled(self) -> None:
        """Host glibc headers are undeclared inputs; a cache hit would mask drift."""
        self.assertRegex(
            _workflow(),
            r'uses:\s*\./\.github/actions/setup-bazelisk\s*\n\s+with:\s*\n\s+disk-cache:\s*"false"',
        )

    def test_setup_bazelisk_honors_disk_cache_false(self) -> None:
        action = (
            _repo_root() / ".github" / "actions" / "setup-bazelisk" / "action.yml"
        ).read_text(encoding="utf-8")
        self.assertRegex(action, r"(?m)^  disk-cache:\s*$")
        self.assertEqual(
            len(
                re.findall(
                    r"disk-cache:\s*\$\{\{\s*inputs\.disk-cache\s*==\s*'false'\s*&&\s*'false'\s*\|\|",
                    action,
                )
            ),
            2,
            "both setup-bazel attempts must respect the disk-cache input",
        )


    def test_setup_bazelisk_provides_bazelisk_name_in_bare_containers(self) -> None:
        """setup-bazel installs Bazelisk as `bazel`; only runner images ship `bazelisk`."""
        action = (
            _repo_root() / ".github" / "actions" / "setup-bazelisk" / "action.yml"
        ).read_text(encoding="utf-8")
        self.assertRegex(
            action,
            r'if ! command -v bazelisk\b[\s\S]*?ln -s "\$bazel_path" "\$\(dirname "\$bazel_path"\)/bazelisk"',
        )

class TestDistroPackages(unittest.TestCase):
    def test_workflow_runs_install_script_after_checkout(self) -> None:
        text = _workflow()
        checkout = text.find("uses: actions/checkout@")
        install = re.search(
            r"(?m)^\s+run:\s*bash\s+\.github/scripts/install_distro_deps\.sh\s*$", text
        )
        self.assertNotEqual(checkout, -1)
        self.assertIsNotNone(install, "workflow must run install_distro_deps.sh")
        assert install is not None
        self.assertLess(checkout, install.start())

    def test_script_installs_gnu_ld_because_linux_links_use_it(self) -> None:
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        self.assertIn('"linux-x86_64": ["--ld-path=/usr/bin/ld"]', module)
        self.assertRegex(_install_script(), r"(?m)^common=\([^)]*\bbinutils\b")

    def test_pacman_skips_seccomp_sandbox_that_fails_under_emulation(self) -> None:
        """pacman 7's downloader seccomp filter fails (error 22) under amd64 emulation."""
        self.assertRegex(
            _install_script(), r"(?m)^\s+pacman\b[^\n]*\s--disable-sandbox-syscalls\b"
        )

    def test_script_installs_glibc_headers_for_each_family(self) -> None:
        script = _install_script()
        for family, headers in (
            ("dnf", "glibc-devel"),
            ("apt-get", "libc6-dev"),
            ("pacman", "glibc"),
            ("zypper", "glibc-devel"),
        ):
            with self.subTest(family=family):
                self.assertRegex(script, rf"(?m)^\s+{re.escape(family)}\b[^\n]*\b{headers}\b")


if __name__ == "__main__":
    unittest.main()
