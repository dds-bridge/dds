#!/usr/bin/env python3
"""Benchmark dtest performance across one or more binaries.

Runs all combinations of solver (solve, calc) and hand file
(list100/1000/…/1), largest files first. Always prints a summary. Per-run
timing rows and build (git/bazel) output are shown transiently, then hidden
unless the --details flag is set. Passes dtest options if given after "--".

Usage:
  python/tests/benchmark.py
  python/tests/benchmark.py --build
  python/tests/benchmark.py -- -n 8 -r
  python/tests/benchmark.py --build --binary /path/to/other/dtest
  python/tests/benchmark.py --branch develop -- -n 8
  python/tests/benchmark.py --wasm_branch develop -- -n 2
  python/tests/benchmark.py --branch develop --wasm_branch develop
  python/tests/benchmark.py --binary /path/to/other/dtest --epsilon 1
  python/tests/benchmark.py --repeats 5 -- -n 4
  REPEATS=3 python/tests/benchmark.py
  ./benchmark.sh   # equivalent wrapper at repo root

Environment:
  BRANCH     Path to the baseline dtest (default: bazel-bin under the current dir)
  BINARY     Optional extra dtest binary to benchmark (like a trailing --binary)
  HANDS_DIR  Directory containing list*.txt files (default: ./hands)
  REPEATS    Runs per combination per binary (default: 1)
  MAX_DEALS  Include list10^n.txt files with 10^n <= N (default: 100)
  DRY_RUN    If 1, print commands only
  DETAILS    If 1, keep per-run rows and build output (default: 0, summary only)
  SYS_USER   If 1, include a sys/user column per binary in the summary
  EPSILON    For a two-binary comparison, max % diff treated as equal (default: 0.5)
"""

from __future__ import annotations

import atexit
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Mapping, Sequence, TextIO

SOLVERS = ("solve", "calc")
ALT_ENTER = "\033[?1049h\033[H\033[2J"
ALT_LEAVE = "\033[?1049l"
GIT_SPEC_KINDS = frozenset({"branch", "wasm_branch"})


def dtest_rel(*, os_name: str = os.name) -> Path:
    """Relative path to Bazel's dtest binary for the given OS."""
    name = "dtest.exe" if os_name == "nt" else "dtest"
    return Path(f"bazel-bin/library/tests/{name}")


DTEST_REL = dtest_rel()
DTEST_WASM_JS_REL = Path("bazel-bin/wasm/dtest.js")
DTEST_WASM_WASM_REL = Path("bazel-bin/wasm/dtest.wasm")


def resolve_bazel_command(*, which: Callable[[str], str | None] | None = None) -> str:
    """Prefer bazelisk; fall back to bazel for older local installs."""
    finder = which or shutil.which
    if finder("bazelisk"):
        return "bazelisk"
    return "bazel"


class BenchmarkError(Exception):
    """User-facing configuration or validation error."""


@dataclass(frozen=True)
class DtestTiming:
    user_ms: float | None
    sys_ms: float | None
    avg_user: float | None
    sys_user: float | None
    hands: int | None = None


@dataclass(frozen=True)
class ResultRow:
    solver: str
    file: str
    bin_idx: int
    rep: int
    user_ms: float | None
    sys_ms: float | None
    avg_user: float | None
    sys_user: float | None
    hands: int | None = None


@dataclass
class Config:
    repeats: int = 1
    max_deals: int = 100
    epsilon: float = 0.5
    dry_run: bool = False
    details: bool = False
    sys_user: bool = False
    build: bool = False
    reverse: bool = False
    branch_binary: Path | None = None  # BRANCH env / default dtest path
    hands_dir: Path | None = None
    specs: list[tuple[str, str]] = field(default_factory=list)
    dtest_extra: list[str] = field(default_factory=list)


def is_power_of_10(n: int) -> bool:
    if n < 1:
        return False
    while n > 1:
        if n % 10 != 0:
            return False
        n //= 10
    return True


def is_dds_root(root: Path) -> bool:
    return (root / "MODULE.bazel").is_file() and (
        root / "library" / "tests" / "BUILD.bazel"
    ).is_file()


def label_for_path(path: str | Path) -> str:
    name = Path(path).name
    return name if name else str(path)


def run_order(num_bins: int, *, reverse: bool) -> list[int]:
    order = list(range(num_bins))
    if reverse:
        order.reverse()
    return order


def within_epsilon(a: float, b: float, eps_pct: float) -> bool:
    e = eps_pct / 100.0
    hi, lo = (a, b) if a > b else (b, a)
    return hi <= 0 or (hi - lo) / hi <= e


def select_hand_files(hands_dir: Path, max_deals: int) -> list[str]:
    candidates: list[tuple[int, str]] = []
    for path in hands_dir.glob("list*.txt"):
        m = re.fullmatch(r"list([0-9]+)\.txt", path.name)
        if not m:
            continue
        count = int(m.group(1))
        if is_power_of_10(count) and count <= max_deals:
            candidates.append((count, path.name))
    if not candidates:
        raise BenchmarkError(
            f"no list10^n.txt files with 10^n <= {max_deals} in {hands_dir}"
        )
    candidates.sort(key=lambda t: t[0], reverse=True)
    return [name for _, name in candidates]


