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
_POSITIONAL_KEY = "__positional__"
# Sentinel for a module_name/name argument whose value is a variable
# reference or a string concatenation rather than a single literal, so the
# true value can't be read back from the source text. Treated as a possible
# match by every caller here (fail closed) rather than silently absent,
# since the latter would let a real override on the guarded module slip
# through whenever its target is computed instead of written as a literal.
_UNRESOLVED = object()


def _add_positional(kwargs: dict, value: object) -> None:
    """Record a newly-seen top-level positional argument, in call order.

    Bazel's APIs here accept more than one leading parameter
    positionally — not just the first (e.g. bazel_dep's name *and*
    version) — so every caller needs all of them, by index, not just
    whichever arrived first. Appending (instead of the previous "only
    the first" guard) keeps them in the order they appeared.
    """
    kwargs.setdefault(_POSITIONAL_KEY, []).append(value)


def _positional(kwargs: dict, index: int) -> object | None:
    """Return the positional argument at index, or None if fewer were given."""
    values = kwargs.get(_POSITIONAL_KEY, [])
    return values[index] if index < len(values) else None


def _skip_ws(text: str, i: int, n: int) -> int:
    """Skip whitespace, including a backslash-newline line continuation.

    A backslash immediately followed by an LF or CRLF line break is a
    line continuation -- dropped entirely, the same rule _decode_escape()
    applies inside a string -- so it is whitespace here too; skipping
    only the bare characters in " \t\r\n" left the backslash itself
    unconsumed, so e.g. "archive_override\\\n(" (the call name and "("
    split across such a continuation) was never recognized as a call.
    """
    while i < n:
        if text[i] in " \t\r\n":
            i += 1
            continue
        if text[i] == "\\" and text[i + 1 : i + 2] == "\n":
            i += 2
            continue
        if text[i] == "\\" and text[i + 1 : i + 3] == "\r\n":
            i += 3
            continue
        break
    return i


def _skip_comment(text: str, i: int, n: int) -> int:
    while i < n and text[i] != "\n":
        i += 1
    return i


_SIMPLE_ESCAPES = {
    "\\": "\\",
    "'": "'",
    '"': '"',
    "a": "\a",
    "b": "\b",
    "f": "\f",
    "n": "\n",
    "r": "\r",
    "t": "\t",
    "v": "\v",
}
_HEX_DIGITS = "0123456789abcdefABCDEF"
_OCTAL_DIGITS = "01234567"


def _decode_escape(text: str, j: int, n: int) -> tuple[int, str]:
    """Decode one backslash escape starting at text[j] == "\\".

    Returns (new_j, decoded_text) for Starlark's string-literal escapes:
    the simple single-character escapes (\\n, \\t, \\\\, ...), up to 3
    octal digits, exactly two hex digits after \\x, the \\uXXXX /
    \\UXXXXXXXX Unicode code point forms (exactly 4 / 8 hex digits), and
    a line continuation — a backslash immediately followed by an actual
    LF or CRLF line break in the source, which Starlark drops entirely
    (unlike "\\n", the two-character escape for a newline *value*).
    Discarding the backslash and keeping the next character literally —
    the previous behavior here — decodes "\\x61pple_support" (a valid
    Starlark escape for "apple_support") to the wrong string
    "x61pple_support", letting an escaped module_name bypass the override
    guard; "\\u0061pple_support" was the same bug for Unicode escapes, and
    a module_name literal split across two source lines by a trailing
    backslash was the same bug for a line continuation. An escape this
    doesn't recognize is left as a literal backslash + next character.
    """
    if j + 1 >= n:
        return j + 1, "\\"
    c = text[j + 1]
    if c == "\r" and j + 2 < n and text[j + 2] == "\n":
        return j + 3, ""
    if c == "\n":
        return j + 2, ""
    if c in _SIMPLE_ESCAPES:
        return j + 2, _SIMPLE_ESCAPES[c]
    if c == "x":
        hex_digits = text[j + 2 : j + 4]
        if len(hex_digits) == 2 and all(d in _HEX_DIGITS for d in hex_digits):
            return j + 4, chr(int(hex_digits, 16))
        return j + 2, "x"
    if c in ("u", "U"):
        width = 4 if c == "u" else 8
        hex_digits = text[j + 2 : j + 2 + width]
        if len(hex_digits) == width and all(d in _HEX_DIGITS for d in hex_digits):
            return j + 2 + width, chr(int(hex_digits, 16))
        return j + 2, c
    if c in _OCTAL_DIGITS:
        k = j + 1
        digits = ""
        while k < n and len(digits) < 3 and text[k] in _OCTAL_DIGITS:
            digits += text[k]
            k += 1
        return k, chr(int(digits, 8))
    return j + 2, "\\" + c


