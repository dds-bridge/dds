#!/usr/bin/env python3
"""Guard that apple_support stays past the release that fixed Xcode 27 ASAN
crosstool warnings, without re-adding the patches that release made obsolete.
"""

from __future__ import annotations

import ast
import unittest
from pathlib import Path


def _repo_root(start: Path | None = None) -> Path:
    # Do not Path.resolve() — under `bazel test` this file is often a runfiles
    # symlink into the execroot/source tree, and resolving leaves the runfiles
    # tree where //MODULE.bazel and other data deps live.
    here = (start or Path(__file__)).absolute()
    for parent in here.parents:
        if (parent / "MODULE.bazel").is_file() and (parent / ".bazelrc").is_file():
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
    """The argument given as keyword or at position, or None if absent.

    **kwargs (a keyword whose arg is None) may supply a missing argument
    without naming it, so it resolves to _UNRESOLVED rather than to absent.
    A *args at the requested position is an ast.Starred node, which
    _literal() already resolves to _UNRESOLVED. (A *args before it could
    shift later positions, but callers only read past position 0 once
    position 0 has resolved to a literal.)
    """
    for kw in call.keywords:
        if kw.arg == keyword:
            return _literal(kw.value)
    if position < len(call.args):
        return _literal(call.args[position])
    if any(kw.arg is None for kw in call.keywords):
        return _UNRESOLVED
    return None


def _apple_support_bazel_dep_version(module_bazel: str) -> str:
    """Return the pinned apple_support bazel_dep version.

    bazel_dep's name and version may each be given positionally or by
    keyword.
    """
    for call in _calls(("bazel_dep",), module_bazel):
        if _argument(call, "name", 0) == "apple_support":
            version = _argument(call, "version", 1)
            if version is not None and version is not _UNRESOLVED:
                return version
    raise AssertionError('expected bazel_dep(... name = "apple_support" ...)')


def _has_apple_support_override(module_bazel: str) -> bool:
    """True if any Bazel module override directive targets apple_support.

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
        if module_name == "apple_support" or module_name is _UNRESOLVED:
            return True
    return False


class TestAppleSupportAsanCrosstoolWarnings(unittest.TestCase):
    def test_module_pins_apple_support_past_xcode27_crosstool_fixes(self) -> None:
        """apple_support < 2.8.4 needs -mmacosx-version-min=11.0 and a dead
        nodiscard hasher(file) call patched out of its wrapped_clang/libtool
        crosstool helpers (used by --config=asan). Both are fixed upstream as
        of 2.8.4, so a plain bazel_dep replaces the single_version_override
        this project used to carry.
        """
        module = (_repo_root() / "MODULE.bazel").read_text(encoding="utf-8")
        version = tuple(
            int(p) for p in _apple_support_bazel_dep_version(module).split(".")
        )
        self.assertGreaterEqual(
            version,
            (2, 8, 4),
            "apple_support must stay >= 2.8.4 (Xcode 27 ASAN crosstool fixes)",
        )
        self.assertFalse(
            _has_apple_support_override(module),
            "apple_support no longer needs a single_version_override/patches",
        )

    def test_apple_support_patches_were_removed(self) -> None:
        root = _repo_root()
        self.assertFalse(
            (root / "patches" / "apple_support_macos_min_11.patch").exists(),
            "min-OS patch is obsolete once apple_support >= 2.8.4 is pinned",
        )
        self.assertFalse(
            (root / "patches" / "apple_support_libtool_nodiscard.patch").exists(),
            "libtool nodiscard patch is obsolete once apple_support >= 2.8.4 is pinned",
        )

    def test_apple_support_bazel_dep_version_tolerates_reordered_args(
        self,
    ) -> None:
        sample = 'bazel_dep(version = "2.10.1", name = "apple_support")'
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_apple_support_bazel_dep_version_accepts_positional_name(
        self,
    ) -> None:
        """bazel_dep's name is positional too: bazel_dep("apple_support",

        version = ...) is valid Starlark.
        """
        sample = 'bazel_dep("apple_support", version = "2.10.1")'
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_apple_support_bazel_dep_version_accepts_positional_version(
        self,
    ) -> None:
        """version is also allowed positionally, as bazel_dep's second

        argument: bazel_dep("apple_support", "2.10.1") is valid
        Starlark, not just the keyword form.
        """
        sample = 'bazel_dep("apple_support", "2.10.1")'
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_apple_support_bazel_dep_version_ignores_commented_out_call(
        self,
    ) -> None:
        sample = """# bazel_dep(name = "apple_support", version = "9.9.9")