def deals_from_hand_file(name: str) -> int | None:
    """Return deal count encoded in listN.txt, or None if not that pattern."""
    m = re.fullmatch(r"list([0-9]+)\.txt", name)
    if not m:
        return None
    return int(m.group(1))


def _parse_ms(token: str) -> float | None:
    if token == "zero":
        return 0.0
    try:
        return float(token)
    except ValueError:
        return None


def parse_dtest_output(text: str) -> DtestTiming:
    hands_f: float | None = None
    user: float | None = None
    sys_ms: float | None = None
    avg: float | None = None
    sys_user: float | None = None
    for line in text.splitlines():
        if line.startswith("Number of hands"):
            hands_f = _parse_ms(line.split()[-1])
        elif line.startswith("User time (ms)"):
            user = _parse_ms(line.split()[-1])
        elif line.startswith("Sys time (ms)"):
            sys_ms = _parse_ms(line.split()[-1])
        elif line.startswith("Avg user time (ms)"):
            avg = _parse_ms(line.split()[-1])
        elif line.startswith("Ratio"):
            # Match awk: /^Ratio[[:space:]]/
            if len(line) > 5 and line[5].isspace():
                sys_user = _parse_ms(line.split()[-1])
    hands: int | None = None
    if hands_f is not None and hands_f >= 0 and hands_f == int(hands_f):
        hands = int(hands_f)
    if avg is None:
        if user == 0:
            avg = 0.0
        elif hands is not None and user is not None and hands > 0:
            avg = user / hands
    return DtestTiming(
        user_ms=user, sys_ms=sys_ms, avg_user=avg, sys_user=sys_user, hands=hands
    )


def dtest_timing_usable(parsed: DtestTiming) -> bool:
    """True when output has the user timing the summary needs.

    Requires avg_user (ms/deal) as well as user_ms. Sys time may be n/a on
    platforms without a process CPU clock (e.g. wasm32).
    """
    return parsed.user_ms is not None and parsed.avg_user is not None


def _fmt_timing(v: float | None) -> str:
    if v is None:
        return "NA"
    if v == int(v):
        return str(int(v))
    return str(v)


def format_run_table_header(run_label_col: str) -> tuple[str, str]:
    """Return (header, separator) for the per-run timing table."""
    header = (
        f"{'solver':<6} {'file':<13} {run_label_col:<12} "
        f"{'user_ms':>8} {'sys_ms':>8} {'avg_user':>10} {'sys/user':>8} run"
    )
    sep = (
        f"{'------':<6} {'-------------':<13} {'------------':<12} "
        f"{'--------':>8} {'--------':>8} {'----------':>10} {'--------':>8} ---"
    )
    return header, sep


def format_run_table_row(
    solver: str,
    file: str,
    lab: str,
    user: str,
    sys_ms: str,
    avg: str,
    sys_user: str,
    run_label: str,
) -> str:
    return (
        f"{solver:<6} {file:<13} {lab:<12} "
        f"{user:>8} {sys_ms:>8} {avg:>10} {sys_user:>8} {run_label}"
    )