def _skip_string(text: str, i: int, n: int) -> tuple[int, str]:
    """Return (end_index, value) for the string literal starting at text[i].

    Handles Starlark triple-quoted strings as a single opaque unit — the
    closing delimiter is matched as the 3-character sequence, never a
    single quote character, so none of the content in between (including
    an embedded quote that would otherwise look like the end of an
    ordinary string) is ever scanned character by character for a fake
    nested call — while still extracting its value just like an ordinary
    quoted string, so a (valid, if unusual) triple-quoted keyword value is
    captured rather than silently dropped. Both forms decode backslash
    escapes via _decode_escape() (value is the unescaped content).
    """
    quote = text[i]
    if text[i : i + 3] == quote * 3:
        delim = quote * 3
        j = i + 3
        chars: list[str] = []
        while j < n:
            if text[j] == "\\" and j + 1 < n:
                j, decoded = _decode_escape(text, j, n)
                chars.append(decoded)
                continue
            if text[j : j + 3] == delim:
                return j + 3, "".join(chars)
            chars.append(text[j])
            j += 1
        return n, "".join(chars)
    j = i + 1
    chars = []
    while j < n:
        if text[j] == "\\" and j + 1 < n:
            j, decoded = _decode_escape(text, j, n)
            chars.append(decoded)
            continue
        if text[j] == quote:
            return j + 1, "".join(chars)
        chars.append(text[j])
        j += 1
    return n, "".join(chars)


def _is_raw_string_start(text: str, i: int, n: int) -> bool:
    """True if text[i] starts a raw string literal: "r"/"R" + quote."""
    return i + 1 < n and text[i] in "rR" and text[i + 1] in "\"'"


def _skip_raw_string(text: str, i: int, n: int) -> tuple[int, str]:
    """Return (end_index, value) for the raw string starting at text[i].

    text[i] is the delimiter quote itself (the caller has already
    consumed the "r"/"R" prefix). Per the Starlark spec (and matching
    real Python raw-string behavior, which it mirrors here), a raw
    string gives a backslash exactly one special power: letting a
    literal quote appear without ending the string. Whether a quote
    that follows a run of backslashes is that kind of escaped quote
    depends on the run's *parity* — r"foo\\" (an even, here zero-after-
    pairing, run) ends normally with two literal backslashes in the
    value, while r"foo\"" (an odd run) does not end there; the quote is
    part of the value, and — unlike an ordinary string's
    _decode_escape() — the backslash stays in the value too, exactly as
    written. Handles a triple-quote delimiter the same way _skip_string()
    does — matched as the 3-character sequence once any escaped quote
    immediately before it has been accounted for the same way.
    """
    quote = text[i]
    if text[i : i + 3] == quote * 3:
        delim = quote * 3
        j = i + 3
        chars: list[str] = []
        while j < n:
            if text[j] == "\\":
                k = j
                while k < n and text[k] == "\\":
                    k += 1
                run = text[j:k]
                if k < n and text[k] == quote and len(run) % 2 == 1:
                    chars.append(run)
                    chars.append(quote)
                    j = k + 1
                    continue
                chars.append(run)
                j = k
                continue
            if text[j : j + 3] == delim:
                return j + 3, "".join(chars)
            chars.append(text[j])
            j += 1
        return n, "".join(chars)
    j = i + 1
    chars = []
    while j < n:
        if text[j] == "\\":
            k = j
            while k < n and text[k] == "\\":
                k += 1
            run = text[j:k]
            if k < n and text[k] == quote and len(run) % 2 == 1:
                chars.append(run)
                chars.append(quote)
                j = k + 1
                continue
            chars.append(run)
            j = k
            continue
        if text[j] == quote:
            return j + 1, "".join(chars)
        chars.append(text[j])
        j += 1
    return n, "".join(chars)


