#!/usr/bin/env python3
"""Guard that MemorySanitizer is wired into Bazel config and Linux CI."""

from __future__ import annotations

import ast
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


# Sentinel for an argument whose value is not a single string literal (a
# variable reference, a concatenation, ...), so it can't be read back from
# the source. Callers treat it as a possible match (fail closed) rather than
# as absent, so an override on a computed target can't slip past the guard.
_UNRESOLVED = object()


def _literal(node: ast.expr) -> object:
    """The node's value if it is a string literal, else _UNRESOLVED."""
    if isinstance(node, ast.Constant) and isinstance(node.value, str):
        return node.value
    return _UNRESOLVED


def _calls(call_names: tuple[str, ...], module_bazel: str) -> list[ast.Call]:
    """Every call, at any nesting depth, of a bare name in call_names.

    MODULE.bazel is Starlark, whose syntax is a subset of Python's, so
    Python's own parser handles strings, comments, line continuations and
    statement boundaries exactly. Attribute calls such as ext.bazel_dep(...)
    have an ast.Attribute callee and are excluded; grouping parens around a
    callable, as in (bazel_dep)(...), leave no trace in the tree.
    """
    return [
        node
        for node in ast.walk(ast.parse(module_bazel))
        if isinstance(node, ast.Call)
        and isinstance(node.func, ast.Name)
        and node.func.id in call_names
    ]


def _argument(call: ast.Call, keyword: str, position: int) -> object | None:
    """The argument given as keyword or at position, or None if absent."""
    for kw in call.keywords:
        if kw.arg == keyword:
            return _literal(kw.value)
    if position < len(call.args):
        return _literal(call.args[position])
    return None


def _toolchains_llvm_bazel_dep_version(module_bazel: str) -> str:
    """Return the pinned toolchains_llvm bazel_dep version.

    bazel_dep's name and version may each be given positionally or by
    keyword.
    """
    for call in _calls(("bazel_dep",), module_bazel):
        if _argument(call, "name", 0) == "toolchains_llvm":
            version = _argument(call, "version", 1)
            if version is not None and version is not _UNRESOLVED:
                return version
    raise AssertionError('expected bazel_dep(... name = "toolchains_llvm" ...)')


def _has_toolchains_llvm_override(module_bazel: str) -> bool:
    """True if any Bazel module override directive targets toolchains_llvm.

    Covers all five override directives. Fails closed (returns True) for
    an override whose module_name is not a string literal, since its
    runtime value can't be read back from the source.
    """
    override_names = (
        "archive_override",
        "git_override",
        "local_path_override",
        "multiple_version_override",
        "single_version_override",
    )
    for call in _calls(override_names, module_bazel):
        module_name = _argument(call, "module_name", 0)
        if module_name == "toolchains_llvm" or module_name is _UNRESOLVED:
            return True
    return False


