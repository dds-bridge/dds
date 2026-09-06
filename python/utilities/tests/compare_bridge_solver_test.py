#!/usr/bin/env python3
"""Unit tests for compare_bridge_solver."""

from __future__ import annotations

import math
import tempfile
import unittest
from pathlib import Path

import compare_bridge_solver as cbs

# deals/fixed/deal.01 from https://github.com/macroxue/bridge-solver
_DEAL_01_TEXT = """\
              J75 AQT86 J AK95 
T3 74 AK763 J864           92 KJ92 T985 Q72 
              AKQ864 53 Q42 T3 
"""

_DEAL_01_PBN = (
    "N:J75.AQT86.J.AK95 92.KJ92.T985.Q72 AKQ864.53.Q42.T3 T3.74.AK763.J864"
)

# RESULTS / solver stdout order per strain: S N W E
_DEAL_01_RESULTS_LINES = """\
N  9  9  3  3
S 11 11  2  2
H  8  8  4  4
D  6  6  6  6
C  7  7  3  3
"""

# DDS res_table[strain][N,E,S,W]
_DEAL_01_DDS_TABLE = [
    [11, 2, 11, 2],  # S
    [8, 4, 8, 4],  # H
    [6, 6, 6, 6],  # D
    [7, 3, 7, 3],  # C
    [9, 3, 9, 3],  # N
]

_VOID_DEAL_TEXT = """\
               KQ3 - T832 AJ9765
72 AJ972 AQ7 KQ2               T96 K83 654 T843
               AJ854 QT654 KJ9 -
"""

_VOID_DEAL_PBN = (
    "N:KQ3..T832.AJ9765 T96.K83.654.T843 AJ854.QT654.KJ9. 72.AJ972.AQ7.KQ2"
)

_SOLVER_STDOUT = """\
N  9  9  3  3  0.01 s 4064.0 M
S 11 11  2  2  0.01 s 4096.0 M
H  8  8  4  4  0.04 s 5440.0 M
D  6  6  6  6  0.05 s 5616.0 M
C  7  7  3  3  0.11 s 8480.0 M
"""


class ParseDealFileTest(unittest.TestCase):
    def test_parse_deal_01_to_pbn(self) -> None:
        holdings = cbs.parse_macroxue_deal(_DEAL_01_TEXT)
        self.assertEqual(cbs.holdings_to_pbn(holdings), _DEAL_01_PBN)

    def test_parse_deal_with_voids_to_pbn(self) -> None:
        holdings = cbs.parse_macroxue_deal(_VOID_DEAL_TEXT)
        self.assertEqual(cbs.holdings_to_pbn(holdings), _VOID_DEAL_PBN)

    def test_round_trip_write_and_parse(self) -> None:
        holdings = cbs.parse_macroxue_deal(_DEAL_01_TEXT)
        text = cbs.format_macroxue_deal(holdings)
        again = cbs.parse_macroxue_deal(text)
        self.assertEqual(cbs.holdings_to_pbn(again), _DEAL_01_PBN)

    def test_parse_rejects_truncated_deal(self) -> None:
        with self.assertRaises(ValueError):
            cbs.parse_macroxue_deal("J75 AQT86 J AK95\n")


class ResultsMappingTest(unittest.TestCase):
    def test_results_lines_map_to_dds_res_table(self) -> None:
        table = cbs.macroxue_results_to_dds_table(_DEAL_01_RESULTS_LINES)
        self.assertEqual(table, _DEAL_01_DDS_TABLE)


class ParseSolverStdoutTest(unittest.TestCase):
    def test_parse_solver_stdout_tricks(self) -> None:
        table = cbs.parse_macroxue_solver_stdout(_SOLVER_STDOUT)
        self.assertEqual(table, _DEAL_01_DDS_TABLE)

    def test_parse_solver_stdout_ignores_deal_diagram(self) -> None:
        noisy = (
            "                          ♠ J75 ♥ AQT86 ♦ J ♣ AK95\n"
            + _SOLVER_STDOUT
        )
        table = cbs.parse_macroxue_solver_stdout(noisy)
        self.assertEqual(table, _DEAL_01_DDS_TABLE)

    def test_parse_solver_stdout_requires_five_strains(self) -> None:
        with self.assertRaises(ValueError):
            cbs.parse_macroxue_solver_stdout("N  9  9  3  3  0.01 s\n")