def _skip_any_string(text: str, i: int, n: int) -> tuple[int, str]:
    """Return (end_index, value) for the string literal starting at text[i],

    recognizing an optional "r"/"R" raw-string prefix before dispatching
    to _skip_raw_string() or _skip_string(). A caller that needs to
    recognize whether a token *is* a string literal (as opposed to
    skipping over one already known to start exactly at i) uses this
    instead of _skip_string() directly, so a raw literal like
    r"rules_cc" is never misread as a bare identifier reference — which
    fails closed on this guard just like any other unresolved value,
    wrongly blocking a completely unrelated module's override.
    """
    if _is_raw_string_start(text, i, n):
        return _skip_raw_string(text, i + 1, n)
    return _skip_string(text, i, n)


def _skip_ws_and_comments(text: str, i: int, n: int) -> int:
    """Skip whitespace and "#" comments, repeatedly, starting at i.

    A single _skip_ws() stops at the first "#"; this loops so a comment
    followed by more whitespace (its trailing newline, a blank line, or
    another comment) is all consumed, landing on the next real token
    regardless of how it's laid out across lines.
    """
    j = i
    while True:
        skipped = _skip_ws(text, j, n)
        if skipped < n and text[skipped] == "#":
            skipped = _skip_comment(text, skipped, n)
        if skipped == j:
            break
        j = skipped
    return j


def _is_complete_literal(text: str, i: int, n: int) -> bool:
    """True if the string literal ending at i is a whole call argument.

    Skips whitespace and comments after the literal. The literal is the
    argument's complete, resolved value only if what follows is ","
    (ending this argument) or ")" (ending the call) — anything else,
    including "+" (concatenation, even across an intervening comment),
    "[" (slicing), "%" (string formatting), "." (a method call), or any
    other operator, means the literal is merely a fragment of a larger
    expression whose true value can't be read back from the source text.
    """
    j = _skip_ws_and_comments(text, i, n)
    return j < n and text[j] in ",)"


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


def _match_parenthesized_call(
    text: str, i: int, n: int, sorted_names: list[str]
) -> tuple[str | None, int]:
    """Match one or more layers of "(" name ")" grouping, starting at
    text[i] == "(", followed by the real call's "(".

    Grouping parens around any expression are legal Starlark and nest
    arbitrarily, and the result can still be called immediately, so
    "((archive_override))(...)" is just as valid a call as the single-
    layer "(archive_override)(...)" — but neither matches a bare
    "name(" call textually, letting this scanner's direct matching be
    bypassed by wrapping the name in one or more layers of parens.
    Comments are tolerated around every token here too, the same as
    Starlark itself ignores them: "(archive_override # note\n)(...)" is
    just as valid. Returns (matched_name, call_paren_index);
    matched_name is None if this position isn't that pattern.
    """
    depth = 0
    j = i
    while j < n and text[j] == "(":
        depth += 1
        j = _skip_ws_and_comments(text, j + 1, n)
    if depth == 0 or not _is_identifier_boundary(text, j):
        return None, 0
    for name in sorted_names:
        if text.startswith(name, j):
            k = _skip_ws_and_comments(text, j + len(name), n)
            closed = 0
            while closed < depth and k < n and text[k] == ")":
                closed += 1
                k = _skip_ws_and_comments(text, k + 1, n)
            if closed == depth and k < n and text[k] == "(":
                return name, k
    return None, 0


def _match_call_name(
    text: str, i: int, n: int, sorted_names: list[str]
) -> tuple[str | None, int]:
    """Try to match a call "name(...)" at text[i], directly or through

    one or more layers of grouping parens (see _match_parenthesized_call).
    Whitespace and comments between the name and its "(" are tolerated
    either way. Returns (matched_name, index_of_opening_paren), or
    (None, 0) if text[i] isn't the start of a recognized call.
    """
    if _is_identifier_boundary(text, i):
        for name in sorted_names:
            if text.startswith(name, i):
                j = _skip_ws_and_comments(text, i + len(name), n)
                if j < n and text[j] == "(":
                    return name, j
    if i < n and text[i] == "(":
        return _match_parenthesized_call(text, i, n, sorted_names)
    return None, 0