def format_summary(
    rows: Sequence[ResultRow],
    *,
    labels: Sequence[str],
    files: Sequence[str],
    epsilon: float,
    sys_user: bool = False,
) -> str:
    nb = len(labels)
    sums: dict[tuple[str, str, int], float] = {}
    counts: dict[tuple[str, str, int], int] = {}
    su_sums: dict[tuple[str, str, int], float] = {}
    su_counts: dict[tuple[str, str, int], int] = {}
    # Per-solver totals for overall avg user ms = sum(user_ms) / sum(deals).
    total_user: dict[str, list[float]] = {s: [0.0] * nb for s in SOLVERS}
    total_deals: dict[str, list[int]] = {s: [0] * nb for s in SOLVERS}
    user_seen: dict[str, list[bool]] = {s: [False] * nb for s in SOLVERS}

    for row in rows:
        key = (row.solver, row.file, row.bin_idx)
        if row.avg_user is not None:
            sums[key] = sums.get(key, 0.0) + row.avg_user
            counts[key] = counts.get(key, 0) + 1
        if row.sys_user is not None:
            su_sums[key] = su_sums.get(key, 0.0) + row.sys_user
            su_counts[key] = su_counts.get(key, 0) + 1
        deals = (
            row.hands
            if row.hands is not None and row.hands > 0
            else deals_from_hand_file(row.file)
        )
        if (
            row.user_ms is not None
            and row.avg_user is not None
            and deals is not None
            and deals > 0
            and row.solver in total_user
        ):
            total_user[row.solver][row.bin_idx] += row.user_ms
            total_deals[row.solver][row.bin_idx] += deals
            user_seen[row.solver][row.bin_idx] = True

    def L(b: int) -> str:
        return labels[b][:12]

    def Lf(b: int) -> str:
        return labels[b]

    def append_sys_user_header(s: str) -> str:
        if sys_user:
            s += f" {'sys/user':>8}"
        return s

    def append_sys_user_dash(s: str) -> str:
        if sys_user:
            s += f" {'--------':>8}"
        return s

    def append_sys_user_blank(s: str) -> str:
        if sys_user:
            s += f" {'':>8}"
        return s

    def append_sys_user_value(s: str, v: float | None) -> str:
        if not sys_user:
            return s
        if v is None:
            return s + f" {'NA':>8}"
        return s + f" {v:8.2f}"

    lines: list[str] = []
    header = f"{'solver':<6} {'file':<13}"
    for b in range(nb):
        header += f" {L(b):>12}"
        header = append_sys_user_header(header)
    if nb == 2:
        header += f" {'rel':>10} {'note':<15}"
    lines.append(header)

    def dash() -> None:
        d = f"{'------':<6} {'-------------':<13}"
        for _ in range(nb):
            d += f" {'------------':>12}"
            d = append_sys_user_dash(d)
        if nb == 2:
            d += f" {'----------':>10} {'---------------':<15}"
        lines.append(d)

    dash()

    for solver in SOLVERS:
        for fname in files:
            avgs: list[float | None] = []
            su_avgs: list[float | None] = []
            for b in range(nb):
                key = (solver, fname, b)
                if key in counts:
                    avgs.append(sums[key] / counts[key])
                else:
                    avgs.append(None)
                if key in su_counts:
                    su_avgs.append(su_sums[key] / su_counts[key])
                else:
                    su_avgs.append(None)
            if all(u is None for u in avgs):
                continue
            line = f"{solver:<6} {fname:<13}"
            for b, u in enumerate(avgs):
                if u is None:
                    line += f" {'NA':>12}"
                else:
                    line += f" {u:12.2f}"
                line = append_sys_user_value(line, su_avgs[b])
            if nb == 2:
                a0, a1 = avgs[0], avgs[1]
                if a0 is not None and a1 is not None and a0 > 0:
                    # If a0 ever becomes zero, we should switch dtest timing from
                    # milliseconds to microseconds.
                    r = a1 / a0
                    if within_epsilon(a0, a1, epsilon):
                        note = "equal"
                    elif r >= 1:
                        note = f"{Lf(0)} faster"
                    else:
                        note = f"{Lf(1)} faster"
                    line += f" {r:9.2f}x {note:<15}"
                else:
                    line += f" {'':>10} {'':<15}"
            lines.append(line)

    dash()
    for solver in SOLVERS:
        if not any(user_seen[solver]):
            continue
        tot = f"{'TOTAL':<6} {solver:<13}"
        allpos = True
        users = total_user[solver]
        deals = total_deals[solver]
        seen = user_seen[solver]
        avgs: list[float | None] = []
        for b in range(nb):
            if seen[b] and deals[b] > 0:
                avg: float | None = users[b] / deals[b]
            else:
                avg = None
            avgs.append(avg)
            if avg is None:
                tot += f" {'NA':>12}"
                allpos = False
            else:
                tot += f" {avg:12.2f}"
                if not (avg > 0):
                    allpos = False
            tot = append_sys_user_blank(tot)
        if nb == 2:
            if allpos and avgs[0] is not None and avgs[1] is not None:
                r = avgs[1] / avgs[0]
                if within_epsilon(avgs[0], avgs[1], epsilon):
                    tnote = "equal"
                elif r >= 1:
                    tnote = f"{Lf(0)} faster"
                else:
                    tnote = f"{Lf(1)} faster"
                tot += f" {r:9.2f}x {tnote:<15}"
            else:
                tot += f" {'':>10} {'':<15}"
        lines.append(tot)
    return "\n".join(lines)


def _env_truthy(env: Mapping[str, str], key: str) -> bool:
    return env.get(key, "0") == "1"


def _parse_positive_int(value: str, name: str) -> int:
    if not re.fullmatch(r"[0-9]+", value) or int(value) < 1:
        raise BenchmarkError(f"{name} must be a positive integer (got: {value})")
    return int(value)


def _parse_nonneg_float(value: str, name: str) -> float:
    if not re.fullmatch(r"[0-9]+(\.[0-9]+)?", value):
        raise BenchmarkError(f"{name} must be a non-negative number (got: {value})")
    return float(value)


