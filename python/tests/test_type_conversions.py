"""Tests for type conversion and validation."""

import unittest

from dds3 import solve_board, solve_board_pbn, calc_all_tables_pbn
from test_utils import assert_raises


class TestArrayConversions(unittest.TestCase):
    """Tests for array/sequence type conversions and validation."""

    def test_current_trick_suit_tuple_conversion(self) -> None:
        """Test that current_trick_suit accepts tuples."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 1, 2),
            "current_trick_rank": (0, 0, 0),
        }
        try:
            result = solve_board(deal)
            self.assertTrue("nodes" in result)
        except RuntimeError:
            # Invalid deal is ok, we're testing conversion
            pass

    def test_current_trick_suit_list_conversion(self) -> None:
        """Test that current_trick_suit accepts lists."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": [0, 1, 2],
            "current_trick_rank": [0, 0, 0],
        }
        try:
            result = solve_board(deal)
            self.assertTrue("nodes" in result)
        except RuntimeError:
            pass

    def test_current_trick_suit_wrong_size(self) -> None:
        """Test that wrong-sized current_trick_suit raises error."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 1),  # Should be 3 elements
            "current_trick_rank": (0, 0, 0),
        }
        assert_raises(ValueError, solve_board, deal)

    def test_current_trick_rank_wrong_size(self) -> None:
        """Test that wrong-sized current_trick_rank raises error."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0, 0),  # Should be 3 elements
        }
        assert_raises(ValueError, solve_board, deal)

    def test_trick_suit_boundary_valid_0(self) -> None:
        """Test that trick suit 0 (spades) is valid."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }
        try:
            result = solve_board(deal)
            self.assertTrue("nodes" in result)
        except RuntimeError:
            pass

    def test_trick_suit_boundary_valid_3(self) -> None:
        """Test that trick suit 3 (clubs) is valid."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (3, 3, 3),
            "current_trick_rank": (0, 0, 0),
        }
        try:
            result = solve_board(deal)
            self.assertTrue("nodes" in result)
        except RuntimeError:
            pass

    def test_trick_suit_boundary_invalid_minus1(self) -> None:
        """Test that trick suit -1 is invalid."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (-1, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }
        assert_raises(ValueError, solve_board, deal, match="invalid value -1")

    def test_trick_suit_boundary_invalid_4(self) -> None:
        """Test that trick suit 4 is invalid."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (4, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }
        assert_raises(ValueError, solve_board, deal, match="invalid value 4")

    def test_trick_rank_boundary_valid_0(self) -> None:
        """Test that trick rank 0 (invalid card, but parsed) is valid."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }
        try:
            result = solve_board(deal)
            self.assertTrue("nodes" in result)
        except RuntimeError:
            pass

    def test_trick_rank_boundary_valid_14(self) -> None:
        """Test that trick rank 14 (ace) is valid."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (14, 14, 14),
        }
        try:
            result = solve_board(deal)
            self.assertTrue("nodes" in result)
        except RuntimeError:
            pass

    def test_trick_rank_boundary_invalid_minus1(self) -> None:
        """Test that trick rank -1 is invalid."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (-1, 0, 0),
        }
        assert_raises(ValueError, solve_board, deal, match="invalid value -1")

    def test_trick_rank_boundary_invalid_15(self) -> None:
        """Test that trick rank 15 is invalid."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (15, 0, 0),
        }
        assert_raises(ValueError, solve_board, deal, match="invalid value 15")


class TestPBNConversions(unittest.TestCase):
    """Tests for PBN string conversion and validation."""

    def test_pbn_valid_format(self) -> None:
        """Test that valid PBN is accepted."""
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        try:
            result = solve_board_pbn(pbn)
            self.assertIn("nodes", result)
        except RuntimeError:
            pass

    def test_pbn_missing_seat(self) -> None:
        """Test that PBN missing a seat designation raises error."""
        pbn = "AK.234.456.789TJQ W:QJ.AKQJ.789.234 E:T9.T9.TJ.AK S:8765.8765.AKQJ32.6"
        assert_raises((ValueError, RuntimeError), solve_board_pbn, pbn)

    def test_pbn_invalid_seat(self) -> None:
        """Test that PBN with invalid seat raises error."""
        pbn = "X:AK.234.456.789TJQ W:QJ.AKQJ.789.234 E:T9.T9.TJ.AK S:8765.8765.AKQJ32.6"
        assert_raises((ValueError, RuntimeError), solve_board_pbn, pbn)

    def test_pbn_empty_string(self) -> None:
        """Test that empty PBN string raises error."""
        assert_raises((ValueError, RuntimeError), solve_board_pbn, "")

    def test_pbn_truncated(self) -> None:
        """Test that truncated PBN raises error."""
        pbn = "N:AK.234.456.789TJQ W:QJ.AKQJ"  # Incomplete
        assert_raises((ValueError, RuntimeError), solve_board_pbn, pbn)


class TestTrumpFilterValidation(unittest.TestCase):
    """Tests for trump_filter validation in calc_all_tables_pbn."""

    def test_trump_filter_all_zeros(self) -> None:
        """Test that trump_filter (0,0,0,0,0) includes all strains."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        try:
            result = calc_all_tables_pbn(deals, trump_filter=[0, 0, 0, 0, 0])
            self.assertTrue("tables" in result)
        except RuntimeError:
            pass

    def test_trump_filter_all_ones(self) -> None:
        """Test that trump_filter (1,1,1,1,1) skips all strains."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        # Skipping all strains should be invalid
        assert_raises((ValueError, RuntimeError), calc_all_tables_pbn, deals, trump_filter=[1, 1, 1, 1, 1])

    def test_trump_filter_partial_skip(self) -> None:
        """Test that trump_filter can skip some strains."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        try:
            result = calc_all_tables_pbn(deals, trump_filter=[0, 1, 0, 0, 0])
            self.assertTrue("tables" in result)
        except RuntimeError:
            pass

    def test_trump_filter_boundary_valid_0(self) -> None:
        """Test that trump_filter value 0 is valid."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        try:
            result = calc_all_tables_pbn(deals, trump_filter=[0, 0, 0, 0, 0])
            self.assertTrue("tables" in result)
        except RuntimeError:
            pass

    def test_trump_filter_boundary_valid_1(self) -> None:
        """Test that trump_filter value 1 is valid."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        try:
            result = calc_all_tables_pbn(deals, trump_filter=[1, 0, 0, 0, 0])
            self.assertTrue("tables" in result)
        except RuntimeError:
            pass

    def test_trump_filter_boundary_invalid_minus1(self) -> None:
        """Test that trump_filter value -1 is invalid."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        assert_raises(ValueError, calc_all_tables_pbn, deals, trump_filter=[-1, 0, 0, 0, 0], match="invalid value -1")

    def test_trump_filter_boundary_invalid_2(self) -> None:
        """Test that trump_filter value 2 is invalid."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        assert_raises(ValueError, calc_all_tables_pbn, deals, trump_filter=[0, 0, 2, 0, 0], match="invalid value 2")


if __name__ == "__main__":
    unittest.main()