def _scan_call_body(
    text: str,
    i: int,
    n: int,
    sorted_names: list[str],
    results: list[dict[str, str]],
) -> int:
    """Scan one call's body, starting right after its opening "(".

    Parses each "identifier = value" pair at the call's own top-level
    parenthesis depth (see _iter_call_kwargs for why only there), then
    appends the resulting {keyword: value} dict to `results` and returns
    the index just past the matching closing ")".

    A recognized call name appearing anywhere in the body -- not just as
    this call's own name -- is itself a real call Bazel executes, e.g.
    `patches = single_version_override(...) or []`; such a nested call
    is recursively scanned the same way (appending its own dict to
    `results` too) and its matched span is skipped whole, rather than
    being silently swallowed as inert text while this call's argument
    list is paren-balanced.
    """
    depth = 1
    bracket_depth = 0
    brace_depth = 0
    kwargs: dict[str, str] = {}
    while i < n and depth > 0:
        ch = text[i]
        if ch == "#":
            i = _skip_comment(text, i, n)
            continue
        at_top = depth == 1 and bracket_depth == 0 and brace_depth == 0
        if ch in "\"'" or _is_raw_string_start(text, i, n):
            if at_top:
                end, value = _skip_any_string(text, i, n)
                _add_positional(
                    kwargs,
                    value if _is_complete_literal(text, end, n) else _UNRESOLVED,
                )
                i = end
                continue
            i, _ = _skip_any_string(text, i, n)
            continue
        nested_name, nested_paren = _match_call_name(text, i, n, sorted_names)
        if nested_name is not None:
            if at_top:
                _add_positional(kwargs, _UNRESOLVED)
            i = _scan_call_body(text, nested_paren + 1, n, sorted_names, results)
            continue
        if ch == "(":
            # A "(" at the top level, not preceded by "ident =" and not
            # a recognized call name (handled above), opens a
            # parenthesized positional expression (e.g. an argument
            # written as ("a" + "b")) rather than a literal or a bare
            # variable reference; mark it unresolved before descending
            # into it, the same as any other non-literal positional
            # value.
            if at_top:
                _add_positional(kwargs, _UNRESOLVED)
            depth += 1
            i += 1
            continue
        if ch == ")":
            depth -= 1
            i += 1
            continue
        if ch == "[":
            # Same reasoning as the "(" branch above: a top-level "["
            # not preceded by "ident =" opens a list expression (e.g.
            # an argument written as ["a"][0]).
            if at_top:
                _add_positional(kwargs, _UNRESOLVED)
            bracket_depth += 1
            i += 1
            continue
        if ch == "]":
            bracket_depth -= 1
            i += 1
            continue
        if ch == "{":
            # Same reasoning again, for a dict/set expression opening
            # a positional argument.
            if at_top:
                _add_positional(kwargs, _UNRESOLVED)
            brace_depth += 1
            i += 1
            continue
        if ch == "}":
            brace_depth -= 1
            i += 1
            continue
        if at_top and _is_identifier_boundary(text, i):
            m = _IDENT_RE.match(text, i)
            if m:
                ident = m.group(0)
                j = _skip_ws_and_comments(text, m.end(), n)
                if j < n and text[j] == "=" and text[j : j + 2] != "==":
                    rhs_start = _skip_ws_and_comments(text, j + 1, n)
                    if rhs_start < n and (
                        text[rhs_start] in "\"'"
                        or _is_raw_string_start(text, rhs_start, n)
                    ):
                        end, value = _skip_any_string(text, rhs_start, n)
                        kwargs[ident] = (
                            value
                            if _is_complete_literal(text, end, n)
                            else _UNRESOLVED
                        )
                        i = end
                        continue
                    # A non-literal right-hand side (a variable
                    # reference, a function call, ...) can't be read
                    # back from the source text. Fail closed: mark it
                    # unresolved rather than leaving it absent, which
                    # would read identically to the argument never
                    # having been given at all.
                    kwargs[ident] = _UNRESOLVED
                    i = rhs_start
                    continue
                # Not "ident = ...": a bare identifier is a variable
                # used as a value, passed positionally; fail closed
                # the same way as a non-literal keyword value, rather
                # than silently dropping it.
                _add_positional(kwargs, _UNRESOLVED)
                i = m.end()
                continue
        i += 1
    results.append(kwargs)
    return i