def parse_args(argv: Sequence[str], env: Mapping[str, str] | None = None) -> Config:
    env = dict(os.environ if env is None else env)
    cfg = Config(
        repeats=_parse_positive_int(env.get("REPEATS", "1"), "repeats"),
        max_deals=_parse_positive_int(env.get("MAX_DEALS", "100"), "max_deals"),
        epsilon=_parse_nonneg_float(env.get("EPSILON", "0.5"), "epsilon"),
        dry_run=_env_truthy(env, "DRY_RUN"),
        details=_env_truthy(env, "DETAILS"),
        sys_user=_env_truthy(env, "SYS_USER"),
    )
    if env.get("HANDS_DIR"):
        cfg.hands_dir = Path(env["HANDS_DIR"])
    if env.get("BRANCH"):
        cfg.branch_binary = Path(env["BRANCH"])

    repeats_given = False
    cli_binary_given = False
    args = list(argv)
    i = 0
    while i < len(args):
        a = args[i]
        if a in ("-h", "--help"):
            print(usage_text())
            raise SystemExit(0)
        if a == "--repeats":
            if repeats_given:
                raise BenchmarkError("--repeats may be given only once")
            repeats_given = True
            i += 1
            if i >= len(args):
                raise BenchmarkError("missing value for --repeats")
            cfg.repeats = _parse_positive_int(args[i], "repeats")
        elif a == "--branch":
            i += 1
            if i >= len(args):
                raise BenchmarkError("missing value for --branch")
            val = args[i]
            if val.startswith("-"):
                raise BenchmarkError(f"--branch ref must not start with '-': {val}")
            cfg.specs.append(("branch", val))
        elif a in ("--wasm_branch", "--wasm-branch"):
            i += 1
            if i >= len(args):
                raise BenchmarkError(f"missing value for {a}")
            val = args[i]
            if val.startswith("-"):
                raise BenchmarkError(f"{a} ref must not start with '-': {val}")
            cfg.specs.append(("wasm_branch", val))
        elif a == "--binary":
            i += 1
            if i >= len(args):
                raise BenchmarkError("missing value for --binary")
            cfg.specs.append(("binary", args[i]))
            cli_binary_given = True
        elif a in ("--max-deals", "--max_deals", "-max-deals", "-max_deals"):
            i += 1
            if i >= len(args):
                raise BenchmarkError("missing value for --max-deals")
            cfg.max_deals = _parse_positive_int(args[i], "max_deals")
        elif a == "--build":
            cfg.build = True
        elif a == "--reverse":
            cfg.reverse = True
        elif a == "--details":
            cfg.details = True
        elif a == "--sys-user":
            cfg.sys_user = True
        elif a == "--epsilon":
            i += 1
            if i >= len(args):
                raise BenchmarkError("missing value for --epsilon")
            cfg.epsilon = _parse_nonneg_float(args[i], "epsilon")
        elif a == "--":
            cfg.dtest_extra = args[i + 1 :]
            break
        else:
            raise BenchmarkError(f"Unknown option: {a}\n{usage_text()}")
        i += 1

    if not cli_binary_given and env.get("BINARY"):
        cfg.specs.append(("binary", env["BINARY"]))

    if cfg.reverse and len(cfg.specs) < 2:
        raise BenchmarkError(
            "--reverse requires at least two binaries "
            "(two or more --branch/--wasm_branch/--binary)"
        )
    return cfg


def usage_text() -> str:
    return """\
Usage: benchmark.py [OPTIONS]

Benchmark dtest across solver/file combinations. Always prints a summary; use
--details for per-run rows and build (git/bazel) output.

Options:
  -h, --help          Show this help
  --repeats N         Runs per combination per binary (default: 1; env: REPEATS)
  --max-deals N       Include list10^n.txt files with 10^n <= N (default: 100; env: MAX_DEALS)
                      (alias: --max_deals)
  --build             Build dtest for the current checkout (bazelisk build //library/tests:dtest)
  --branch NAME       Git branch to build and benchmark ("." means the current branch).
                      Repeatable. Each named branch is checked out, dtest is built and
                      its binary saved, then the original branch is restored. A clean
                      tree is required only when a ref would switch away from HEAD.
  --wasm_branch NAME  Like --branch, but builds //wasm:dtest_wasm and runs it under Node.
                      Alias: --wasm-branch. Labels appear as wasm:NAME.
  --binary PATH       Path to a prebuilt dtest binary to benchmark. Repeatable.
                      A .js path (dtest_wasm) is run via node with the sibling .wasm.
  --details           Keep per-run timing rows and build (git/bazel) output
  --sys-user          Include a sys/user column per binary in the summary
                      (env: SYS_USER=1)
  --epsilon PCT       For a two-binary comparison, treat timings within PCT% as equal
                      (default: 0.5; env: EPSILON)
  --reverse           Reverse the per-repeat dispatch order of the binaries
  --                  End benchmark options; remaining args are passed to dtest
                      (e.g. -- -n 8 -r for 8 threads and slow-board report)

--branch, --wasm_branch, and --binary may be given any number of times and
combined; the binaries are benchmarked in the order specified and the first is
the baseline. --binary must not point at the checkout's bazel-bin dtest (or
wasm dtest.js) when the matching --branch/--wasm_branch is also used (those
builds overwrite that path). The current checkout is benchmarked by default
only when none of those flags is given. With two binaries the summary adds a
rel and a "faster" note; with three or more it shows only the per-binary
averages (no note).

Environment:
  BRANCH, BINARY, HANDS_DIR, REPEATS, MAX_DEALS, DRY_RUN, DETAILS, SYS_USER, EPSILON

Examples:
  python/tests/benchmark.py
  python/tests/benchmark.py --build
  python/tests/benchmark.py -- -n 8
  python/tests/benchmark.py --repeats 3 -- -n 4 -r
  python/tests/benchmark.py --branch develop
  python/tests/benchmark.py --branch develop --branch opus-two-percent
  python/tests/benchmark.py --branch develop --branch opus-two-percent --branch fastest
  python/tests/benchmark.py --branch opus-two-percent --binary /path/to/dtest
  python/tests/benchmark.py --branch develop --repeats 3 -- -n 8
  python/tests/benchmark.py --wasm_branch develop
  python/tests/benchmark.py --branch develop --wasm_branch develop
  python/tests/benchmark.py --binary /path/to/dtest
  python/tests/benchmark.py --binary /path/to/dtest --details
  python/tests/benchmark.py --binary /path/to/dtest --sys-user
  python/tests/benchmark.py --binary /path/to/dtest --epsilon 1
  python/tests/benchmark.py --binary /path/to/dtest --reverse
  python/tests/benchmark.py --repeats 5 --binary /path/to/dtest
  DRY_RUN=1 python/tests/benchmark.py
  ./benchmark.sh"""