class TestMsanBazelConfig(unittest.TestCase):
    def test_bazelrc_defines_msan_config(self) -> None:
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        self.assertRegex(
            bazelrc,
            r"(?m)^build:msan\s+--features=msan\b",
            "expected build:msan to enable the toolchains_llvm msan feature",
        )
        self.assertRegex(
            bazelrc,
            r"(?m)^build:msan\s+--extra_toolchains=@llvm_toolchain_msan//:all\b",
            "expected build:msan to select the instrumented-libc++ toolchain",
        )
        self.assertRegex(
            bazelrc,
            r"(?m)^test:msan\s+--test_timeout=",
            "expected test:msan timeouts like the other sanitizer configs",
        )

    def test_sanitizer_test_timeouts_are_strictly_increasing(self) -> None:
        """--test_timeout is (short, moderate, long, eternal); values must escalate.

        Small solver tests need short=300 under sanitizers (default moderate
        budget) without timeout="moderate", which warns on fast local runs.
        Duplicate short/moderate values collapse the hierarchy.
        """
        bazelrc = (_repo_root() / ".bazelrc").read_text(encoding="utf-8")
        configs = ("asan", "tsan", "ubsan", "msan")
        pattern = re.compile(
            r"(?m)^test:(%s)\s+--test_timeout=(\d+),(\d+),(\d+),(\d+)\s*$"
            % "|".join(configs)
        )
        found = {
            match.group(1): tuple(int(match.group(i)) for i in range(2, 6))
            for match in pattern.finditer(bazelrc)
        }
        self.assertEqual(
            set(found),
            set(configs),
            "expected --test_timeout for asan, tsan, ubsan, and msan",
        )
        timeout_values = set(found.values())
        self.assertEqual(
            len(timeout_values),
            1,
            f"expected identical four-value --test_timeout across sanitizers, got {found}",
        )
        for name, timeouts in found.items():
            short, moderate, long, eternal = timeouts
            self.assertGreaterEqual(
                short,
                300,
                f"test:{name} short timeout must keep sanitizer headroom for size=small",
            )
            self.assertLess(
                short,
                moderate,
                f"test:{name} short must be < moderate ({timeouts})",
            )
            self.assertLess(
                moderate,
                long,
                f"test:{name} moderate must be < long ({timeouts})",
            )
            self.assertLess(
                long,
                eternal,
                f"test:{name} long must be < eternal ({timeouts})",
            )

    def test_module_defines_msan_toolchain_with_instrumented_libcxx(self) -> None:
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        self.assertIn(
            'name = "llvm_toolchain_msan"',
            module,
            "expected a dedicated llvm_toolchain_msan for instrumented libc++",
        )
        self.assertRegex(
            module,
            r"libcxx_url\s*=",
            "msan requires an instrumented libc++ archive via libcxx_url",
        )
        self.assertRegex(
            module,
            r"libcxx_sha256\s*=",
            "msan libcxx_url must be pinned with libcxx_sha256",
        )

    def test_module_pins_toolchains_llvm_past_unused_stdlib_fix(self) -> None:
        """BCR 1.8.0 emitted unused -stdlib=libc++, breaking Linux -Werror builds.

        toolchains_llvm#791 dropped the redundant flag; it shipped in the 1.9.0
        release, so a plain bazel_dep replaces the archive_override this
        project used to carry to pin a specific pre-release commit.
        """
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        version = tuple(
            int(p) for p in _toolchains_llvm_bazel_dep_version(module).split(".")
        )
        self.assertGreaterEqual(
            version,
            (1, 9, 0),
            "toolchains_llvm must stay >= 1.9.0 (drops unused -stdlib=libc++, #791)",
        )
        self.assertFalse(
            _has_toolchains_llvm_override(module),
            "toolchains_llvm no longer needs an archive_override pin",
        )

    def test_toolchains_llvm_bazel_dep_version_tolerates_reordered_args(
        self,
    ) -> None:
        sample = 'bazel_dep(version = "1.11.2", name = "toolchains_llvm")'
        self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.11.2")

    def test_toolchains_llvm_bazel_dep_version_accepts_positional_name(
        self,
    ) -> None:
        """bazel_dep's name is positional too: bazel_dep("toolchains_llvm",

        version = ...) is valid Starlark.
        """
        sample = 'bazel_dep("toolchains_llvm", version = "1.11.2")'
        self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.11.2")

    def test_toolchains_llvm_bazel_dep_version_accepts_positional_version(
        self,
    ) -> None:
        """version is also allowed positionally, as bazel_dep's second

        argument: bazel_dep("toolchains_llvm", "1.11.2") is valid
        Starlark, not just the keyword form.
        """
        sample = 'bazel_dep("toolchains_llvm", "1.11.2")'
        self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.11.2")

    def test_toolchains_llvm_bazel_dep_version_ignores_commented_out_call(
        self,
    ) -> None:
        sample = """# bazel_dep(name = "toolchains_llvm", version = "9.9.9")
bazel_dep(name = "toolchains_llvm", version = "1.11.2")
"""
        self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.11.2")

    def test_toolchains_llvm_bazel_dep_version_ignores_fake_assignment_in_string(
        self,
    ) -> None:
        """Assignment-shaped text inside an unrelated string value for a

        different module's bazel_dep must not be read as real args.
        """
        sample = """bazel_dep(
    name = "rules_cc",
    version = "0.2.26",
    patch_cmds = ['historically name = "toolchains_llvm", version = "9.9.9"'],
)"""
        with self.assertRaises(AssertionError):
            _toolchains_llvm_bazel_dep_version(sample)

    def test_toolchains_llvm_bazel_dep_version_ignores_fake_call_in_triple_quote(
        self,
    ) -> None:
        """A fake bazel_dep(...) inside a triple-quoted string -- even one

        with an odd number of embedded quote characters before it, which
        would desync a naive per-quote toggle -- must stay opaque.
        """
        sample = '''"""
Note: the old pin "mentioned here used
bazel_dep(name = "toolchains_llvm", version = "9.9.9")
"""
bazel_dep(name = "toolchains_llvm", version = "1.11.2")
'''
        self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.11.2")

    def test_toolchains_llvm_bazel_dep_version_rejects_identifier_substring(
        self,
    ) -> None:
        """"bazel_dep" appearing inside a longer identifier, or as a

        qualified/attribute call, is not a real bazel_dep call.
        """
        for sample in (
            'legacy_bazel_dep(name = "toolchains_llvm", version = "9.9.9")',
            'bazel_depfoo(name = "toolchains_llvm", version = "9.9.9")',
            'extensions.bazel_dep(name = "toolchains_llvm", version = "9.9.9")',
        ):
            with self.assertRaises(AssertionError):
                _toolchains_llvm_bazel_dep_version(sample)

    def test_toolchains_llvm_bazel_dep_version_ignores_spaced_attribute_call(
        self,
    ) -> None:
        """"ext. bazel_dep(...)" is still an attribute call, whatever

        whitespace, line continuation, or comment separates the "." from
        the name; only the real top-level bazel_dep's version counts.
        """
        for qualifier, close in (("ext. ", ""), ("ext.\\\n", ""), ("[ext. # note\n", "]")):
            sample = (
                f'{qualifier}bazel_dep(name = "toolchains_llvm", version = "9.9.9"){close}\n'
                'bazel_dep(name = "toolchains_llvm", version = "1.9.0")'
            )
            with self.subTest(qualifier=qualifier):
                self.assertEqual(_toolchains_llvm_bazel_dep_version(sample), "1.9.0")

    def test_toolchains_llvm_bazel_dep_version_rejects_versionless_call(
        self,
    ) -> None:
        """Tokens inside a keyword's value are never positional arguments.

        Here no version is pinned at all; the "9.9.9" literal is just the
        tail of repo_name's concatenated value and must not be read back
        as bazel_dep's second positional (version) argument.
        """
        sample = (
            'bazel_dep(name = "toolchains_llvm", dev_dependency = False, '
            'repo_name = "toolchains_llvm_" + "9.9.9")'
        )
        with self.assertRaises(AssertionError):
            _toolchains_llvm_bazel_dep_version(sample)

    def test_toolchains_llvm_bazel_dep_version_keeps_statements_separate(
        self,
    ) -> None:
        """A newline outside brackets ends a statement, so a callable on one
        line and a tuple on the next are two statements, not a call.
        """
        for callable_expr in ("bazel_dep", "(bazel_dep)"):
            sample = (
                f"dep_function = {callable_expr}\n"
                '("toolchains_llvm", "9.9.9")\n'
                'bazel_dep(name = "toolchains_llvm", version = "1.11.2")\n'
            )
            with self.subTest(callable_expr=callable_expr):
                self.assertEqual(
                    _toolchains_llvm_bazel_dep_version(sample), "1.11.2"
                )

    def test_has_toolchains_llvm_override_keeps_statements_separate(self) -> None:
        for callable_expr in ("archive_override", "(archive_override)"):
            sample = f'override = {callable_expr}\n("toolchains_llvm", "x")\n'
            with self.subTest(callable_expr=callable_expr):
                self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_same_line_args(self) -> None:
        sample = (
            'archive_override(module_name = "toolchains_llvm", '
            'strip_prefix = "toolchains_llvm-abc123")'
        )
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_positional_module_name(
        self,
    ) -> None:
        """The override APIs accept module_name as their first positional

        argument; archive_override("toolchains_llvm", ...) is as real an
        override as the keyword form.
        """
        sample = (
            'archive_override("toolchains_llvm", '
            'strip_prefix = "toolchains_llvm-abc123")'
        )
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_parenthesized_callable_indirection(
        self,
    ) -> None:
        """Grouping parens around any expression are legal Starlark, and

        the result can be called immediately: (archive_override)(...) is
        a valid call equivalent to archive_override(...), but doesn't
        match a bare "name(" call textually.
        """
        sample = (
            '(archive_override)(module_name = "toolchains_llvm", '
            'strip_prefix = "toolchains_llvm-abc123")'
        )
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_rejects_longer_identifier_in_parens(
        self,
    ) -> None:
        """"(archive_override_foo)(...)" must not be misread as a

        parenthesized reference to archive_override.
        """
        sample = (
            '(archive_override_foo)(module_name = "toolchains_llvm", '
            'strip_prefix = "x")'
        )
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_fails_closed_on_non_literal_module_name(
        self,
    ) -> None:
        """A module_name that is not a single string literal can't be read
        back from the source; the guard must treat it as a possible
        toolchains_llvm override rather than as some other module.
        """
        for module_name in (
            "module_name = target",
            "target",
            '("toolchains_" + "llvm")',
            '["toolchains_llvm"][0]',
            '{"toolchains_llvm": 1}.keys()[0]',
            'module_name = "toolchains_" + "llvm"',
            'module_name = "xtoolchains_llvm"[1:]',
            'module_name = "toolchains_%s" % "llvm"',
        ):
            sample = f'archive_override({module_name}, strip_prefix = "x")'
            with self.subTest(module_name=module_name):
                self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_positional_string_in_list(
        self,
    ) -> None:
        """A bare string that is an *element* of a list value (not the

        call's own first positional argument) must not be read as
        module_name.
        """
        sample = """archive_override(
    module_name = "apple_support",
    urls = ["toolchains_llvm"],
)"""
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_reordered_args(self) -> None:
        sample = """
archive_override(
    strip_prefix = "toolchains_llvm-abc123",
    module_name = "toolchains_llvm",
)
"""
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_commented_out_call(
        self,
    ) -> None:
        sample = (
            '# archive_override(module_name = "toolchains_llvm", '
            'strip_prefix = "x")\n'
        )
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_other_modules(self) -> None:
        sample = """
single_version_override(
    module_name = "apple_support",
    version = "2.10.1",
)
"""
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_fake_assignment_in_string(
        self,
    ) -> None:
        """Assignment-shaped text inside an unrelated string value for a

        different module's override must not be read as real args.
        """
        sample = """single_version_override(
    module_name = "apple_support",
    version = "2.10.1",
    patch_cmds = ['the module_name = "toolchains_llvm" override is legacy'],
)"""
        self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_nested_override_call(
        self,
    ) -> None:
        """A recognized call nested inside another call's argument value

        (e.g. patches = single_version_override(...) or []) is still a
        real override Bazel executes, not inert text; it must not be
        silently swallowed while the outer call's own argument list is
        paren-balanced.
        """
        sample = (
            'single_version_override(module_name = "apple_support", '
            'patches = single_version_override(module_name = "toolchains_llvm", '
            'version = "1.11.2") or [])'
        )
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_ignores_spaced_attribute_call(
        self,
    ) -> None:
        """"ext. archive_override(...)" is an attribute call, not the

        archive_override directive, whatever separates the "." from the
        name.
        """
        for qualifier, close in (("ext. ", ""), ("ext.\\\n", ""), ("[ext. # note\n", "]")):
            sample = (
                f'{qualifier}archive_override(module_name = "toolchains_llvm", '
                f'urls = ["x"]){close}'
            )
            with self.subTest(qualifier=qualifier):
                self.assertFalse(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_local_path_override(self) -> None:
        sample = """
local_path_override(
    module_name = "toolchains_llvm",
    path = "../toolchains_llvm",
)
"""
        self.assertTrue(_has_toolchains_llvm_override(sample))

    def test_has_toolchains_llvm_override_detects_multiple_version_override(
        self,
    ) -> None:
        sample = """
multiple_version_override(
    module_name = "toolchains_llvm",
    versions = ["1.9.0", "1.11.2"],
)
"""
        self.assertTrue(_has_toolchains_llvm_override(sample))


class TestMsanLinuxCi(unittest.TestCase):
    def test_ci_linux_runs_msan_job(self) -> None:
        text = (_repo_root() / ".github" / "workflows" / "ci_linux.yml").read_text(
            encoding="utf-8"
        )
        self.assertRegex(
            text,
            r"(?m)^  msan:\s*$",
            "expected a dedicated msan job in ci_linux.yml",
        )
        # MODULE.bazel pins the MSAN LLVM + instrumented libc++ to the Ubuntu
        # 22.04 distribution; keep the runner on that OS so the overlay matches.
        self.assertRegex(
            text,
            r"(?m)^  msan:\s*\n(?:[ \t]+[^\n]*\n)*?[ \t]+runs-on:\s*ubuntu-22\.04\s*$",
            "msan CI must pin ubuntu-22.04 to match the MSAN toolchain overlay",
        )
        self.assertRegex(
            text,
            r"bazelisk\s+test\s+--config=msan\b",
            "expected Linux CI to run tests under --config=msan",
        )
        # MSAN is Linux-only; keep the job scoped to library tests like ASAN/UBSAN.
        self.assertRegex(
            text,
            r"bazelisk\s+test\s+--config=msan\b[^\n]*//library/tests/",
            "expected msan CI to exercise //library/tests/...",
        )


class TestMsanNotOnMacosCi(unittest.TestCase):
    def test_ci_macos_does_not_run_msan(self) -> None:
        text = (_repo_root() / ".github" / "workflows" / "ci_macos.yml").read_text(
            encoding="utf-8"
        )
        self.assertIsNone(
            re.search(r"--config=msan\b", text),
            "MSAN is Linux-only; macOS CI must not enable --config=msan",
        )


class TestMsanCompatibleParallelBoardsAllocHooks(unittest.TestCase):
    def test_custom_new_delete_are_disabled_under_memory_sanitizer(self) -> None:
        """MSAN's runtime owns operator new/delete; custom defs duplicate-symbol.

        parallel_boards_test.cpp overrides global new/delete for allocation
        counting. Those symbols must not be compiled under MemorySanitizer.
        """
        path = (
            _repo_root()
            / "library"
            / "tests"
            / "system"
            / "parallel_boards_test.cpp"
        )
        text = path.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r"void\*\s+operator\s+new\s*\(",
            "expected a custom operator new for allocation tracking",
        )
        self.assertRegex(
            text,
            r"__has_feature\s*\(\s*memory_sanitizer\s*\)",
            "expected MSAN detection via __has_feature(memory_sanitizer)",
        )
        # Both Clang feature-test and GCC-style __SANITIZE_MEMORY__ may be set;
        # defining DDS_TEST_MEMORY_SANITIZER twice triggers -Wmacro-redefined.
        self.assertRegex(
            text,
            r"#\s*ifndef\s+DDS_TEST_MEMORY_SANITIZER[\s\S]*?"
            r"#\s*if\s+defined\s*\(\s*__SANITIZE_MEMORY__\s*\)",
            "second MSAN detect must be guarded to avoid macro redefinition",
        )
        self.assertEqual(
            len(re.findall(r"#\s*define\s+DDS_TEST_MEMORY_SANITIZER\s+1\b", text)),
            2,
            "expected two paths that set DDS_TEST_MEMORY_SANITIZER to 1",
        )
        # Custom replacements must not be linked under MSAN (duplicate symbols).
        self.assertRegex(
            text,
            r"#\s*if\s*!DDS_TEST_MEMORY_SANITIZER[\s\S]*?"
            r"void\*\s+operator\s+new\s*\(",
            "custom operator new must be compiled only when MSAN is off",
        )


if __name__ == "__main__":
    unittest.main()
