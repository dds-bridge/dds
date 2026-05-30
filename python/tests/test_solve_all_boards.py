"""Tests for solve_all_boards_pbn and solve_all_boards_bin wrappers."""

import unittest

from dds3 import solve_all_boards_bin, solve_all_boards_pbn, solve_board, solve_board_pbn
from test_utils import assert_raises

# Reused across tests: a complete, valid 13-card deal in PBN notation.
_PBN = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"

# Binary equivalent of the same deal for solve_all_boards_bin tests.
# remainCards[hand][suit] bitmasks for: N, E, S, W × ♠,♥,♦,♣
# Bit n is set for rank n (2→bit2=0x4, A→bit14=0x4000). Each suit sums to 0x7FFC.
_BINARY_DEAL = {
    "trump": 4,  # NT
    "first": 0,  # North
    "remain_cards": [
        [0x1840, 0x2064, 0x0920, 0x0700],  # North  ♠QJ6 ♥K652 ♦J85 ♣T98
        [0x0188, 0x0A80, 0x44D0, 0x1010],  # East   ♠873 ♥J97 ♦AT764 ♣Q4
        [0x2020, 0x0508, 0x3200, 0x40E4],  # South  ♠K5 ♥T83 ♦KQ9 ♣A7652
        [0x4614, 0x5010, 0x000C, 0x2808],  # West   ♠AT942 ♥AQ4 ♦32 ♣KJ3
    ],
    "current_trick_suit": (0, 0, 0),
    "current_trick_rank": (0, 0, 0),
}


def _result_shape_ok(result: dict) -> bool:
    """Return True if a FutureTricks dict has the expected shape."""
    return (
        "nodes" in result
        and "cards" in result
        and "suit" in result
        and "rank" in result
        and "equals" in result
        and "score" in result
        and len(result["suit"]) == 13
    )


class TestSolveAllBoardsPBN(unittest.TestCase):
    """Tests for solve_all_boards_pbn (PBN format batch input)."""

    def test_single_board_returns_list_of_one(self) -> None:
        boards = [{"remain_cards": _PBN, "trump": 4, "first": 0}]
        results = solve_all_boards_pbn(boards)
        self.assertIsInstance(results, list)
        self.assertEqual(len(results), 1)
        self.assertTrue(_result_shape_ok(results[0]))

    def test_multiple_boards(self) -> None:
        boards = [{"remain_cards": _PBN, "trump": 4, "first": 0}] * 3
        results = solve_all_boards_pbn(boards)
        self.assertEqual(len(results), 3)
        for r in results:
            self.assertTrue(_result_shape_ok(r))

    def test_empty_list_returns_empty(self) -> None:
        results = solve_all_boards_pbn([])
        self.assertEqual(results, [])

    def test_all_optional_keys_default(self) -> None:
        """Only remain_cards is required; all others have documented defaults."""
        boards = [{"remain_cards": _PBN}]
        results = solve_all_boards_pbn(boards)
        self.assertEqual(len(results), 1)
        self.assertTrue(_result_shape_ok(results[0]))

    def test_result_matches_single_board_pbn(self) -> None:
        """Batch result for one board should agree with single-board result on card count."""
        single = solve_board_pbn(_PBN, trump=4, first=0)
        batch = solve_all_boards_pbn([{"remain_cards": _PBN, "trump": 4, "first": 0}])
        # Only the first `cards` entries are meaningful; compare the count and top score.
        self.assertEqual(single["cards"], batch[0]["cards"])
        self.assertEqual(single["score"][0], batch[0]["score"][0])

    def test_different_trump_suits_accepted(self) -> None:
        for trump in range(5):
            boards = [{"remain_cards": _PBN, "trump": trump, "first": 0}]
            results = solve_all_boards_pbn(boards)
            self.assertEqual(len(results), 1)

    def test_different_first_seats_accepted(self) -> None:
        for first in range(4):
            boards = [{"remain_cards": _PBN, "trump": 4, "first": first}]
            results = solve_all_boards_pbn(boards)
            self.assertEqual(len(results), 1)

    def test_invalid_trump_raises_value_error(self) -> None:
        boards = [{"remain_cards": _PBN, "trump": 5}]
        assert_raises(ValueError, solve_all_boards_pbn, boards, match="invalid value 5")

    def test_invalid_first_raises_value_error(self) -> None:
        boards = [{"remain_cards": _PBN, "first": 4}]
        assert_raises(ValueError, solve_all_boards_pbn, boards, match="invalid value 4")

    def test_invalid_trick_suit_raises_value_error(self) -> None:
        boards = [{"remain_cards": _PBN, "current_trick_suit": (0, 0, 5)}]
        assert_raises(ValueError, solve_all_boards_pbn, boards, match="invalid value 5")

    def test_invalid_trick_rank_raises_value_error(self) -> None:
        boards = [{"remain_cards": _PBN, "current_trick_rank": (2, 2, 15)}]
        assert_raises(ValueError, solve_all_boards_pbn, boards, match="invalid value 15")

    def test_too_many_boards_raises_value_error(self) -> None:
        boards = [{"remain_cards": _PBN}] * 201
        assert_raises(ValueError, solve_all_boards_pbn, boards, match="exceeds maximum")

    def test_invalid_pbn_string_raises_error(self) -> None:
        boards = [{"remain_cards": "not a pbn"}]
        assert_raises((ValueError, RuntimeError), solve_all_boards_pbn, boards)

    def test_per_board_target_and_solutions(self) -> None:
        boards = [{"remain_cards": _PBN, "trump": 4, "first": 0, "target": 9, "solutions": 1, "mode": 0}]
        results = solve_all_boards_pbn(boards)
        self.assertTrue(_result_shape_ok(results[0]))


