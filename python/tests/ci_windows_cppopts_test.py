#!/usr/bin/env python3
"""Guard that Windows MSVC flags do not fight Bazel's defaults (D9025)."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() — under `bazel test` this file is often a runfiles
    # symlink into the execroot/source tree, and resolving leaves the runfiles
    # tree where //.bazelrc and other data deps live.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        if (parent / ".bazelrc").is_file() and (parent / "CPPVARIABLES.bzl").is_file():
            return parent
    raise AssertionError("could not locate repository root from test file path")


def _windows_cppopts_block(cppvariables: str) -> str:
    """Return the string list body for the //:build_windows select arm."""
    match = re.search(
        r'"//:build_windows":\s*\[(.*?)\]',
        cppvariables,
        flags=re.DOTALL,
    )
    if match is None:
        raise AssertionError("expected //:build_windows arm in DDS_CPPOPTS")
    return match.group(1)


def _debug_windows_cppopts_block(cppvariables: str) -> str:
    match = re.search(
        r'"//:debug_build_windows":\s*\[(.*?)\]',
        cppvariables,
        flags=re.DOTALL,
    )
    if match is None:
        raise AssertionError("expected //:debug_build_windows arm in DDS_CPPOPTS")
    return match.group(1)


# MSVC optimisation-level flags that fight Bazel's compilation_mode (/Od vs /O2
# D9025). Broader than /Od|/O2 so /O1, /Ox, etc. cannot sneak back in.
_MSVC_OPT_LEVEL_COPT = re.compile(r'"/O[0-9a-zA-Z]')


def _bazelisk_invocation_has_config_opt(text: str, subcommand: str) -> bool:
    """True if any bazelisk <subcommand> invocation includes --config=opt.

    Flag order after the subcommand is not significant, and a YAML `run:`
    prefix on the same line is allowed. Full-line comments (leading `#`) and
    trailing inline `# ...` comments are ignored so a commented-out
    `--config=opt` cannot satisfy the CI guard.
    """
    pattern = re.compile(
        rf"^[ \t]*(?:run:[ \t]+)?bazelisk\s+{re.escape(subcommand)}\b.*--config=opt\b"
    )
    for line in text.splitlines():
        code = line.split("#", 1)[0]
        if not code.strip():
            continue
        if pattern.search(code):
            return True
    return False


class TestWindowsMsvcCppoptsAvoidD9025(unittest.TestCase):
    def test_windows_cppopts_do_not_override_bazel_optimization(self) -> None:
        """Bazel already sets /Od (fastbuild/dbg) or /O2 (opt); re-stating any
        MSVC /O* level flag in DDS_CPPOPTS produces cl D9025.
        """
        text = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        for label, block in (
            ("build_windows", _windows_cppopts_block(text)),
            ("debug_build_windows", _debug_windows_cppopts_block(text)),
        ):
            self.assertNotRegex(
                block,
                _MSVC_OPT_LEVEL_COPT,
                f"{label} must not set /O* level flags; use compilation_mode instead",
            )

    def test_msvc_opt_level_copt_rejects_common_overrides(self) -> None:
        for flag in ('"/Od"', '"/O1"', '"/O2"', '"/Ox"', '"/Og"'):
            self.assertRegex(
                flag,
                _MSVC_OPT_LEVEL_COPT,
                f"expected {flag} to be treated as an optimisation-level override",
            )
        self.assertNotRegex(
            '"/W4"',
            _MSVC_OPT_LEVEL_COPT,
            "/W4 is a warning flag, not an /O* level override",
        )

    def test_windows_cppopts_do_not_override_toolchain_cxx_standard(self) -> None:
        """MSVC /std comes from the patched rules_cc default_cpp_std feature
        (/std:c++20). Restating it in DDS_CPPOPTS yields cl D9025 against that
        default. Keep language-standard selection out of build:windows --cxxopt
        so wasm transitions on Windows hosts do not see MSVC /std flags.
        """
        text = (_repo_root() / "CPPVARIABLES.bzl").read_text(encoding="utf-8")
        for label, block in (
            ("build_windows", _windows_cppopts_block(text)),
            ("debug_build_windows", _debug_windows_cppopts_block(text)),
        ):
            self.assertNotRegex(
                block,
                r'"/std:',
                f"{label} must not set /std:; MSVC default_cpp_std is C++20",
            )

    def test_windows_bazelrc_keeps_default_cpp_std_without_host_cxxopt(self) -> None:
        """default_cpp_std (patched to /std:c++20) covers googletest and every
        MSVC cc_* compile. Do not disable it, and do not add build:windows
        --cxxopt=/std (leaks into wasm).
        """
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        self.assertIsNone(
            re.search(
                r"(?m)^build:windows\s+--features=-default_cpp_std\b",
                bazelrc,
            ),
            "build:windows must not disable default_cpp_std",
        )
        self.assertIsNone(
            re.search(r"(?m)^build:windows\s+--cxxopt=/std:", bazelrc),
            "build:windows must not set --cxxopt=/std:... (leaks into wasm)",
        )
        self.assertIsNone(
            re.search(r"(?m)^build:windows\s+--host_cxxopt=/std:", bazelrc),
            "build:windows must not set --host_cxxopt=/std:...",
        )


