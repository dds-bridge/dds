#!/usr/bin/env python3
"""Compare DDS vs macroxue/bridge-solver on full DD-table wall time.

Apples-to-apples: five strains × four declarers, DDS forced to one thread.
Requires a built macroxue ``solver`` binary (not vendored here).

Usage:
  bazelisk run //python/utilities:compare_bridge_solver -- \\
    --bridge-solver /path/to/solver \\
    --deals-dir /path/to/bridge-solver/deals/fixed \\
    --limit 25
"""

from __future__ import annotations

import argparse
import math
import os
import re
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Mapping, Sequence

from dds3 import calc_all_tables_pbn, initialize_static_memory

_SEATS = ("N", "E", "S", "W")
_SUITS = ("S", "H", "D", "C")
# macroxue RESULTS / stdout strain letter → DDS strain index (S,H,D,C,N)
_STRAIN_TO_DDS = {"S": 0, "H": 1, "D": 2, "C": 3, "N": 4}
# macroxue per-row trick order is S N W E → DDS hand indices N E S W
_SNWE_TO_NESW = (2, 0, 3, 1)

_TRICK_LINE_RE = re.compile(
    r"^([NSHDC])\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)(?:\s|$)"
)


@dataclass(frozen=True)
class SolveResult:
    wall_ms: float
    res_table: list[list[int]]


@dataclass(frozen=True)
class DealCompareRow:
    name: str
    dds_ms: float
    bridge_solver_ms: float
    tables_match: bool


@dataclass(frozen=True)
class CompareSummary:
    rows: list[DealCompareRow]
    total_dds_ms: float
    total_bridge_solver_ms: float
    mismatch_count: int
    geometric_mean_ratio: float


def parse_macroxue_deal(text: str) -> dict[str, list[str]]:
    """Parse a macroxue deal file into suit holdings per seat.

    Returns ``{"N"|"E"|"S"|"W": [spades, hearts, diamonds, clubs]}`` with
    empty strings for voids.
    """
    lines = [line.rstrip("\n") for line in text.splitlines() if line.strip()]
    if len(lines) < 3:
        raise ValueError(
            f"macroxue deal needs at least 3 hand lines, got {len(lines)}"
        )

    north = _four_suit_tokens(lines[0])
    west, east = _split_west_east_line(lines[1])
    south = _four_suit_tokens(lines[2])
    return {
        "N": north,
        "E": east,
        "S": south,
        "W": west,
    }


def _normalize_suit(token: str) -> str:
    token = token.strip().upper()
    if token in ("", "-"):
        return ""
    return token


def _four_suit_tokens(line: str) -> list[str]:
    tokens = line.split()
    if len(tokens) != 4:
        raise ValueError(
            f"expected 4 suit tokens, got {len(tokens)} in {line!r}"
        )
    return [_normalize_suit(t) for t in tokens]


def _split_west_east_line(line: str) -> tuple[list[str], list[str]]:
    tokens = line.split()
    if len(tokens) != 8:
        raise ValueError(
            f"West/East line needs 8 suit tokens, got {len(tokens)} in {line!r}"
        )
    west = [_normalize_suit(t) for t in tokens[:4]]
    east = [_normalize_suit(t) for t in tokens[4:]]
    return west, east


def holdings_to_pbn(holdings: dict[str, list[str]]) -> str:
    """Build a DDS PBN remain-cards string starting at North."""
    parts: list[str] = []
    for seat in _SEATS:
        suits = holdings[seat]
        if len(suits) != 4:
            raise ValueError(f"{seat} needs 4 suits, got {suits!r}")
        parts.append(".".join(suits))
    return "N:" + " ".join(parts)


def format_macroxue_deal(holdings: dict[str, list[str]]) -> str:
    """Serialize holdings back to a compact macroxue deal file."""

    def suit_tok(holding: str) -> str:
        return holding if holding else "-"

    def hand_line(seat: str, indent: int = 0) -> str:
        suits = " ".join(suit_tok(s) for s in holdings[seat])
        return (" " * indent) + suits

    west = " ".join(suit_tok(s) for s in holdings["W"])
    east = " ".join(suit_tok(s) for s in holdings["E"])
    middle = f"{west}           {east}"
    return "\n".join(
        [
            hand_line("N", 14),
            middle,
            hand_line("S", 14),
            "",
        ]
    )


def macroxue_results_to_dds_table(text: str) -> list[list[int]]:
    """Map RESULTS-style strain lines (S N W E) to DDS ``res_table``."""
    return parse_macroxue_solver_stdout(text)


