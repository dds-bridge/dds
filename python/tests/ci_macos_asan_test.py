#!/usr/bin/env python3
"""Guard that apple_support stays past the release that fixed Xcode 27 ASAN
crosstool warnings, without re-adding the patches that release made obsolete.
"""

from __future__ import annotations

import re
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


_IDENT_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def _skip_ws(text: str, i: int, n: int) -> int:
    while i < n and text[i] in " \t\r\n":
        i += 1
    return i


def _skip_comment(text: str, i: int, n: int) -> int:
    while i < n and text[i] != "\n":
        i += 1
    return i


def _skip_string(text: str, i: int, n: int) -> tuple[int, str | None]:
    """Return (end_index, value) for the string literal starting at text[i].

    Handles Starlark triple-quoted strings as a single opaque unit (value
    is None — never needed as a captured kwarg value here, and critically,
    none of its content — including a quote character that would otherwise
    look like the end of a single-quoted string — is scanned character by
    character for a fake nested call). Ordinary quoted strings are handled
    with backslash escapes (value is the unescaped content).
    """
    quote = text[i]
    if text[i : i + 3] == quote * 3:
        delim = quote * 3
        j = i + 3
        while j < n:
            if text[j] == "\\":
                j += 2
                continue
            if text[j : j + 3] == delim:
                return j + 3, None
            j += 1
        return n, None
    j = i + 1
    chars: list[str] = []
    while j < n:
        if text[j] == "\\" and j + 1 < n:
            chars.append(text[j + 1])
            j += 2
            continue
        if text[j] == quote:
            return j + 1, "".join(chars)
        chars.append(text[j])
        j += 1
    return n, "".join(chars)


def _is_identifier_boundary(text: str, i: int) -> bool:
    """True unless the character before i could continue an identifier.

    Rejects matching a call/keyword name in the middle of a longer
    identifier (e.g. "legacy_bazel_dep(" or "bazel_depfoo(") and rejects a
    qualified/attribute call (e.g. "extensions.bazel_dep(") by also
    treating a preceding "." as non-boundary.
    """
    if i == 0:
        return True
    prev = text[i - 1]
    return not (prev.isalnum() or prev == "_" or prev == ".")


def _iter_call_kwargs(call_names: tuple[str, ...], text: str) -> list[dict[str, str]]:
    """Yield the {keyword: string value} args of each top-level call.

    A single linear scan recognizes a call name, and each "identifier =
    value" pair inside its body, only at a genuine identifier boundary and
    only outside strings/comments. This matters beyond just truncation: a
    plain regex search over body *text* (even with comments stripped)
    still matches an unrelated argument's *string value* that happens to
    contain assignment-shaped text, e.g.
    `patch_cmds = ['historically name = "apple_support", ...']` — a
    bazel_dep for a different module would be misread as the apple_support
    one. Parsing real "identifier = value" pairs only at the call's
    top-level parenthesis depth, with identifier-matching paused for every
    character inside a string or comment (string/comment skipping is a
    single jump via _skip_string/_skip_comment, not a per-character toggle,
    so a triple-quoted string's content — including embedded quote
    characters — is never scanned as code), makes that impossible: nothing
    inside a string literal or comment is ever offered to the identifier
    matcher. Only simple `identifier = "string"` / `identifier = 'string'`
    assignments are captured (list/nested-call values are skipped over for
    paren balancing but not parsed), which is everything every caller here
    needs. Whitespace between a call name and "(" is tolerated
    ("archive_override (..." is valid Starlark).
    """
    sorted_names = sorted(call_names, key=len, reverse=True)
    results: list[dict[str, str]] = []
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        if ch == "#":
            i = _skip_comment(text, i, n)
            continue
        if ch in "\"'":
            i, _ = _skip_string(text, i, n)
            continue
        matched_name = None
        paren_index = None
        if _is_identifier_boundary(text, i):
            for name in sorted_names:
                if text.startswith(name, i):
                    j = _skip_ws(text, i + len(name), n)
                    if j < n and text[j] == "(":
                        matched_name = name
                        paren_index = j
                        break
        if matched_name is None:
            i += 1
            continue
        i = paren_index + 1
        depth = 1
        kwargs: dict[str, str] = {}
        while i < n and depth > 0:
            ch = text[i]
            if ch == "#":
                i = _skip_comment(text, i, n)
                continue
            if ch in "\"'":
                i, _ = _skip_string(text, i, n)
                continue
            if ch == "(":
                depth += 1
                i += 1
                continue
            if ch == ")":
                depth -= 1
                i += 1
                continue
            if depth == 1 and _is_identifier_boundary(text, i):
                m = _IDENT_RE.match(text, i)
                if m:
                    ident = m.group(0)
                    j = _skip_ws(text, m.end(), n)
                    if j < n and text[j] == "=" and text[j : j + 2] != "==":
                        j = _skip_ws(text, j + 1, n)
                        if j < n and text[j] in "\"'":
                            end, value = _skip_string(text, j, n)
                            if value is not None:
                                kwargs[ident] = value
                            i = end
                            continue
                    i = m.end()
                    continue
            i += 1
        results.append(kwargs)
    return results