def _rules_cc_bazel_dep_version(module_bazel: str) -> str:
    for match in re.finditer(r'bazel_dep\(\s*([^)]*)\)', module_bazel, flags=re.DOTALL):
        body = match.group(1)
        name = re.search(r'name\s*=\s*"rules_cc"', body)
        version = re.search(r'version\s*=\s*"([^"]+)"', body)
        if name and version:
            return version.group(1)
    raise AssertionError('expected bazel_dep(... name = "rules_cc" ... version = "...")')


def _rules_cc_override_version(module_bazel: str) -> str:
    patch = r'"//:patches/rules_cc_msvc_default_cpp_std_cxx20\.patch"'
    for match in re.finditer(
        r"single_version_override\(\s*([^)]*)\)",
        module_bazel,
        flags=re.DOTALL,
    ):
        body = match.group(1)
        if re.search(r'module_name\s*=\s*"rules_cc"', body) is None:
            continue
        if re.search(patch, body) is None:
            continue
        if re.search(r"patch_strip\s*=\s*1\b", body) is None:
            continue
        version = re.search(r'version\s*=\s*"([^"]+)"', body)
        if version:
            return version.group(1)
    raise AssertionError(
        "expected single_version_override(module_name=\"rules_cc\", version=..., "
        "patches=[//:patches/rules_cc_msvc_default_cpp_std_cxx20.patch], patch_strip=1)"
    )


def _exports_files_python_visibility_block(build_bazel: str) -> str | None:
    """Return the exports_files(...) body for the CI-guard config exports."""
    for match in re.finditer(
        r"exports_files\(\s*(\[[^\]]*\])\s*,\s*visibility\s*=\s*\[[^\]]*\]\s*,?\s*\)",
        build_bazel,
        flags=re.DOTALL,
    ):
        block = match.group(1)
        if re.search(r'"\.bazelrc"', block) and re.search(
            r'"patches/rules_cc_msvc_default_cpp_std_cxx20\.patch"',
            block,
        ):
            return block
    return None


class TestRulesCcMsvcDefaultCppStdCxx20(unittest.TestCase):
    _PATCH = "patches/rules_cc_msvc_default_cpp_std_cxx20.patch"

    def test_module_applies_rules_cc_msvc_cxx20_patch(self) -> None:
        """Patch wiring must track MODULE.bazel versions — do not hard-code the
        rules_cc pin in this guard (versions live only in MODULE.bazel / lock).
        """
        text = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        dep_version = _rules_cc_bazel_dep_version(text)
        override_version = _rules_cc_override_version(text)
        self.assertEqual(
            dep_version,
            override_version,
            "rules_cc bazel_dep version must match single_version_override "
            "version so the MSVC C++20 patch applies to the resolved module",
        )

    def test_rules_cc_version_helpers_reject_missing_wiring(self) -> None:
        with self.assertRaises(AssertionError):
            _rules_cc_bazel_dep_version('bazel_dep(name = "platforms", version = "1.0")')
        with self.assertRaises(AssertionError):
            _rules_cc_override_version('bazel_dep(name = "rules_cc", version = "9.9.9")')

    def test_rules_cc_version_helpers_accept_any_matching_pin(self) -> None:
        sample = """
bazel_dep(name = "rules_cc", version = "1.2.3")
single_version_override(
    module_name = "rules_cc",
    version = "1.2.3",
    patches = ["//:patches/rules_cc_msvc_default_cpp_std_cxx20.patch"],
    patch_strip = 1,
)
"""
        self.assertEqual(_rules_cc_bazel_dep_version(sample), "1.2.3")
        self.assertEqual(_rules_cc_override_version(sample), "1.2.3")

    def test_rules_cc_version_helpers_tolerate_reordered_args(self) -> None:
        sample = """
bazel_dep(
    version = "4.5.6",
    name = "rules_cc",
)
single_version_override(
    patch_strip = 1,
    patches = ["//:patches/rules_cc_msvc_default_cpp_std_cxx20.patch"],
    version = "4.5.6",
    module_name = "rules_cc",
    # extra fields must not break the guard
    registry = "https://example.com",
)
"""
        self.assertEqual(_rules_cc_bazel_dep_version(sample), "4.5.6")
        self.assertEqual(_rules_cc_override_version(sample), "4.5.6")

    def test_patch_raises_msvc_default_cpp_std_to_cxx20(self) -> None:
        patch = (_repo_root() / self._PATCH).read_text(encoding="utf-8")
        self.assertIn(
            "windows_cc_toolchain_config.bzl",
            patch,
            f"{self._PATCH} must target windows_cc_toolchain_config.bzl",
        )
        self.assertRegex(
            patch,
            r"(?m)^-\s*flags = \[\"/std:c\+\+17\"\],",
            f"{self._PATCH} must remove /std:c++17 from default_cpp_std",
        )
        self.assertRegex(
            patch,
            r"(?m)^\+\s*flags = \[\"/std:c\+\+20\"\],",
            f"{self._PATCH} must set /std:c++20 in default_cpp_std",
        )