def _git(root: Path, *args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", "-C", str(root), *args],
        check=check,
        text=True,
        capture_output=True,
    )


def _git_out(root: Path, *args: str) -> str:
    return _git(root, *args).stdout.strip()


def reject_checkout_binary_with_branch(
    root: Path, specs: Sequence[tuple[str, str]]
) -> None:
    """Disallow --binary pointing at checkout artifacts overwritten by branch builds.

    --branch overwrites root/DTEST_REL; --wasm_branch overwrites the wasm dtest.js
    (and sibling .wasm). An explicit --binary at those paths would run the wrong
    artifact after the original branch is restored.
    """
    has_native = any(kind == "branch" for kind, _ in specs)
    has_wasm = any(kind == "wasm_branch" for kind, _ in specs)
    if not has_native and not has_wasm:
        return
    checkout_dtest = (root / DTEST_REL).resolve()
    checkout_wasm_js = (root / DTEST_WASM_JS_REL).resolve()
    for kind, val in specs:
        if kind != "binary":
            continue
        resolved = Path(val).resolve()
        if has_native and resolved == checkout_dtest:
            raise BenchmarkError(
                f"--binary may not target the checkout's {DTEST_REL} when "
                "--branch is used (branch builds overwrite that path); "
                "copy the binary elsewhere or use --branch . for HEAD"
            )
        if has_wasm and resolved == checkout_wasm_js:
            raise BenchmarkError(
                f"--binary may not target the checkout's {DTEST_WASM_JS_REL} when "
                "--wasm_branch is used (wasm branch builds overwrite that path); "
                "copy the artifacts elsewhere or use --wasm_branch . for HEAD"
            )


def git_prep_for_branches(
    root: Path, specs: list[tuple[str, str]]
) -> tuple[list[tuple[str, str]], str]:
    """Validate git state and resolve '.' branch shorthand.

    Applies to both --branch and --wasm_branch specs.
    Returns (resolved_specs, orig_branch).
    """
    try:
        probe = _git(root, "rev-parse", "--is-inside-work-tree", check=False)
    except OSError as e:
        raise BenchmarkError(
            "--branch/--wasm_branch requires git to be installed and on PATH"
        ) from e
    if probe.returncode != 0:
        raise BenchmarkError(
            f"--branch/--wasm_branch requires a git work tree at {root}"
        )
    sym = _git(root, "symbolic-ref", "--quiet", "--short", "HEAD", check=False)
    if sym.returncode == 0 and sym.stdout.strip():
        orig_branch = sym.stdout.strip()
    else:
        orig_branch = _git_out(root, "rev-parse", "HEAD")

    resolved: list[tuple[str, str]] = []
    for kind, val in specs:
        if kind in GIT_SPEC_KINDS and val == ".":
            resolved.append((kind, orig_branch))
        else:
            resolved.append((kind, val))

    for kind, val in resolved:
        if kind not in GIT_SPEC_KINDS:
            continue
        ok = _git(root, "rev-parse", "--verify", "--quiet", f"{val}^{{commit}}", check=False)
        if ok.returncode != 0:
            flag = "--wasm_branch" if kind == "wasm_branch" else "--branch"
            raise BenchmarkError(f"{flag}: unknown git ref '{val}'")

    head_commit = _git_out(root, "rev-parse", "HEAD")
    needs_switch = False
    for kind, val in resolved:
        if kind not in GIT_SPEC_KINDS:
            continue
        ref_commit = _git_out(root, "rev-parse", "--verify", "--quiet", f"{val}^{{commit}}")
        if ref_commit != head_commit:
            needs_switch = True
            break

    if needs_switch:
        status = _git_out(root, "status", "--porcelain", "--untracked-files=normal")
        if status:
            raise BenchmarkError(
                "working tree not clean; commit, stash, or remove changes "
                "(tracked or untracked) before using --branch/--wasm_branch "
                "with a different commit"
            )
    return resolved, orig_branch


