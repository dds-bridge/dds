"""Tests for analyse_play_pbn, analyse_all_plays_pbn, dealer_par and set_max_threads."""

import unittest

from dds3 import (
    SolverContext,
    analyse_all_plays_pbn,
    analyse_play,
    analyse_play_pbn,
    calc_dd_table,
    dealer_par,
    set_max_threads,
)

# A full 52-card deal in PBN (from docs/python_interface.md).
DEAL = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"

# A laydown deal (North all spades, East all hearts, South all diamonds, West
# all clubs) in both binary and PBN form, matching the C++ dds_c_api_test.cpp
# reference board, so a play on one representation can be cross-checked
# against the same play on the other.
REFERENCE_DEAL_BIN = {
    "trump": 0,  # Spades
    "first": 0,  # North
    "remain_cards": [
        [0x7FFC, 0, 0, 0],
        [0, 0x7FFC, 0, 0],
        [0, 0, 0x7FFC, 0],
        [0, 0, 0, 0x7FFC],
    ],
    "current_trick_suit": (0, 0, 0),
    "current_trick_rank": (0, 0, 0),
}
REFERENCE_DEAL_PBN = (
    "N:AKQJT98765432... .AKQJT98765432.. ..AKQJT98765432. ...AKQJT98765432"
)

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

    def test_analyse_play_basic(self) -> None:
        # North leads the spade 2 on the reference (binary) deal.
        result = analyse_play(REFERENCE_DEAL_BIN, play_suit=[0], play_rank=[2])
        self.assertIn("number", result)
        self.assertIn("tricks", result)
        self.assertIsInstance(result["tricks"], list)
        self.assertEqual(len(result["tricks"]), result["number"])

    def test_analyse_play_matches_analyse_play_pbn(self) -> None:
        # Same deal and play, binary vs. PBN representation: must agree.
        bin_result = analyse_play(REFERENCE_DEAL_BIN, play_suit=[0], play_rank=[2])
        pbn_result = analyse_play_pbn(REFERENCE_DEAL_PBN, play="S2", trump=0, first=0)
        self.assertEqual(bin_result["number"], pbn_result["number"])
        self.assertEqual(bin_result["tricks"], pbn_result["tricks"])

    def test_analyse_play_with_context(self) -> None:
        ctx = SolverContext()
        with_context = analyse_play(
            REFERENCE_DEAL_BIN, play_suit=[0], play_rank=[2], context=ctx)
        without_context = analyse_play(REFERENCE_DEAL_BIN, play_suit=[0], play_rank=[2])
        self.assertEqual(with_context, without_context)

    def test_analyse_play_mismatched_lengths(self) -> None:
        with self.assertRaises(ValueError):
            analyse_play(REFERENCE_DEAL_BIN, play_suit=[0, 1], play_rank=[2])


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


class TestSetMaxThreads(unittest.TestCase):
    """Tests for the deprecated set_max_threads() thread-resource hook.

    user_threads is ignored regardless of value (see docs/python_interface.md);
    it is accepted only for backward compatibility with the legacy call
    signature, so a negative value must not be rejected.
    """

    def test_negative_user_threads_is_ignored_not_rejected(self) -> None:
        with self.assertWarns(DeprecationWarning):
            set_max_threads(-1)

    def test_emits_deprecation_warning(self) -> None:
        with self.assertWarns(DeprecationWarning):
            set_max_threads(0)


if __name__ == "__main__":
    unittest.main()