def _iter_call_kwargs(call_names: tuple[str, ...], text: str) -> list[dict[str, str]]:
    """Yield the {keyword: string value} args of each call, top-level or

    nested in another call's argument value (see _scan_call_body).

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
    assignments are captured (list values are skipped over for paren
    balancing but not parsed, aside from a nested recognized call).
    Whitespace between a call name and "(" is tolerated ("archive_override
    (..." is valid Starlark).

    Bazel's `*_override`/`bazel_dep` APIs accept their leading arguments
    (module_name / name, and for bazel_dep also version) positionally —
    every bare top-level value before any "identifier =" is appended, in
    order, to the list under `_POSITIONAL_KEY` (see `_add_positional`),
    letting callers read any of them by index (`_positional`), not just
    the first. "Top-level" also tracks `[...]`/`{...}` nesting (not just
    parens), so a string that's an *element* of a list value (e.g.
    inside `urls = [...]`) is never mistaken for a positional call
    argument.
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
        if ch in "\"'" or _is_raw_string_start(text, i, n):
            i, _ = _skip_any_string(text, i, n)
            continue
        matched_name, paren_index = _match_call_name(text, i, n, sorted_names)
        if matched_name is None:
            i += 1
            continue
        i = _scan_call_body(text, paren_index + 1, n, sorted_names, results)
    return results


def _apple_support_bazel_dep_version(module_bazel: str) -> str:
    """Return the pinned apple_support bazel_dep version.

    bazel_dep's first two parameters, name and version, are both
    allowed positionally (bazel_dep("apple_support", "2.10.1")), not
    just name; _positional(kwargs, 1) reads version there the same way
    _positional(kwargs, 0) already reads name, rather than assuming
    version is only ever given as a keyword.
    """
    for kwargs in _iter_call_kwargs(("bazel_dep",), module_bazel):
        name = kwargs.get("name")
        if name is None:
            name = _positional(kwargs, 0)
        if name == "apple_support":
            version = kwargs.get("version")
            if version is None:
                version = _positional(kwargs, 1)
            if version is not None and version is not _UNRESOLVED:
                return version
    raise AssertionError('expected bazel_dep(... name = "apple_support" ...)')


def _has_apple_support_override(module_bazel: str) -> bool:
    """True if any Bazel module override directive targets apple_support.

    Covers all five override directives (archive_override, git_override,
    local_path_override, multiple_version_override, single_version_override)
    — not just single_version_override — and matches each call's keyword
    or first-positional module_name argument regardless of argument order
    or formatting, rather than assuming module_name is the first line
    after the opening "(". Fails closed (returns True) for an override
    whose module_name/name is a variable reference or a string
    concatenation — _UNRESOLVED — since the real text that will resolve
    to at runtime can't be read back from the source, and treating that
    the same as "not apple_support" would let a real override on a
    computed target slip past this guard undetected.
    """
    override_names = (
        "archive_override",
        "git_override",
        "local_path_override",
        "multiple_version_override",
        "single_version_override",
    )
    for kwargs in _iter_call_kwargs(override_names, module_bazel):
        module_name = kwargs.get("module_name")
        if module_name is None:
            module_name = _positional(kwargs, 0)
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

    def test_apple_support_bazel_dep_version_accepts_raw_triple_quoted_name(
        self,
    ) -> None:
        """name = r\"\"\"apple_support\"\"\" is a valid Starlark raw

        triple-quoted literal; it must resolve to the complete literal
        rather than stopping at the first lone quote.
        """
        sample = 'bazel_dep(name = r"""apple_support""", version = "2.10.1")'
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

    def test_apple_support_bazel_dep_version_skips_comment_after_equals(
        self,
    ) -> None:
        """A comment between "=" and the literal on the next line is valid

        Starlark formatting; it must be skipped, not mistaken for a
        non-literal RHS that leaves the version unresolved.
        """
        sample = """bazel_dep(
    name = "apple_support",
    version =
    # pinned past the Xcode 27 fixes
    "2.10.1",
)"""
        self.assertEqual(_apple_support_bazel_dep_version(sample), "2.10.1")

    def test_apple_support_bazel_dep_version_skips_comment_between_keyword_and_equals(
        self,
    ) -> None:
        """A comment between the keyword and "=" is valid Starlark

        formatting (same as between "=" and the value); it must be
        skipped so the keyword is still recognized as "name", not
        misread as a bare positional identifier that leaves the real
        name argument unresolved.
        """
        sample = """bazel_dep(
    name
    # pinned past the Xcode 27 fixes
    = "apple_support",
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

    def test_has_apple_support_override_detects_triple_quoted_module_name(
        self,
    ) -> None:
        """A triple-quoted keyword value is valid Starlark and must still

        be extracted, not silently treated as absent.
        """
        sample = (
            'single_version_override(module_name = """apple_support""", '
            'version = "1.24.2")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_decodes_hex_escape_in_module_name(
        self,
    ) -> None:
        """\\x61 is a valid Starlark hex escape for "a"; the decoded value

        must still be compared, not the raw escape text.
        """
        sample = (
            'single_version_override(module_name = "\\x61pple_support", '
            'version = "1.24.2")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_decodes_unicode_escape_in_module_name(
        self,
    ) -> None:
        """\\u0061 is a valid Starlark 4-hex-digit Unicode escape for "a";

        the decoded value must still be compared, not the raw escape text.
        """
        sample = (
            'single_version_override(module_name = "\\u0061pple_support", '
            'version = "1.24.2")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_decodes_big_unicode_escape_in_module_name(
        self,
    ) -> None:
        """\\U00000061 is a valid Starlark 8-hex-digit Unicode escape for

        "a"; the decoded value must still be compared, not the raw escape
        text.
        """
        sample = (
            'single_version_override(module_name = "\\U00000061pple_support", '
            'version = "1.24.2")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_decodes_line_continuation_in_module_name(
        self,
    ) -> None:
        """A backslash immediately followed by an actual newline in the

        source is a line continuation that Starlark drops entirely, not
        the two-character "\\n" escape for a newline value; the module
        name spans two source lines but still resolves to one literal.
        """
        sample = (
            'archive_override(module_name = "apple_\\\n'
            'support", strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_decodes_crlf_line_continuation_in_module_name(
        self,
    ) -> None:
        """Same as the LF case, but for a CRLF line ending."""
        sample = (
            'archive_override(module_name = "apple_\\\r\n'
            'support", strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_keyword_variable_module_name(
        self,
    ) -> None:
        """module_name set to a variable reference can't be read back from

        the source text; the guard must fail closed (treat it as a
        possible apple_support override) rather than silently treating
        the override as targeting some other module.
        """
        sample = 'archive_override(module_name = target, strip_prefix = "x")'
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_positional_variable_module_name(
        self,
    ) -> None:
        """Same as the keyword case, but module_name is passed positionally

        as a variable reference.
        """
        sample = 'archive_override(target, strip_prefix = "x")'
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_parenthesized_positional_module_name(
        self,
    ) -> None:
        """A parenthesized expression passed as the first positional

        argument (e.g. ("apple_" + "support")) can't be read back as a
        literal from the source text either; it must fail closed the
        same as any other non-literal positional value, not be silently
        skipped while descending into the parens.
        """
        sample = 'archive_override(("apple_" + "support"), strip_prefix = "x")'
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_list_positional_module_name(
        self,
    ) -> None:
        """A list expression passed as the first positional argument (e.g.

        ["apple_support"][0]) can't be read back as a literal from the
        source text either; it must fail closed, not be silently skipped
        while descending into the list.
        """
        sample = 'archive_override(["apple_support"][0], strip_prefix = "x")'
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_dict_positional_module_name(
        self,
    ) -> None:
        """Same as the list case, but for a dict expression opening the

        first positional argument.
        """
        sample = (
            'archive_override({"apple_support": 1}.keys()[0], '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_concatenated_module_name(
        self,
    ) -> None:
        """module_name assembled via string concatenation has no single

        literal value in the source text; only the first fragment
        ("apple_") would be captured by a naive literal-only parser, which
        must not be mistaken for the whole, real value.
        """
        sample = (
            'archive_override(module_name = "apple_" + "support", '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_concat_split_by_comment(
        self,
    ) -> None:
        """A comment between the first fragment and the "+" must not hide

        the concatenation: the literal still isn't the whole argument.
        """
        sample = """archive_override(
    module_name = "apple_"  # fixes a typo below
    + "support",
    strip_prefix = "x",
)"""
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_sliced_module_name(
        self,
    ) -> None:
        """A slice expression is a literal followed by "[...]", not a

        complete argument by itself; the literal alone must not be
        mistaken for the slice's actual result.
        """
        sample = (
            'archive_override(module_name = "xapple_support"[1:], '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_fails_closed_on_percent_formatted_module_name(
        self,
    ) -> None:
        """A "%"-formatted literal is a template, not the resolved value;

        "apple_%s" % "support" resolves to "apple_support" but the
        literal "apple_%s" alone must not be mistaken for it.
        """
        sample = (
            'archive_override(module_name = "apple_%s" % "support", '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

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

    def test_has_apple_support_override_ignores_raw_string_for_other_module(
        self,
    ) -> None:
        """r"rules_cc" is a valid Starlark raw string literal; it must be

        recognized as the complete, resolved literal "rules_cc" rather
        than misread as a non-literal (bare identifier "r" + a separate
        quoted string), which would fail closed and incorrectly block a
        valid override on a completely unrelated module.
        """
        sample = 'archive_override(module_name = r"rules_cc", strip_prefix = "x")'
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_raw_string_module_name(
        self,
    ) -> None:
        """A raw string literal is still a real override when it does

        name apple_support.
        """
        sample = (
            'archive_override(module_name = r"apple_support", '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_raw_string_positional_module_name(
        self,
    ) -> None:
        """A raw string literal is recognized in the first-positional form

        too, not just as a keyword value.
        """
        sample = 'archive_override(r"apple_support", strip_prefix = "x")'
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_raw_triple_quoted_for_other_module(
        self,
    ) -> None:
        """r\"\"\"rules_cc\"\"\" is a valid Starlark raw triple-quoted literal;

        it must resolve to the complete literal "rules_cc" rather than
        stopping at the first lone quote (which would leave the value
        unresolved and fail closed, incorrectly blocking a valid
        override on a completely unrelated module).
        """
        sample = (
            'archive_override(module_name = r"""rules_cc""", '
            'strip_prefix = "x")'
        )
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_raw_triple_quoted_module_name(
        self,
    ) -> None:
        """A raw triple-quoted literal is still a real override when it

        does name apple_support.
        """
        sample = (
            'archive_override(module_name = r"""apple_support""", '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_decodes_escaped_quote_in_raw_string(
        self,
    ) -> None:
        """Starlark raw strings still give backslash-quote special meaning

        (a literal quote, string continues) even though every other
        backslash is kept completely literal; r"apple_\\"support" (an
        embedded quote) is a different string from "apple_support" and
        must not be mistaken for an apple_support override.
        """
        sample = (
            'archive_override(module_name = r"apple_\\"support", '
            'strip_prefix = "x")'
        )
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_ends_raw_string_at_even_backslash_run(
        self,
    ) -> None:
        """An even run of backslashes immediately before the closing quote

        (here two) pairs up and does not escape it; the raw string ends
        normally there, with both backslashes kept literally in the
        value, so the argument after it (module_name) is parsed
        correctly rather than being swallowed into a runaway string.
        """
        sample = (
            'archive_override(strip_prefix = r"apple_support\\\\", '
            'module_name = "rules_cc")'
        )
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_continues_raw_string_at_odd_backslash_run(
        self,
    ) -> None:
        """An odd run of backslashes immediately before a quote (here

        three) still escapes it, same as a single backslash; the string
        does not end there and both the backslashes and the quote stay
        literally in the value.
        """
        sample = (
            'archive_override(module_name = r"apple_support\\\\\\"x", '
            'strip_prefix = "x")'
        )
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_raw_string_outside_recognized_call(
        self,
    ) -> None:
        """A raw string literal outside any recognized call must be

        skipped using the raw-aware dispatcher, the same as inside a
        call body; otherwise its contents are fed to the ordinary
        escape decoder, which raises on sequences that are only valid
        because the "r" prefix makes them literal (e.g. "\\U12345678"
        is not a valid 8-hex-digit Unicode escape, and "\\e" is not a
        recognized simple escape either), spuriously failing a guard
        that doesn't even apply to this unrelated statement.
        """
        sample = (
            'SDK_PATH = r"C:\\U12345678\\emsdk"\n'
            'archive_override(module_name = "rules_cc", strip_prefix = "x")'
        )
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_skips_comment_between_equals_and_value(
        self,
    ) -> None:
        """A comment between "=" and the literal on the next line is valid

        Starlark formatting; it must be skipped, not mistaken for a
        non-literal RHS that falsely trips the guard for an override on
        some other module.
        """
        sample = """archive_override(
    module_name =
    # pin past Xcode 27 fixes
    "rules_cc",
    strip_prefix = "x",
)"""
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_skips_comment_between_keyword_and_equals(
        self,
    ) -> None:
        """A comment between the keyword and "=" is valid Starlark

        formatting; it must be skipped so "module_name" is still
        recognized as that keyword, not misread as a bare positional
        identifier -- which would fail closed and falsely trip the
        guard even though the real module_name names a different,
        unrelated module.
        """
        sample = """local_path_override(
    module_name
    # note
    = "emsdk",
    path = "../emsdk",
)"""
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

    def test_has_apple_support_override_detects_call_after_lf_continuation(
        self,
    ) -> None:
        """A backslash-LF line continuation between the call name and "("

        is valid Starlark, dropped entirely the same as inside a string;
        it must not leave the call unrecognized.
        """
        sample = (
            'archive_override\\\n(module_name = "apple_support", '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_call_after_crlf_continuation(
        self,
    ) -> None:
        """Same as the LF case, for a backslash-CRLF line continuation."""
        sample = (
            'archive_override\\\r\n(module_name = "apple_support", '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_comment_before_own_paren(
        self,
    ) -> None:
        """A comment between the callable name and its own "(" is legal

        inside a parenthesized expression statement, e.g.
        "(local_path_override # note\\n(module_name = ..., ...))"; the
        direct call-name match must skip comments, not just whitespace,
        there too.
        """
        sample = (
            "(local_path_override # note\n"
            '(module_name = "apple_support", path = "../apple_support"))'
        )
        self.assertTrue(_has_apple_support_override(sample))

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

    def test_has_apple_support_override_detects_parenthesized_call_with_comment(
        self,
    ) -> None:
        """Starlark ignores comments; a comment between the grouped

        callable name and the closing paren -- (archive_override #
        note\\n)(...) -- must not hide this call from the same
        detection that handles the plain "(name)(" form.
        """
        sample = (
            '(archive_override # note\n'
            ')(module_name = "apple_support", strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_doubly_parenthesized_call(
        self,
    ) -> None:
        """Grouping parens nest arbitrarily in Starlark; two layers --

        ((archive_override))(...) -- are just as valid a call as one,
        and must not bypass the same detection that handles the
        single-layer "(name)(" form.
        """
        sample = (
            '((archive_override))(module_name = "apple_support", '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_detects_triply_parenthesized_call(
        self,
    ) -> None:
        """Same as the double-layer case, but with three layers."""
        sample = (
            '(((archive_override)))(module_name = "apple_support", '
            'strip_prefix = "x")'
        )
        self.assertTrue(_has_apple_support_override(sample))

    def test_has_apple_support_override_ignores_doubly_parenthesized_other_module(
        self,
    ) -> None:
        sample = (
            '((archive_override))(module_name = "rules_cc", '
            'strip_prefix = "x")'
        )
        self.assertFalse(_has_apple_support_override(sample))

    def test_has_apple_support_override_rejects_longer_identifier_in_nested_parens(
        self,
    ) -> None:
        """"((archive_override_foo))(...)" must not be misread as a

        doubly-parenthesized reference to archive_override.
        """
        sample = (
            '((archive_override_foo))(module_name = "apple_support", '
            'strip_prefix = "x")'
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