class BenchmarkRunner:
    def __init__(
        self,
        root: Path,
        cfg: Config,
        *,
        err: TextIO = sys.stderr,
        out: TextIO = sys.stdout,
    ) -> None:
        self.root = root
        self.cfg = cfg
        self.err = err
        self.out = out
        self.tmp_bins: list[Path] = []
        self.tmp_dirs: list[Path] = []
        self.build_log: Path | None = None
        self.orig_branch: str | None = None
        self.alt_screen_active = False
        self.git_branch = "unknown"
        atexit.register(self.cleanup)

    def cleanup(self) -> None:
        if self.alt_screen_active:
            try:
                self.out.write(ALT_LEAVE)
                self.out.flush()
            except OSError:
                pass
            self.alt_screen_active = False
        if self.orig_branch:
            cur = ""
            try:
                sym = _git(
                    self.root,
                    "symbolic-ref",
                    "--quiet",
                    "--short",
                    "HEAD",
                    check=False,
                )
                cur = (
                    sym.stdout.strip()
                    if sym.returncode == 0
                    else _git_out(self.root, "rev-parse", "HEAD")
                )
            except (subprocess.CalledProcessError, OSError):
                cur = ""
            if cur and cur != self.orig_branch:
                print(f"Restoring git branch '{self.orig_branch}'...", file=self.err)
                _git(self.root, "checkout", self.orig_branch, check=False)
        for p in self.tmp_bins:
            try:
                p.unlink(missing_ok=True)
            except OSError:
                pass
        self.tmp_bins.clear()
        for d in self.tmp_dirs:
            try:
                shutil.rmtree(d, ignore_errors=True)
            except OSError:
                pass
        self.tmp_dirs.clear()
        if self.build_log is not None:
            try:
                self.build_log.unlink(missing_ok=True)
            except OSError:
                pass
            self.build_log = None

    def current_label(self) -> str:
        if self.git_branch and self.git_branch != "unknown":
            return self.git_branch
        return "branch"

    def run_build(self, cmd: Sequence[str], *, cwd: Path | None = None) -> None:
        if self.cfg.details:
            subprocess.run(cmd, cwd=cwd or self.root, check=True)
            return
        if self.build_log is None:
            fd, name = tempfile.mkstemp(prefix="dds-dtest-build.")
            os.close(fd)
            self.build_log = Path(name)
        with open(self.build_log, "w", encoding="utf-8") as log:
            proc = subprocess.run(
                cmd,
                cwd=cwd or self.root,
                stdout=log,
                stderr=subprocess.STDOUT,
                check=False,
            )
        if proc.returncode != 0:
            self.err.write(self.build_log.read_text(encoding="utf-8", errors="replace"))
            raise subprocess.CalledProcessError(proc.returncode, cmd)

    def bazel_dtest(self) -> None:
        self.run_build(
            [resolve_bazel_command(), "build", "//library/tests:dtest"],
            cwd=self.root,
        )

    def bazel_dtest_wasm(self) -> None:
        self.run_build(
            [resolve_bazel_command(), "build", "//wasm:dtest_wasm"],
            cwd=self.root,
        )

    def checkout_and_build(self, name: str) -> None:
        self.run_build(["git", "-C", str(self.root), "checkout", name])
        self.bazel_dtest()

    def build_branch_binary(self, name: str, dest: Path) -> None:
        if self.cfg.dry_run:
            bazel = resolve_bazel_command()
            print(f"DRY_RUN: git -C {self.root} checkout {name}", file=self.err)
            print(
                f"DRY_RUN: (cd {self.root} && {bazel} build //library/tests:dtest)",
                file=self.err,
            )
            print(f"DRY_RUN: cp -L {self.root / DTEST_REL} {dest}", file=self.err)
            return
        print(f"Building dtest from '{name}'...", file=self.err)
        self.checkout_and_build(name)
        src = self.root / DTEST_REL
        shutil.copy2(src, dest, follow_symlinks=True)
        dest.chmod(dest.stat().st_mode | 0o111)

    def build_wasm_branch(self, name: str, dest_dir: Path) -> Path:
        """Checkout, build dtest_wasm, copy js+wasm into dest_dir; return js path."""
        dest_js = dest_dir / "dtest.js"
        dest_wasm = dest_dir / "dtest.wasm"
        if self.cfg.dry_run:
            bazel = resolve_bazel_command()
            print(f"DRY_RUN: git -C {self.root} checkout {name}", file=self.err)
            print(
                f"DRY_RUN: (cd {self.root} && {bazel} build //wasm:dtest_wasm)",
                file=self.err,
            )
            print(
                f"DRY_RUN: cp -L {self.root / DTEST_WASM_JS_REL} {dest_js}",
                file=self.err,
            )
            print(
                f"DRY_RUN: cp -L {self.root / DTEST_WASM_WASM_REL} {dest_wasm}",
                file=self.err,
            )
            return dest_js
        print(f"Building dtest_wasm from '{name}'...", file=self.err)
        self.run_build(["git", "-C", str(self.root), "checkout", name])
        self.bazel_dtest_wasm()
        shutil.copy2(self.root / DTEST_WASM_JS_REL, dest_js, follow_symlinks=True)
        shutil.copy2(self.root / DTEST_WASM_WASM_REL, dest_wasm, follow_symlinks=True)
        return dest_js

    def new_tmp_bin(self) -> Path:
        fd, name = tempfile.mkstemp(prefix="dds-dtest-bin.", suffix=".exe" if os.name == "nt" else "")
        os.close(fd)
        path = Path(name)
        self.tmp_bins.append(path)
        return path

    def new_tmp_wasm_dir(self) -> Path:
        path = Path(tempfile.mkdtemp(prefix="dds-dtest-wasm."))
        self.tmp_dirs.append(path)
        return path

    def restore_branch(self, rebuild: bool) -> None:
        assert self.orig_branch is not None
        if self.cfg.dry_run:
            print(f"DRY_RUN: git -C {self.root} checkout {self.orig_branch}", file=self.err)
            if rebuild:
                bazel = resolve_bazel_command()
                print(
                    f"DRY_RUN: (cd {self.root} && {bazel} build //library/tests:dtest)",
                    file=self.err,
                )
            return
        if rebuild:
            print(f"Restoring '{self.orig_branch}' and rebuilding...", file=self.err)
            self.checkout_and_build(self.orig_branch)
        else:
            print(f"Restoring '{self.orig_branch}'...", file=self.err)
            self.run_build(["git", "-C", str(self.root), "checkout", self.orig_branch])

    def build_binaries(self) -> tuple[list[str], list[Path]]:
        specs = list(self.cfg.specs)
        nspecs = len(specs)
        ngit = sum(1 for k, _ in specs if k in GIT_SPEC_KINDS)

        if nspecs == 0:
            assert self.cfg.branch_binary is not None
            return [self.current_label()], [self.cfg.branch_binary]

        reject_checkout_binary_with_branch(self.root, specs)

        if ngit > 0:
            specs, self.orig_branch = git_prep_for_branches(self.root, specs)

        labels: list[str] = []
        paths: list[Path] = []
        for kind, val in specs:
            if kind == "branch":
                t = self.new_tmp_bin()
                self.build_branch_binary(val, t)
                paths.append(t)
                labels.append(val)
            elif kind == "wasm_branch":
                d = self.new_tmp_wasm_dir()
                js = self.build_wasm_branch(val, d)
                paths.append(js)
                labels.append(f"wasm:{val}")
            else:
                paths.append(Path(val))
                labels.append(label_for_path(val))

        if ngit > 0:
            self.restore_branch(False)
        return labels, paths

    def dtest_command(self, binary: Path, solver: str, hands: Path) -> list[str]:
        # Absolute -f / script so wasm runs (cwd = js parent) still resolve paths.
        hands_arg = str(hands.resolve())
        args = ["-f", hands_arg, "-s", solver, *self.cfg.dtest_extra]
        if binary.suffix == ".js":
            return ["node", str(binary.resolve()), *args]
        return [str(binary), *args]

    def run_dtest(self, binary: Path, solver: str, hands: Path) -> DtestTiming:
        cmd = self.dtest_command(binary, solver, hands)
        if self.cfg.dry_run:
            print(f"DRY_RUN: {' '.join(cmd)}", file=self.err)
            return DtestTiming(None, None, None, None)

        cwd = binary.resolve().parent if binary.suffix == ".js" else None
        proc = subprocess.run(
            cmd, capture_output=True, text=True, check=False, cwd=cwd
        )
        out = proc.stdout + proc.stderr
        if proc.returncode != 0:
            print(f"error: dtest failed: {' '.join(cmd)}", file=self.err)
            print(out, file=self.err)
            raise SystemExit(1)
        parsed = parse_dtest_output(out)
        if not dtest_timing_usable(parsed):
            print(f"warning: incomplete dtest timing output: {' '.join(cmd)}", file=self.err)
        return parsed

    def detect_git_branch(self) -> None:
        probe = _git(self.root, "rev-parse", "--is-inside-work-tree", check=False)
        if probe.returncode != 0:
            self.git_branch = "unknown"
            return
        abbrev = _git(self.root, "rev-parse", "--abbrev-ref", "HEAD", check=False)
        self.git_branch = abbrev.stdout.strip() if abbrev.returncode == 0 else "unknown"


