"""Tests for context reuse with solve_board and calc_par."""

import unittest

from dds3 import SolverContext
from dds3 import calc_par
from dds3 import solve_board
from dds3 import solve_board_pbn


# Test data constants
R2 = 0x0004
R3 = 0x0008
R4 = 0x0010
R5 = 0x0020
R6 = 0x0040
R7 = 0x0080
R8 = 0x0100
R9 = 0x0200
RT = 0x0400
RJ = 0x0800
RQ = 0x1000
RK = 0x2000
RA = 0x4000


class TestContextReuse(unittest.TestCase):
    def _make_simple_deal(self) -> dict:
        return {
            "trump": 0,
            "first": 0,
            "remain_cards": [
                [0x7FFC, 0, 0, 0],
                [0, 0x7FFC, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }

    def _make_table_deal(self) -> dict:
        return {
            "cards": [
                [RQ | RJ | R6, RK | R6 | R5 | R2, RJ | R8 | R5, RT | R9 | R8],
                [R8 | R7 | R3, RJ | R9 | R7, RA | RT | R7 | R6 | R4, RQ | R4],
                [RK | R5, RT | R8 | R3, RK | RQ | R9, RA | R7 | R6 | R5 | R2],
                [RA | RT | R9 | R4 | R2, RA | RQ | R4, R3 | R2, RK | RJ | R3],
            ]
        }

    def test_create_solver_context(self) -> None:
        ctx = SolverContext()
        self.assertIsNotNone(ctx)
        self.assertIsInstance(ctx, SolverContext)

    def test_multiple_contexts(self) -> None:
        ctx1 = SolverContext()
        ctx2 = SolverContext()
        self.assertIsNotNone(ctx1)
        self.assertIsNotNone(ctx2)
        self.assertIsNot(ctx1, ctx2)

    def test_solve_board_without_context(self) -> None:
        result = solve_board(self._make_simple_deal())
        self.assertIn("nodes", result)
        self.assertIn("score", result)

    def test_solve_board_with_context(self) -> None:
        ctx = SolverContext()
        result = solve_board(self._make_simple_deal(), context=ctx)
        self.assertIn("nodes", result)
        self.assertIn("score", result)

    def test_solve_board_context_reuse(self) -> None:
        ctx = SolverContext()
        deal1 = self._make_simple_deal()
        deal2 = {
            "trump": 1,
            "first": 1,
            "remain_cards": [
                [0, 0x7FFC, 0, 0],
                [0x7FFC, 0, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }

        result1 = solve_board(deal1, context=ctx)
        result2 = solve_board(deal2, context=ctx)

        self.assertIn("nodes", result1)
        self.assertIn("score", result1)
        self.assertIn("nodes", result2)
        self.assertIn("score", result2)

    def test_solve_board_pbn_with_context(self) -> None:
        ctx = SolverContext()
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        result = solve_board_pbn(pbn, trump=4, first=0, context=ctx)
        self.assertIn("nodes", result)
        self.assertIn("score", result)

    def test_solve_board_pbn_without_context(self) -> None:
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        result = solve_board_pbn(pbn, trump=4, first=0)
        self.assertIn("nodes", result)
        self.assertIn("score", result)

    def test_calc_par_without_context(self) -> None:
        result = calc_par(self._make_table_deal(), vulnerable=0)
        self.assertIsInstance(result, dict)
        self.assertIn("dd_table", result)
        self.assertIn("par_results", result)

    def test_calc_par_results_consistency(self) -> None:
        table_deal = self._make_table_deal()
        result1 = calc_par(table_deal, vulnerable=0)
        result2 = calc_par(table_deal, vulnerable=0)
        self.assertEqual(result1["par_results"]["par_score"], result2["par_results"]["par_score"])


if __name__ == "__main__":
    unittest.main()