def _apple_support_bazel_dep_version(module_bazel: str) -> str:
    for kwargs in _iter_call_kwargs(("bazel_dep",), module_bazel):
        if kwargs.get("name") == "apple_support" and "version" in kwargs:
            return kwargs["version"]
    raise AssertionError('expected bazel_dep(... name = "apple_support" ...)')


def _has_apple_support_override(module_bazel: str) -> bool:
    """True if any Bazel module override directive targets apple_support.

    Covers all five override directives (archive_override, git_override,
    local_path_override, multiple_version_override, single_version_override)
    — not just single_version_override — and matches each call's keyword
    args regardless of argument order or formatting, rather than assuming
    module_name is the first line after the opening "(".
    """
    override_names = (
        "archive_override",
        "git_override",
        "local_path_override",
        "multiple_version_override",
        "single_version_override",
    )
    for kwargs in _iter_call_kwargs(override_names, module_bazel):
        if kwargs.get("module_name") == "apple_support":
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

    def test_apple_support_bazel_dep_version_survives_an_earlier_paren(
        self,
    ) -> None:
        """A ")" inside a leading comment must not truncate the call body."""
        sample = """bazel_dep(
    # fixes Xcode 27 crosstool warnings (see docs)
    version = "2.10.1",
    name = "apple_support",
)"""
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_apple_support_bazel_dep_version_ignores_commented_out_call(
        self,
    ) -> None:
        sample = """# bazel_dep(name = "apple_support", version = "9.9.9")
bazel_dep(name = "apple_support", version = "2.10.1")
"""
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_apple_support_bazel_dep_version_ignores_commented_out_version(
        self,
    ) -> None:
        """A stale version in a comment inside the call must not win."""
        sample = """bazel_dep(
    name = "apple_support",
    # version = "9.9.9" (stale)
    version = "2.10.1",
)"""
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

    def test_has_apple_support_override_detects_same_line_args(self) -> None:
        sample = (
            'single_version_override(module_name = "apple_support", version = "1.24.2")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_reordered_args(self) -> None:
        sample = """
single_version_override(
    patches = ["//:patches/apple_support_macos_min_11.patch"],
    version = "1.24.2",
    module_name = "apple_support",
)
"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_survives_an_earlier_paren(self) -> None:
        """A ")" inside a leading comment must not truncate the call body."""
        sample = """single_version_override(
    # pin past the Xcode 27 fixes (see docs)
    module_name = "apple_support",
    version = "1.24.2",
)"""
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

    def test_has_apple_support_override_ignores_commented_module_name(
        self,
    ) -> None:
        """A comment mentioning apple_support in a rules_cc override is not

        itself an apple_support override.
        """
        sample = """single_version_override(
    # module_name = "apple_support" (old pin, no longer used)
    module_name = "rules_cc",
    version = "0.2.26",
)"""
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

    def test_has_apple_support_override_tolerates_whitespace_before_paren(
        self,
    ) -> None:
        sample = 'archive_override (module_name = "apple_support", strip_prefix = "x")'
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_archive_override(self) -> None:
        sample = """
archive_override(
    module_name = "apple_support",
    urls = ["https://example.com/apple_support.tar.gz"],
)
"""
        self.assertTrue(_has_apple_support_override(sample))

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