class TestSolveAllBoardsBin(unittest.TestCase):
    """Tests for solve_all_boards_bin (binary format batch input)."""

    def test_single_board_returns_list_of_one(self) -> None:
        results = solve_all_boards_bin([_BINARY_DEAL])
        self.assertIsInstance(results, list)
        self.assertEqual(len(results), 1)
        self.assertTrue(_result_shape_ok(results[0]))

    def test_multiple_boards(self) -> None:
        results = solve_all_boards_bin([_BINARY_DEAL] * 3)
        self.assertEqual(len(results), 3)
        for r in results:
            self.assertTrue(_result_shape_ok(r))

    def test_empty_list_returns_empty(self) -> None:
        results = solve_all_boards_bin([])
        self.assertEqual(results, [])

    def test_result_matches_single_board_bin(self) -> None:
        """Batch result for one board should agree with single-board result on card count."""
        single = solve_board(_BINARY_DEAL)
        batch = solve_all_boards_bin([_BINARY_DEAL])
        self.assertEqual(single["cards"], batch[0]["cards"])
        self.assertEqual(single["score"][0], batch[0]["score"][0])

    def test_invalid_trump_raises_value_error(self) -> None:
        bad = {**_BINARY_DEAL, "trump": 5}
        assert_raises(ValueError, solve_all_boards_bin, [bad], match="invalid value 5")

    def test_invalid_first_raises_value_error(self) -> None:
        bad = {**_BINARY_DEAL, "first": 4}
        assert_raises(ValueError, solve_all_boards_bin, [bad], match="invalid value 4")

    def test_invalid_trick_suit_raises_value_error(self) -> None:
        bad = {**_BINARY_DEAL, "current_trick_suit": (0, 0, 5)}
        assert_raises(ValueError, solve_all_boards_bin, [bad], match="invalid value 5")

    def test_invalid_trick_rank_raises_value_error(self) -> None:
        bad = {**_BINARY_DEAL, "current_trick_rank": (2, 2, 15)}
        assert_raises(ValueError, solve_all_boards_bin, [bad], match="invalid value 15")

    def test_too_many_boards_raises_value_error(self) -> None:
        assert_raises(ValueError, solve_all_boards_bin, [_BINARY_DEAL] * 201, match="exceeds maximum")

    def test_invalid_remain_cards_size_raises_value_error(self) -> None:
        bad = {**_BINARY_DEAL, "remain_cards": [[0, 0, 0, 0]] * 3}  # 3 hands instead of 4
        assert_raises(ValueError, solve_all_boards_bin, [bad])

    def test_per_board_target_and_solutions(self) -> None:
        board = {**_BINARY_DEAL, "target": 9, "solutions": 1, "mode": 0}
        results = solve_all_boards_bin([board])
        self.assertTrue(_result_shape_ok(results[0]))

    def test_different_trump_suits_accepted(self) -> None:
        for trump in range(5):
            board = {**_BINARY_DEAL, "trump": trump}
            results = solve_all_boards_bin([board])
            self.assertEqual(len(results), 1)


class TestSolveAllBoardsParity(unittest.TestCase):
    """Cross-function consistency between PBN and binary batch variants."""

    def test_pbn_and_bin_agree_on_cards(self) -> None:
        """solve_all_boards_pbn and solve_all_boards_bin should agree on trick count and top score."""
        pbn_results = solve_all_boards_pbn([{"remain_cards": _PBN, "trump": 4, "first": 0}])
        bin_results = solve_all_boards_bin([_BINARY_DEAL])
        self.assertEqual(pbn_results[0]["cards"], bin_results[0]["cards"])
        self.assertEqual(pbn_results[0]["score"][0], bin_results[0]["score"][0])


if __name__ == "__main__":
    unittest.main()