class TestBazeliskConfigOptMatching(unittest.TestCase):
    def test_matches_when_config_opt_is_first_flag(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "bazelisk build --config=opt //...",
                "build",
            )
        )

    def test_matches_when_other_flags_precede_config_opt(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "bazelisk build --verbose_failures --config=opt //...",
                "build",
            )
        )

    def test_matches_when_config_opt_is_not_adjacent_to_subcommand(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "    bazelisk test --test_output=errors --config=opt //...",
                "test",
            )
        )

    def test_matches_yaml_run_prefix_on_same_line(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "        run: bazelisk test --verbose_failures --config=opt //...",
                "test",
            )
        )

    def test_rejects_missing_config_opt(self) -> None:
        self.assertFalse(
            _bazelisk_invocation_has_config_opt(
                "bazelisk build --verbose_failures //...",
                "build",
            )
        )

    def test_rejects_config_opt_on_unrelated_subcommand(self) -> None:
        self.assertFalse(
            _bazelisk_invocation_has_config_opt(
                "bazelisk fetch --config=opt //...\nbazelisk build //...",
                "build",
            )
        )

    def test_rejects_commented_out_bazelisk_invocation(self) -> None:
        self.assertFalse(
            _bazelisk_invocation_has_config_opt(
                "# bazelisk build --config=opt //...\nbazelisk build //...",
                "build",
            )
        )

    def test_rejects_indented_commented_out_bazelisk_invocation(self) -> None:
        self.assertFalse(
            _bazelisk_invocation_has_config_opt(
                "        # run: bazelisk test --config=opt //...\n"
                "        run: bazelisk test --verbose_failures //...",
                "test",
            )
        )

    def test_matches_active_line_when_commented_sibling_exists(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "# bazelisk build //...\n"
                "bazelisk build --verbose_failures --config=opt //...",
                "build",
            )
        )

    def test_rejects_trailing_comment_containing_config_opt(self) -> None:
        self.assertFalse(
            _bazelisk_invocation_has_config_opt(
                "bazelisk build //... # --config=opt\n",
                "build",
            )
        )

    def test_rejects_yaml_run_line_with_trailing_config_opt_comment(self) -> None:
        self.assertFalse(
            _bazelisk_invocation_has_config_opt(
                "        run: bazelisk test --verbose_failures //...  # --config=opt\n",
                "test",
            )
        )

    def test_matches_when_config_opt_precedes_trailing_comment(self) -> None:
        self.assertTrue(
            _bazelisk_invocation_has_config_opt(
                "bazelisk build --config=opt //...  # release codegen\n",
                "build",
            )
        )


def _windows_ci_workflow_text() -> str:
    return (
        _repo_root() / ".github" / "workflows" / "ci_windows.yml"
    ).read_text(encoding="utf-8")


def _workflow_job_bodies(text: str) -> dict[str, str]:
    """Map top-level GitHub Actions job ids under `jobs:` to their bodies."""
    jobs: dict[str, list[str]] = {}
    current: str | None = None
    in_jobs = False
    for line in text.splitlines():
        if re.match(r"^jobs:\s*$", line):
            in_jobs = True
            current = None
            continue
        if not in_jobs:
            continue
        job_header = re.match(r"^  ([A-Za-z0-9_-]+):\s*$", line)
        if job_header:
            current = job_header.group(1)
            jobs[current] = []
            continue
        if current is None:
            continue
        if line.startswith("  ") or not line.strip():
            jobs[current].append(line)
        else:
            current = None
    return {name: "\n".join(body) for name, body in jobs.items()}