def main(argv: Sequence[str] | None = None, env: Mapping[str, str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    env_map = dict(os.environ if env is None else env)

    root = Path.cwd()
    if not is_dds_root(root):
        print(f"error: '{root}' is not a DDS checkout root", file=sys.stderr)
        print(
            "       cd to the root of the dds repository before running benchmark.py",
            file=sys.stderr,
        )
        return 1

    try:
        cfg = parse_args(argv, env=env_map)
    except BenchmarkError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    if cfg.hands_dir is None:
        cfg.hands_dir = root / "hands"
    if cfg.branch_binary is None:
        cfg.branch_binary = root / DTEST_REL

    runner = BenchmarkRunner(root, cfg)
    runner.detect_git_branch()

    try:
        labels, paths = runner.build_binaries()
    except BenchmarkError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError:
        return 1

    num_bins = len(paths)
    nspecs = len(cfg.specs)
    ngit = sum(1 for k, _ in cfg.specs if k in GIT_SPEC_KINDS)
    # After git_prep, specs may have changed on the runner via build_binaries;
    # recompute ngit from original cfg (git-spec count is stable).
    if nspecs == 0 or ngit == nspecs:
        run_label_col = "branch"
    elif ngit == 0:
        run_label_col = "binary"
    else:
        run_label_col = "label"
    if ngit > 0:
        cfg.build = False

    try:
        files = select_hand_files(cfg.hands_dir, cfg.max_deals)
    except BenchmarkError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    if cfg.build:
        if cfg.dry_run:
            bazel = resolve_bazel_command()
            print(
                f"DRY_RUN: (cd {root} && {bazel} build //library/tests:dtest)",
                file=sys.stderr,
            )
        else:
            print("Building //library/tests:dtest...", file=sys.stderr)
            try:
                runner.bazel_dtest()
            except subprocess.CalledProcessError:
                return 1

    if not cfg.dry_run:
        for i, p in enumerate(paths):
            if p.suffix == ".js":
                wasm = p.with_suffix(".wasm")
                if not p.is_file() or not wasm.is_file():
                    print(
                        f"error: wasm dtest artifacts not found: {p} / {wasm} "
                        f"({labels[i]})",
                        file=sys.stderr,
                    )
                    if i == 0:
                        print(
                            f"hint: {resolve_bazel_command()} build //wasm:dtest_wasm",
                            file=sys.stderr,
                        )
                    return 1
                if shutil.which("node") is None:
                    print(
                        "error: node is required on PATH to run dtest_wasm (.js)",
                        file=sys.stderr,
                    )
                    return 1
                continue
            if not (p.is_file() and os.access(p, os.X_OK)):
                print(
                    f"error: binary not found or not executable: {p} ({labels[i]})",
                    file=sys.stderr,
                )
                if i == 0:
                    print(
                        f"hint: {resolve_bazel_command()} build //library/tests:dtest",
                        file=sys.stderr,
                    )
                return 1

    order = run_order(num_bins, reverse=cfg.reverse)

    for f in files:
        if not (cfg.hands_dir / f).is_file():
            print(f"error: hand file not found: {cfg.hands_dir / f}", file=sys.stderr)
            return 1

    print("DDS dtest benchmark")
    print("===================")
    for i, (lab, path) in enumerate(zip(labels, paths)):
        tag = "baseline:" if i == 0 else f"binary {i + 1}:"
        print(f"{tag:<12} {lab}  ({path})")
    if num_bins >= 2:
        if cfg.details:
            print(f"{'details:':<12} on (per-run rows + build output)")
        else:
            print(f"{'details:':<12} off (summary only)")
        order_str = ", ".join(labels[idx] for idx in order)
        print(f"{'run order:':<12} interleaved {order_str}")
        if num_bins == 2:
            print(f"{'epsilon:':<12} {cfg.epsilon}%")
    if cfg.sys_user:
        print(f"{'sys-user:':<12} on")
    print(f"{'hands dir:':<12} {cfg.hands_dir}")
    print(f"{'max_deals:':<12} {cfg.max_deals}")
    print(f"{'files:':<12} {' '.join(files)}")
    print(f"{'git branch:':<12} {runner.git_branch}")
    print(f"{'repeats:':<12} {cfg.repeats}")
    if cfg.dtest_extra:
        print(f"{'dtest args:':<12} {' '.join(cfg.dtest_extra)}")
    print()
    sys.stdout.flush()

    show_run_lines = False
    alt_screen = False
    if not cfg.dry_run:
        if cfg.details:
            show_run_lines = True
        elif runner.out.isatty():
            show_run_lines = True
            alt_screen = True

    if alt_screen:
        runner.out.write(ALT_ENTER)
        runner.out.flush()
        runner.alt_screen_active = True

    def print_run_header() -> None:
        header, sep = format_run_table_header(run_label_col)
        print(header)
        print(sep)

    def print_run_row(
        solver: str,
        file: str,
        lab: str,
        user: str,
        sys_ms: str,
        avg: str,
        sys_user: str,
        run_label: str,
    ) -> None:
        print(
            format_run_table_row(
                solver, file, lab, user, sys_ms, avg, sys_user, run_label
            )
        )

    if not cfg.dry_run and show_run_lines:
        print_run_header()

    total_runs = len(SOLVERS) * len(files) * num_bins * cfg.repeats
    run_no = 0
    results: list[ResultRow] = []

    for solver in SOLVERS:
        for file in files:
            hands = cfg.hands_dir / file
            for rep in range(1, cfg.repeats + 1):
                run_label = f"{rep}/{cfg.repeats}" if cfg.repeats > 1 else "1/1"
                for idx in order:
                    bin_path = paths[idx]
                    run_no += 1
                    if cfg.dry_run:
                        runner.run_dtest(bin_path, solver, hands)
                        continue
                    parsed = runner.run_dtest(bin_path, solver, hands)
                    if show_run_lines:
                        print_run_row(
                            solver,
                            file,
                            labels[idx][:12],
                            _fmt_timing(parsed.user_ms),
                            _fmt_timing(parsed.sys_ms),
                            _fmt_timing(parsed.avg_user),
                            _fmt_timing(parsed.sys_user),
                            run_label,
                        )
                    results.append(
                        ResultRow(
                            solver,
                            file,
                            idx,
                            rep,
                            parsed.user_ms,
                            parsed.sys_ms,
                            parsed.avg_user,
                            parsed.sys_user,
                            parsed.hands,
                        )
                    )

    if alt_screen:
        runner.out.write(ALT_LEAVE)
        runner.out.flush()
        runner.alt_screen_active = False

    if not cfg.dry_run:
        print()
        print("Summary (avg user ms)")
        print("==============================================================================")
        print(
            format_summary(
                results,
                labels=labels,
                files=files,
                epsilon=cfg.epsilon,
                sys_user=cfg.sys_user,
            )
        )

    print()
    if cfg.dry_run:
        print(f"DRY_RUN: {total_runs} dtest invocations (not run).")
    else:
        print(f"Completed {run_no} runs ({total_runs} expected).")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except BenchmarkError as e:
        print(f"error: {e}", file=sys.stderr)
        raise SystemExit(1) from e
