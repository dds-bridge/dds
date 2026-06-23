"""Tests for analyse_play_pbn, analyse_all_plays_pbn and dealer_par."""

import unittest

from dds3 import (
    analyse_all_plays_pbn,
    analyse_play_pbn,
    calc_dd_table,
    dealer_par,
    set_max_threads,
)

# A full 52-card deal in PBN (from docs/python_interface.md).
DEAL = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"

# Rank bitmask constants.
R2, R3, R4, R5, R6, R7, R8 = 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100
R9, RT, RJ, RQ, RK, RA = 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000

# Test hand 0 from examples/hands.cpp (cards[hand][suit]).
HAND0 = {
    "cards": [
        [RQ | RJ | R6, RK | R6 | R5 | R2, RJ | R8 | R5, RT | R9 | R8],
        [R8 | R7 | R3, RJ | R9 | R7, RA | RT | R7 | R6 | R4, RQ | R4],
        [RK | R5, RT | R8 | R3, RK | RQ | R9, RA | R7 | R6 | R5 | R2],
        [RA | RT | R9 | R4 | R2, RA | RQ | R4, R3 | R2, RK | RJ | R3],
    ]
}


class TestAnalysePlay(unittest.TestCase):
    """Tests for analyse_play_pbn / analyse_all_plays_pbn."""

    def setUp(self) -> None:
        set_max_threads(0)

    def test_analyse_play_pbn_basic(self) -> None:
        # North leads the spade 6 in NT.
        result = analyse_play_pbn(DEAL, play="S6", trump=4, first=0)
        self.assertIn("number", result)
        self.assertIn("tricks", result)
        self.assertIsInstance(result["tricks"], list)
        self.assertEqual(len(result["tricks"]), result["number"])

    def test_analyse_play_pbn_odd_length(self) -> None:
        with self.assertRaises(ValueError):
            analyse_play_pbn(DEAL, play="S6S", trump=4, first=0)

    def test_analyse_all_plays_pbn(self) -> None:
        deals = [
            {"remain_cards": DEAL, "play": "S6", "trump": 4, "first": 0},
            {"remain_cards": DEAL, "play": "S6", "trump": 1, "first": 0},
        ]
        results = analyse_all_plays_pbn(deals)
        self.assertEqual(len(results), 2)
        for result in results:
            self.assertIn("tricks", result)
            self.assertIsInstance(result["tricks"], list)

    def test_analyse_all_plays_missing_play(self) -> None:
        with self.assertRaises(KeyError):
            analyse_all_plays_pbn([{"remain_cards": DEAL}])


class TestDealerPar(unittest.TestCase):
    """Tests for dealer_par."""

    def test_dealer_par_basic(self) -> None:
        dd_table = calc_dd_table(HAND0)
        result = dealer_par(dd_table, dealer=0, vulnerable=0)
        self.assertIn("score", result)
        self.assertIn("number", result)
        self.assertIn("contracts", result)
        self.assertIsInstance(result["score"], int)
        self.assertIsInstance(result["contracts"], list)
        self.assertEqual(len(result["contracts"]), result["number"])

    def test_dealer_par_all_dealers(self) -> None:
        dd_table = calc_dd_table(HAND0)
        for dealer in range(4):
            result = dealer_par(dd_table, dealer=dealer, vulnerable=0)
            self.assertIsInstance(result["score"], int)

    def test_dealer_par_invalid_dealer(self) -> None:
        dd_table = calc_dd_table(HAND0)
        with self.assertRaises(ValueError):
            dealer_par(dd_table, dealer=4, vulnerable=0)

    def test_dealer_par_invalid_vulnerability(self) -> None:
        dd_table = calc_dd_table(HAND0)
        with self.assertRaises(ValueError):
            dealer_par(dd_table, dealer=0, vulnerable=9)


if __name__ == "__main__":
    unittest.main()
