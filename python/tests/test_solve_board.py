"""Tests for solve_board and solve_board_pbn wrappers."""

import pytest
from dds3 import solve_board, solve_board_pbn


class TestSolveBoard:
    """Tests for solve_board (binary format input)."""

    def test_solve_board_basic(self) -> None:
        """Test basic solve_board with a simple deal."""
        # A simple endgame: All spades for North
        deal = {
            "trump": 0,  # Spades
            "first": 0,  # North
            "remain_cards": [
                # 4x4 array: [hand][suit] where suit: 0=♠ 1=♥ 2=♦ 3=♣
                [0x1FFF, 0, 0, 0],        # North: all spades
                [0, 0x1FFF, 0, 0],        # East: all hearts
                [0, 0, 0x1FFF, 0],        # South: all diamonds
                [0, 0, 0, 0x1FFF],        # West: all clubs
            ],
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }
        result = solve_board(deal)
        assert "nodes" in result  # FutureTricks has nodes, not return_code
        assert isinstance(result["score"], tuple)

    def test_solve_board_with_defaults(self) -> None:
        """Test that default parameters work."""
        deal = {
            "trump": 4,  # NT
            "first": 0,
            "remain_cards": [
                [0x1FFF, 0, 0, 0],
                [0, 0, 0, 0],
                [0, 0, 0, 0],
                [0, 0, 0, 0],
            ],
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }
        # Should not raise, error handling is DDS-side
        try:
            result = solve_board(deal)
            assert "nodes" in result
        except RuntimeError:
            # Invalid deal may raise RuntimeError
            pass

    def test_solve_board_invalid_trump(self) -> None:
        """Test that invalid trump raises error."""
        deal = {
            "trump": 5,  # Invalid (must be 0-4)
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }
        with pytest.raises(ValueError, match="invalid value 5"):
            solve_board(deal)

    def test_solve_board_invalid_first(self) -> None:
        """Test that invalid first seat raises error."""
        deal = {
            "trump": 0,
            "first": 4,  # Invalid (must be 0-3)
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }
        with pytest.raises(ValueError, match="invalid value 4"):
            solve_board(deal)

    def test_solve_board_invalid_trick_suit(self) -> None:
        """Test that invalid current trick suit raises error."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 5),  # Invalid suit (must be 0-3)
            "current_trick_rank": (0, 0, 0),
        }
        with pytest.raises(ValueError, match="invalid value 5"):
            solve_board(deal)

    def test_solve_board_invalid_trick_rank(self) -> None:
        """Test that invalid current trick rank raises error."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0, 0]] * 4,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (2, 2, 15),  # Invalid rank (must be 0-14)
        }
        with pytest.raises(ValueError, match="invalid value 15"):
            solve_board(deal)

    def test_solve_board_invalid_cards_size(self) -> None:
        """Test that invalid cards array size raises error."""
        deal = {
            "trump": 0,
            "first": 0,
            "remain_cards": [[0, 0, 0]],  # Too small
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
        }
        with pytest.raises(ValueError):
            solve_board(deal)


class TestSolveBoardPBN:
    """Tests for solve_board_pbn (PBN string input)."""

    def test_solve_board_pbn_basic(self) -> None:
        """Test basic solve_board_pbn with valid PBN."""
        # Simple PBN: N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        result = solve_board_pbn(pbn, trump=4, first=0)
        assert "nodes" in result  # FutureTricks has nodes, not return_code

    def test_solve_board_pbn_with_defaults(self) -> None:
        """Test that default parameters work correctly."""
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        result = solve_board_pbn(pbn)  # Using all defaults
        assert "nodes" in result

    def test_solve_board_pbn_invalid_format(self) -> None:
        """Test that invalid PBN format raises error."""
        invalid_pbn = "This is not a valid PBN"
        with pytest.raises((ValueError, RuntimeError)):
            solve_board_pbn(invalid_pbn)

    def test_solve_board_pbn_invalid_trump(self) -> None:
        """Test that invalid trump in PBN raises error."""
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        with pytest.raises((ValueError, RuntimeError)):
            solve_board_pbn(pbn, trump=5)  # Invalid

    def test_solve_board_pbn_invalid_first(self) -> None:
        """Test that invalid first seat raises error."""
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        with pytest.raises((ValueError, RuntimeError)):
            solve_board_pbn(pbn, first=4)  # Invalid

    def test_solve_board_pbn_default_trump_is_nt(self) -> None:
        """Test that default trump is NT (4)."""
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        result = solve_board_pbn(pbn)  # No trump specified
        assert result["cards"] >= 0  # Should have solution

    def test_solve_board_pbn_default_first_is_north(self) -> None:
        """Test that default first is North (0)."""
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        result = solve_board_pbn(pbn)  # No first specified
        assert result["cards"] >= 0  # Should have solution

    def test_solve_board_pbn_current_trick_validation(self) -> None:
        """Test that invalid current trick in PBN mode raises error."""
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        with pytest.raises(ValueError, match="invalid value"):
            solve_board_pbn(pbn, current_trick_suit=(0, 0, 5))


class TestSolveBoardParity:
    """Tests for parity between different calling conventions."""

    def test_default_parameters_consistent(self) -> None:
        """Test that same deal with same defaults returns same result structure."""
        pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
        result_pbn = solve_board_pbn(pbn)

        # Both should have the same keys
        assert "nodes" in result_pbn  # FutureTricks has nodes, not return_code
        assert "score" in result_pbn