def parse_macroxue_solver_stdout(text: str) -> list[list[int]]:
    """Extract the 5×4 DD table from ``solver`` stdout or a RESULTS block."""
    found: dict[str, list[int]] = {}
    for line in text.splitlines():
        match = _TRICK_LINE_RE.match(line.strip())
        if not match:
            continue
        strain = match.group(1)
        snwe = [int(match.group(i)) for i in range(2, 6)]
        nesw = [0, 0, 0, 0]
        for src, dst in enumerate(_SNWE_TO_NESW):
            nesw[dst] = snwe[src]
        found[strain] = nesw

    missing = [s for s in ("S", "H", "D", "C", "N") if s not in found]
    if missing:
        raise ValueError(
            "macroxue output missing strain line(s): " + ",".join(missing)
        )

    return [found[s] for s in ("S", "H", "D", "C", "N")]


def list_deal_files(deals_dir: Path, *, limit: int | None = None) -> list[Path]:
    """Return sorted deal files under *deals_dir*, skipping RESULTS."""
    files = sorted(
        path
        for path in deals_dir.iterdir()
        if path.is_file()
        and path.name != "RESULTS"
        and not path.name.startswith(".")
    )
    if limit is not None:
        files = files[:limit]
    return files


def compare_deals(
    deals_dir: Path,
    *,
    run_dds: Callable[[str], SolveResult],
    run_macroxue: Callable[[Path], SolveResult],
    limit: int | None = None,
) -> CompareSummary:
    """Run both solvers on each deal file and aggregate timings."""
    rows: list[DealCompareRow] = []
    ratios: list[float] = []
    total_dds = 0.0
    total_mx = 0.0
    mismatches = 0

    for path in list_deal_files(deals_dir, limit=limit):
        holdings = parse_macroxue_deal(path.read_text(encoding="utf-8"))
        pbn = holdings_to_pbn(holdings)
        dds = run_dds(pbn)
        mx = run_macroxue(path)
        match = dds.res_table == mx.res_table
        if not match:
            mismatches += 1
        rows.append(
            DealCompareRow(
                name=path.name,
                dds_ms=dds.wall_ms,
                bridge_solver_ms=mx.wall_ms,
                tables_match=match,
            )
        )
        total_dds += dds.wall_ms
        total_mx += mx.wall_ms
        if mx.wall_ms > 0:
            ratios.append(dds.wall_ms / mx.wall_ms)

    if ratios:
        geo = math.exp(sum(math.log(r) for r in ratios) / len(ratios))
    else:
        geo = float("nan")

    return CompareSummary(
        rows=rows,
        total_dds_ms=total_dds,
        total_bridge_solver_ms=total_mx,
        mismatch_count=mismatches,
        geometric_mean_ratio=geo,
    )


def format_summary(
    summary: CompareSummary,
    *,
    dds_threads: int | None = None,
) -> str:
    """Human-readable per-deal table plus totals."""
    lines = [
        f"{'deal':<12} {'dds_ms':>10} {'bridge_ms':>10} {'ratio':>8} status",
        "-" * 52,
    ]
    for row in summary.rows:
        ratio = (
            row.dds_ms / row.bridge_solver_ms
            if row.bridge_solver_ms > 0
            else float("nan")
        )
        status = "ok" if row.tables_match else "MISMATCH"
        ratio_s = f"{ratio:8.3f}" if ratio == ratio else f"{'nan':>8}"
        lines.append(
            f"{row.name:<12} {row.dds_ms:10.1f} {row.bridge_solver_ms:10.1f} "
            f"{ratio_s} {status}"
        )
    lines.append("-" * 52)
    geo = summary.geometric_mean_ratio
    geo_s = f"{geo:.3f}" if geo == geo else "nan"
    lines.append(
        f"{'TOTAL':<12} {summary.total_dds_ms:10.1f} "
        f"{summary.total_bridge_solver_ms:10.1f}"
    )
    footer = (
        f"deals={len(summary.rows)} mismatches={summary.mismatch_count} "
        f"geo_mean_dds/bridge={geo_s}"
    )
    if dds_threads is not None:
        footer += f" dds_threads={dds_threads}"
    lines.append(footer)
    return "\n".join(lines) + "\n"


def run_dds_calc_table(pbn: str, *, max_threads: int = 1) -> SolveResult:
    """Time a single-deal DD table via ``dds3``."""
    started = time.perf_counter()
    result = calc_all_tables_pbn([pbn], max_threads=max_threads)
    elapsed_ms = (time.perf_counter() - started) * 1000.0
    tables = result["tables"]
    if not tables:
        raise RuntimeError("dds3 returned no tables")
    return SolveResult(wall_ms=elapsed_ms, res_table=tables[0]["res_table"])