bazel_dep(name = "apple_support", version = "2.10.1")
"""
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_apple_support_bazel_dep_version_ignores_fake_assignment_in_string(
        self,
    ) -> None:
        """Assignment-shaped text inside an unrelated string value for a

        different module's bazel_dep must not be read as real args.
        """
        sample = """bazel_dep(
    name = "rules_cc",
    version = "0.2.26",
    patch_cmds = ['historically name = "apple_support", version = "9.9.9"'],
)"""
        with self.assertRaises(AssertionError):
            _apple_support_bazel_dep_version(sample)

    def test_apple_support_bazel_dep_version_ignores_fake_call_in_triple_quote(
        self,
    ) -> None:
        """A fake bazel_dep(...) inside a triple-quoted string -- even one

        with an odd number of embedded quote characters before it, which
        would desync a naive per-quote toggle -- must stay opaque.
        """
        sample = '''"""
Note: the old pin "mentioned here used
bazel_dep(name = "apple_support", version = "9.9.9")
"""
bazel_dep(name = "apple_support", version = "2.10.1")
'''
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_apple_support_bazel_dep_version_rejects_identifier_substring(
        self,
    ) -> None:
        """"bazel_dep" appearing inside a longer identifier, or as a

        qualified/attribute call, is not a real bazel_dep call.
        """
        for sample in (
            'legacy_bazel_dep(name = "apple_support", version = "9.9.9")',
            'bazel_depfoo(name = "apple_support", version = "9.9.9")',
            'extensions.bazel_dep(name = "apple_support", version = "9.9.9")',
        ):
            with self.assertRaises(AssertionError):
                _apple_support_bazel_dep_version(sample)

    def test_apple_support_bazel_dep_version_ignores_spaced_attribute_call(
        self,
    ) -> None:
        """"ext. bazel_dep(...)" is still an attribute call, whatever

        whitespace, line continuation, or comment separates the "." from
        the name; only the real top-level bazel_dep's version counts.
        """
        for qualifier, close in (("ext. ", ""), ("ext.\\\n", ""), ("[ext. # note\n", "]")):
            sample = (
                f'{qualifier}bazel_dep(name = "apple_support", version = "9.9.9"){close}\n'
                'bazel_dep(name = "apple_support", version = "1.24.2")'
            )
            with self.subTest(qualifier=qualifier):
                self.assertEqual(_apple_support_bazel_dep_version(sample), "1.24.2")

    def test_apple_support_bazel_dep_version_rejects_versionless_call(
        self,
    ) -> None:
        """Tokens inside a keyword's value are never positional arguments.

        Here no version is pinned at all; the "9.9.9" literal is just the
        tail of repo_name's concatenated value and must not be read back
        as bazel_dep's second positional (version) argument.
        """
        sample = (
            'bazel_dep(name = "apple_support", dev_dependency = False, '
            'repo_name = "apple_support_" + "9.9.9")'
        )
        with self.assertRaises(AssertionError):
            _apple_support_bazel_dep_version(sample)

    def test_apple_support_bazel_dep_version_keeps_statements_separate(
        self,
    ) -> None:
        """A newline outside brackets ends a statement, so a callable on one
        line and a tuple on the next are two statements, not a call.
        """
        for callable_expr in ("bazel_dep", "(bazel_dep)"):
            sample = (
                f"dep_function = {callable_expr}\n"
                '("apple_support", "9.9.9")\n'
                'bazel_dep(name = "apple_support", version = "1.24.2")\n'
            )
            with self.subTest(callable_expr=callable_expr):
                self.assertEqual(_apple_support_bazel_dep_version(sample), "1.24.2")

    def test_has_apple_support_override_keeps_statements_separate(self) -> None:
        for callable_expr in ("archive_override", "(archive_override)"):
            sample = f'override = {callable_expr}\n("apple_support", "x")\n'
            with self.subTest(callable_expr=callable_expr):
                self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_same_line_args(self) -> None:
        sample = (
            'single_version_override(module_name = "apple_support", version = "1.24.2")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_positional_module_name(
        self,
    ) -> None:
        """The override APIs accept module_name as their first positional

        argument; archive_override("apple_support", ...) is as real an
        override as the keyword form.
        """
        sample = (
            'archive_override("apple_support", '
            'urls = ["https://example.com/apple_support.tar.gz"])'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_non_literal_module_name(
        self,
    ) -> None:
        """A module_name that is not a single string literal can't be read
        back from the source; the guard must treat it as a possible
        apple_support override rather than as some other module.
        """
        for module_name in (
            "module_name = target",
            "target",
            '("apple_" + "support")',
            '["apple_support"][0]',
            '{"apple_support": 1}.keys()[0]',
            'module_name = "apple_" + "support"',
            'module_name = "xapple_support"[1:]',
            'module_name = "apple_%s" % "support"',
        ):
            sample = f'archive_override({module_name}, strip_prefix = "x")'
            with self.subTest(module_name=module_name):
                self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_keyword_unpacking(
        self,
    ) -> None:
        """**kwargs may supply module_name, whose value can't be read back."""
        for unpacked in (
            '**{"module_name": "apple_support", "path": "../apple_support"}',
            "**override_kwargs",
        ):
            sample = f"local_path_override({unpacked})"
            with self.subTest(unpacked=unpacked):
                self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_positional_unpacking(
        self,
    ) -> None:
        """*args may supply module_name positionally."""
        sample = 'local_path_override(*["apple_support"], path = "../x")'
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_unpacking_after_positional_module_name(
        self,
    ) -> None:
        """module_name given positionally can't also come from **kwargs."""
        sample = 'local_path_override("rules_cc", **{"path": "../x"})'
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_positional_string_in_list(
        self,
    ) -> None:
        """A bare string that is an *element* of a list value (not the

        call's own first positional argument) must not be read as
        module_name.
        """
        sample = """archive_override(
    module_name = "rules_cc",
    urls = ["apple_support"],
)"""
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_reordered_args(self) -> None:
        sample = """
single_version_override(
    patches = ["//:patches/apple_support_macos_min_11.patch"],
    version = "1.24.2",
    module_name = "apple_support",
)
"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_commented_out_call(self) -> None:
        sample = (
            '# archive_override(module_name = "apple_support", '
            'strip_prefix = "x")\n'
        )
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_other_modules(self) -> None:
        sample = """