def _active_bazelisk_lines(text: str, subcommand: str) -> list[str]:
    """Return non-comment lines that invoke bazelisk <subcommand>."""
    lines: list[str] = []
    for line in text.splitlines():
        code = line.split("#", 1)[0]
        if re.search(rf"bazelisk\s+{re.escape(subcommand)}\b", code):
            lines.append(code)
    return lines


def _excludes_package_pattern(line: str, package: str) -> bool:
    """True if a Bazel target-pattern list excludes //<package>/...

    The exclusion must be quoted (`'-//pkg/...'` or `\"-//pkg/...\"`). On
    Windows CI the default shell is pwsh, which treats a bare `-//...` token as
    a PowerShell switch and strips Bazel's `--` end-of-options marker, so
    unquoted exclusions fail with \"Invalid options syntax\".
    """
    return bool(
        re.search(
            rf"""(?<![\w/])['"]-//{re.escape(package)}/\.\.\.['"](?!\S)""",
            line,
        )
    )


def _includes_all_packages_pattern(line: str) -> bool:
    """True if the line includes a (optionally quoted) //... target pattern."""
    return bool(re.search(r"""(?<![\w/])['"]?//\.\.\.['"]?(?!\S)""", line))


class TestWorkflowJobBodies(unittest.TestCase):
    def test_splits_jobs_by_id(self) -> None:
        sample = """
name: sample
jobs:
  native:
    runs-on: windows-latest
    steps:
      - run: echo native
  wasm_web:
    runs-on: windows-latest
    steps:
      - run: echo wasm
"""
        bodies = _workflow_job_bodies(sample)
        self.assertEqual(set(bodies), {"native", "wasm_web"})
        self.assertIn("echo native", bodies["native"])
        self.assertIn("echo wasm", bodies["wasm_web"])
        self.assertNotIn("echo wasm", bodies["native"])


class TestPwshQuotedExclusionPatterns(unittest.TestCase):
    def test_requires_quotes_around_negative_patterns(self) -> None:
        self.assertFalse(
            _excludes_package_pattern(
                "bazelisk fetch -- //... -//wasm/... -//web/...",
                "wasm",
            ),
            "bare -//wasm/... is unsafe under pwsh",
        )
        self.assertTrue(
            _excludes_package_pattern(
                "bazelisk fetch -- '//...' '-//wasm/...' '-//web/...'",
                "wasm",
            )
        )
        self.assertTrue(
            _excludes_package_pattern(
                'bazelisk build -- "//..." "-//web/..."',
                "web",
            )
        )

    def test_includes_all_packages_accepts_quoted_or_bare(self) -> None:
        self.assertTrue(_includes_all_packages_pattern("bazelisk fetch -- //..."))
        self.assertTrue(
            _includes_all_packages_pattern("bazelisk fetch -- '//...' '-//wasm/...'")
        )


class TestWindowsCiUsesOpt(unittest.TestCase):
    def test_windows_ci_passes_config_opt(self) -> None:
        """Without /O2 in DDS_CPPOPTS, CI must opt in via --config=opt."""
        text = _windows_ci_workflow_text()
        for subcommand in ("build", "test"):
            lines = _active_bazelisk_lines(text, subcommand)
            self.assertTrue(lines, f"expected at least one bazelisk {subcommand}")
            for line in lines:
                self.assertRegex(
                    line,
                    r"--config=opt\b",
                    f"expected every Windows CI {subcommand} to use --config=opt: {line.strip()}",
                )

    def test_windows_ci_test_prints_failing_output(self) -> None:
        """Python test.log is not in the Windows bazel-testlogs artifact."""
        text = _windows_ci_workflow_text()
        test_lines = _active_bazelisk_lines(text, "test")
        self.assertTrue(test_lines, "expected at least one bazelisk test")
        for line in test_lines:
            self.assertIn(
                "--test_output=errors",
                line,
                "expected every Windows CI test to use --test_output=errors so "
                f"failing unittest output appears in the job log: {line.strip()}",
            )