def run_macroxue_solver(
    deal_path: Path,
    *,
    bridge_solver: Path,
) -> SolveResult:
    """Time ``solver -i -f FILE -m0`` and parse its trick table."""
    started = time.perf_counter()
    completed = subprocess.run(
        [
            str(bridge_solver),
            "-i",
            "-f",
            str(deal_path),
            "-m0",
        ],
        check=False,
        capture_output=True,
        text=True,
    )
    elapsed_ms = (time.perf_counter() - started) * 1000.0
    if completed.returncode != 0:
        raise RuntimeError(
            f"{bridge_solver} failed on {deal_path.name} "
            f"(exit {completed.returncode}): {completed.stderr or completed.stdout}"
        )
    table = parse_macroxue_solver_stdout(completed.stdout)
    return SolveResult(wall_ms=elapsed_ms, res_table=table)


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Benchmark DDS vs macroxue/bridge-solver on full DD tables. "
            "Default DDS threading is 1 for a fair single-thread compare; "
            "raise --dds-threads to measure multi-threaded DDS."
        )
    )
    parser.add_argument(
        "--bridge-solver",
        type=Path,
        required=True,
        help="Path to the built macroxue solver binary",
    )
    parser.add_argument(
        "--deals-dir",
        type=Path,
        required=True,
        help="Directory of macroxue deal files (e.g. deals/fixed)",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Only compare the first N deal files (sorted by name)",
    )
    parser.add_argument(
        "--dds-threads",
        "--threads",
        type=int,
        default=1,
        metavar="N",
        dest="dds_threads",
        help=(
            "DDS worker threads for calc_all_tables_pbn "
            "(default: 1; 0 = all hardware threads)"
        ),
    )
    return parser


def _validate_args(args: argparse.Namespace) -> None:
    if args.dds_threads < 0:
        raise SystemExit("--dds-threads must be >= 0 (0 = all hardware threads)")


def os_access_executable(path: Path) -> bool:
    """True if *path* looks executable (or is a Windows .exe)."""
    if os.name == "nt":
        return path.suffix.lower() in {".exe", ".bat", ".cmd"} or os.access(
            path, os.X_OK
        )
    return os.access(path, os.X_OK)


def resolve_user_path(
    path: Path,
    *,
    environ: Mapping[str, str] | None = None,
    cwd: Path | None = None,
) -> Path:
    """Resolve CLI paths relative to the user's invoking directory under bazel run.

    ``bazel run`` changes cwd to the runfiles tree, so ``../bridge-solver/solver``
    must be anchored at ``BUILD_WORKING_DIRECTORY`` (the directory from which
    bazel was invoked), not the runfiles cwd.
    """
    env = os.environ if environ is None else environ
    expanded = path.expanduser()
    if expanded.is_absolute():
        return expanded.resolve()

    working = env.get("BUILD_WORKING_DIRECTORY")
    base = Path(working) if working else (cwd if cwd is not None else Path.cwd())
    return (base / expanded).resolve()


def main(argv: Sequence[str] | None = None) -> int:
    args = _build_parser().parse_args(argv)
    _validate_args(args)
    bridge_solver = resolve_user_path(args.bridge_solver)
    deals_dir = resolve_user_path(args.deals_dir)

    if not bridge_solver.is_file():
        print(f"bridge-solver not found: {bridge_solver}", file=sys.stderr)
        return 1
    if not os_access_executable(bridge_solver):
        print(
            f"bridge-solver is not executable: {bridge_solver}",
            file=sys.stderr,
        )
        return 1
    if not deals_dir.is_dir():
        print(f"deals directory not found: {deals_dir}", file=sys.stderr)
        return 1

    initialize_static_memory()

    def dds_runner(pbn: str) -> SolveResult:
        return run_dds_calc_table(pbn, max_threads=args.dds_threads)

    def mx_runner(path: Path) -> SolveResult:
        return run_macroxue_solver(path, bridge_solver=bridge_solver)

    try:
        summary = compare_deals(
            deals_dir,
            run_dds=dds_runner,
            run_macroxue=mx_runner,
            limit=args.limit,
        )
    except (ValueError, RuntimeError, OSError) as exc:
        print(f"compare failed: {exc}", file=sys.stderr)
        return 1

    sys.stdout.write(
        format_summary(summary, dds_threads=args.dds_threads)
    )
    return 1 if summary.mismatch_count else 0

if __name__ == "__main__":
    raise SystemExit(main())