class CompareSummaryTest(unittest.TestCase):
    def test_compare_deals_aggregates_timings_and_mismatches(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            deals_dir = Path(tmp)
            (deals_dir / "deal.01").write_text(_DEAL_01_TEXT, encoding="utf-8")
            (deals_dir / "RESULTS").write_text("ignored\n", encoding="utf-8")

            def fake_dds(_pbn: str) -> cbs.SolveResult:
                return cbs.SolveResult(
                    wall_ms=20.0, res_table=_DEAL_01_DDS_TABLE
                )

            def fake_mx(_path: Path) -> cbs.SolveResult:
                return cbs.SolveResult(
                    wall_ms=10.0, res_table=_DEAL_01_DDS_TABLE
                )

            summary = cbs.compare_deals(
                deals_dir,
                run_dds=fake_dds,
                run_macroxue=fake_mx,
            )

        self.assertEqual(len(summary.rows), 1)
        self.assertEqual(summary.rows[0].name, "deal.01")
        self.assertEqual(summary.rows[0].dds_ms, 20.0)
        self.assertEqual(summary.rows[0].bridge_solver_ms, 10.0)
        self.assertTrue(summary.rows[0].tables_match)
        self.assertEqual(summary.mismatch_count, 0)
        self.assertEqual(summary.total_dds_ms, 20.0)
        self.assertEqual(summary.total_bridge_solver_ms, 10.0)
        self.assertAlmostEqual(summary.geometric_mean_ratio, 2.0)

    def test_compare_deals_flags_table_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            deals_dir = Path(tmp)
            (deals_dir / "deal.01").write_text(_DEAL_01_TEXT, encoding="utf-8")

            bad = [row[:] for row in _DEAL_01_DDS_TABLE]
            bad[0][0] = 99

            summary = cbs.compare_deals(
                deals_dir,
                run_dds=lambda _pbn: cbs.SolveResult(1.0, _DEAL_01_DDS_TABLE),
                run_macroxue=lambda _path: cbs.SolveResult(1.0, bad),
            )

        self.assertEqual(summary.mismatch_count, 1)
        self.assertFalse(summary.rows[0].tables_match)

    def test_compare_deals_respects_limit(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            deals_dir = Path(tmp)
            for name in ("deal.01", "deal.02"):
                (deals_dir / name).write_text(_DEAL_01_TEXT, encoding="utf-8")

            summary = cbs.compare_deals(
                deals_dir,
                run_dds=lambda _pbn: cbs.SolveResult(1.0, _DEAL_01_DDS_TABLE),
                run_macroxue=lambda _path: cbs.SolveResult(
                    1.0, _DEAL_01_DDS_TABLE
                ),
                limit=1,
            )

        self.assertEqual(len(summary.rows), 1)

    def test_format_summary_includes_ok_and_totals(self) -> None:
        summary = cbs.CompareSummary(
            rows=[
                cbs.DealCompareRow(
                    name="deal.01",
                    dds_ms=20.0,
                    bridge_solver_ms=10.0,
                    tables_match=True,
                )
            ],
            total_dds_ms=20.0,
            total_bridge_solver_ms=10.0,
            mismatch_count=0,
            geometric_mean_ratio=2.0,
        )
        text = cbs.format_summary(summary, dds_threads=8)
        self.assertIn("deal.01", text)
        self.assertIn("ok", text)
        self.assertIn("20.0", text)
        self.assertIn("10.0", text)
        self.assertIn("ratio", text.lower())
        self.assertIn("dds_threads=8", text)
        self.assertNotIn("MISMATCH", text)

    def test_parser_accepts_dds_threads_and_threads_alias(self) -> None:
        parser = cbs._build_parser()
        args = parser.parse_args(
            [
                "--bridge-solver",
                "/tmp/solver",
                "--deals-dir",
                "/tmp/deals",
                "--dds-threads",
                "8",
            ]
        )
        self.assertEqual(args.dds_threads, 8)

        aliased = parser.parse_args(
            [
                "--bridge-solver",
                "/tmp/solver",
                "--deals-dir",
                "/tmp/deals",
                "--threads",
                "4",
            ]
        )
        self.assertEqual(aliased.dds_threads, 4)

    def test_parser_defaults_to_one_dds_thread(self) -> None:
        parser = cbs._build_parser()
        args = parser.parse_args(
            [
                "--bridge-solver",
                "/tmp/solver",
                "--deals-dir",
                "/tmp/deals",
            ]
        )
        self.assertEqual(args.dds_threads, 1)

    def test_parser_rejects_negative_dds_threads(self) -> None:
        parser = cbs._build_parser()
        args = parser.parse_args(
            [
                "--bridge-solver",
                "/tmp/solver",
                "--deals-dir",
                "/tmp/deals",
                "--dds-threads",
                "-1",
            ]
        )
        with self.assertRaises(SystemExit):
            cbs._validate_args(args)

    def test_geometric_mean_ratio_empty_is_nan(self) -> None:
        summary = cbs.CompareSummary(
            rows=[],
            total_dds_ms=0.0,
            total_bridge_solver_ms=0.0,
            mismatch_count=0,
            geometric_mean_ratio=float("nan"),
        )
        self.assertTrue(math.isnan(summary.geometric_mean_ratio))


class ResolveUserPathTest(unittest.TestCase):
    def test_absolute_path_unchanged(self) -> None:
        absolute = Path("/tmp/bridge-solver/solver").resolve()
        self.assertEqual(
            cbs.resolve_user_path(
                absolute,
                environ={},
                cwd=Path("/somewhere/else"),
            ),
            absolute,
        )

    def test_relative_path_uses_build_working_directory(self) -> None:
        # Arrange: bazel run sets cwd to runfiles; user meant the invoking dir.
        with tempfile.TemporaryDirectory() as tmp:
            working = Path(tmp) / "dds"
            sibling = Path(tmp) / "bridge-solver"
            working.mkdir()
            sibling.mkdir()
            solver = sibling / "solver"
            solver.write_text("", encoding="utf-8")

            # Act
            resolved = cbs.resolve_user_path(
                Path("../bridge-solver/solver"),
                environ={"BUILD_WORKING_DIRECTORY": str(working)},
                cwd=Path(tmp) / "fake-runfiles",
            )

            # Assert
            self.assertEqual(resolved, solver.resolve())

    def test_relative_path_falls_back_to_cwd(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            cwd = Path(tmp)
            target = cwd / "solver"
            target.write_text("", encoding="utf-8")

            resolved = cbs.resolve_user_path(
                Path("solver"),
                environ={},
                cwd=cwd,
            )

            self.assertEqual(resolved, target.resolve())


if __name__ == "__main__":
    unittest.main()