class TestWindowsCiSplitsWasmWeb(unittest.TestCase):
    """Keep emsdk/wasm work off the native Windows critical path."""

    def test_has_parallel_native_and_wasm_web_jobs(self) -> None:
        bodies = _workflow_job_bodies(_windows_ci_workflow_text())
        self.assertIn(
            "build_and_test",
            bodies,
            "expected a native Windows job named build_and_test",
        )
        self.assertIn(
            "wasm_web",
            bodies,
            "expected a parallel Windows job named wasm_web for //wasm and //web",
        )

    def test_native_job_excludes_wasm_and_web_patterns(self) -> None:
        native = _workflow_job_bodies(_windows_ci_workflow_text())["build_and_test"]
        for subcommand in ("fetch", "build", "test"):
            lines = _active_bazelisk_lines(native, subcommand)
            self.assertTrue(
                lines,
                f"native job must invoke bazelisk {subcommand}",
            )
            for line in lines:
                self.assertTrue(
                    _includes_all_packages_pattern(line),
                    f"native {subcommand} should still cover //... : {line.strip()}",
                )
                for package in ("wasm", "web"):
                    self.assertTrue(
                        _excludes_package_pattern(line, package),
                        f"native {subcommand} must quote '-//{package}/...' "
                        f"for pwsh: {line.strip()}",
                    )

    def test_wasm_web_job_targets_only_wasm_and_web(self) -> None:
        wasm_web = _workflow_job_bodies(_windows_ci_workflow_text())["wasm_web"]
        for subcommand in ("fetch", "build", "test"):
            lines = _active_bazelisk_lines(wasm_web, subcommand)
            self.assertTrue(
                lines,
                f"wasm_web job must invoke bazelisk {subcommand}",
            )
            for line in lines:
                self.assertRegex(
                    line,
                    r"(?<![\w/])//wasm/\.\.\.(?!\S)",
                    f"wasm_web {subcommand} must include //wasm/... : {line.strip()}",
                )
                self.assertRegex(
                    line,
                    r"(?<![\w/])//web/\.\.\.(?!\S)",
                    f"wasm_web {subcommand} must include //web/... : {line.strip()}",
                )
                self.assertIsNone(
                    re.search(r"(?<![\w/-])//\.\.\.(?!\S)", line),
                    f"wasm_web {subcommand} must not use unscoped //... : "
                    f"{line.strip()}",
                )

    def test_job_log_artifacts_are_distinct(self) -> None:
        text = _windows_ci_workflow_text()
        bodies = _workflow_job_bodies(text)
        native_artifact = re.search(
            r"(?m)^\s+name:\s*(\S*bazel-test-logs\S*)\s*$",
            bodies["build_and_test"],
        )
        wasm_artifact = re.search(
            r"(?m)^\s+name:\s*(\S*bazel-test-logs\S*)\s*$",
            bodies["wasm_web"],
        )
        self.assertIsNotNone(native_artifact, "native job must upload test logs")
        self.assertIsNotNone(wasm_artifact, "wasm_web job must upload test logs")
        assert native_artifact is not None and wasm_artifact is not None
        self.assertNotEqual(
            native_artifact.group(1),
            wasm_artifact.group(1),
            "parallel Windows jobs need distinct artifact names",
        )


class TestWindowsCppoptsConfigExportsVisibility(unittest.TestCase):
    def test_exports_files_visibility_matches_filegroup(self) -> None:
        """exports_files defaults to public; keep it aligned with the restricted
        windows_cppopts_config_files filegroup so these roots are not world-readable
        labels.
        """
        text = (_repo_root() / "BUILD.bazel").read_text(encoding="utf-8")
        block = _exports_files_python_visibility_block(text)
        self.assertIsNotNone(
            block,
            "expected exports_files([...], visibility = [\"//python:__pkg__\"]) "
            "for CI guard config roots",
        )
        assert block is not None
        for entry in (
            ".bazelrc",
            "CPPVARIABLES.bzl",
            "MODULE.bazel",
            "patches/rules_cc_msvc_default_cpp_std_cxx20.patch",
        ):
            self.assertIn(
                f'"{entry}"',
                block,
                f"exports_files must include {entry}",
            )
        self.assertRegex(
            text,
            r"exports_files\(\s*\[[^\]]*\]\s*,\s*visibility\s*=\s*\[\s*\"//python:__pkg__\"\s*\]\s*,?\s*\)",
            'exports_files visibility must be restricted to ["//python:__pkg__"]',
        )

    def test_exports_files_guard_tolerates_reordered_entries(self) -> None:
        sample = """
exports_files(
    [
        "MODULE.bazel",
        ".bazelrc",
        "patches/rules_cc_msvc_default_cpp_std_cxx20.patch",
        "CPPVARIABLES.bzl",
    ],
    visibility = ["//python:__pkg__"],
)
"""
        block = _exports_files_python_visibility_block(sample)
        self.assertIsNotNone(block)
        assert block is not None
        self.assertIn('"MODULE.bazel"', block)
        self.assertIn('"patches/rules_cc_msvc_default_cpp_std_cxx20.patch"', block)


if __name__ == "__main__":
    unittest.main()