single_version_override(
    module_name = "rules_cc",
    version = "0.2.26",
)
"""
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_fake_assignment_in_string(
        self,
    ) -> None:
        """Assignment-shaped text inside an unrelated string value for a

        different module's override must not be read as real args.
        """
        sample = """single_version_override(
    module_name = "rules_cc",
    version = "0.2.26",
    patch_cmds = ['the module_name = "apple_support" override is legacy'],
)"""
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_nested_override_call(
        self,
    ) -> None:
        """A recognized call nested inside another call's argument value

        (e.g. patches = single_version_override(...) or []) is still a
        real override Bazel executes, not inert text; it must not be
        silently swallowed while the outer call's own argument list is
        paren-balanced.
        """
        sample = (
            'single_version_override(module_name = "rules_cc", '
            'patches = single_version_override(module_name = "apple_support", '
            'version = "1.24.2") or [])'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_spaced_attribute_call(
        self,
    ) -> None:
        """"ext. archive_override(...)" is an attribute call, not the

        archive_override directive, whatever separates the "." from the
        name.
        """
        for qualifier, close in (("ext. ", ""), ("ext.\\\n", ""), ("[ext. # note\n", "]")):
            sample = (
                f'{qualifier}archive_override(module_name = "apple_support", '
                f'urls = ["x"]){close}'
            )
            with self.subTest(qualifier=qualifier):
                self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_archive_override(self) -> None:
        sample = """
archive_override(
    module_name = "apple_support",
    urls = ["https://example.com/apple_support.tar.gz"],
)
"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_parenthesized_callable_indirection(
        self,
    ) -> None:
        """Grouping parens around any expression are legal Starlark, and

        the result can be called immediately: (archive_override)(...) is
        a valid call equivalent to archive_override(...), but doesn't
        match a bare "name(" call textually.
        """
        sample = (
            '(archive_override)(module_name = "apple_support", '
            'urls = ["https://example.com/apple_support.tar.gz"])'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_rejects_longer_identifier_in_parens(
        self,
    ) -> None:
        """"(archive_override_foo)(...)" must not be misread as a

        parenthesized reference to archive_override.
        """
        sample = (
            '(archive_override_foo)(module_name = "apple_support", '
            'urls = ["x"])'
        )
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_git_override(self) -> None:
        sample = """
git_override(
    module_name = "apple_support",
    remote = "https://github.com/bazelbuild/apple_support",
    commit = "abc123",
)
"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_local_path_override(self) -> None:
        sample = """
local_path_override(
    module_name = "apple_support",
    path = "../apple_support",
)
"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_multiple_version_override(
        self,
    ) -> None:
        sample = """
multiple_version_override(
    module_name = "apple_support",
    versions = ["1.24.2", "2.10.1"],
)
"""
        self.assertTrue(_has_apple_support_override(sample))


if __name__ == "__main__":
    unittest.main()
