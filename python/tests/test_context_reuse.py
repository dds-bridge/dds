"""Tests for context reuse with solve_board and calc_par (no pytest required)."""

from dds3 import (
    SolverContext,
    solve_board,
    solve_board_pbn,
    calc_par,
)


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


def test_create_solver_context() -> None:
    """Test that SolverContext can be created."""
    ctx = SolverContext()
    assert ctx is not None
    assert isinstance(ctx, SolverContext)
    print("✓ test_create_solver_context passed")


def test_multiple_contexts() -> None:
    """Test that multiple SolverContext instances can be created."""
    ctx1 = SolverContext()
    ctx2 = SolverContext()
    assert ctx1 is not None
    assert ctx2 is not None
    assert ctx1 is not ctx2  # Different instances
    print("✓ test_multiple_contexts passed")


def test_solve_board_without_context() -> None:
    """Test solve_board works without context (backward compatibility)."""
    deal = {
        "trump": 0,  # Spades
        "first": 0,  # North
        "remain_cards": [
            [0x7FFC, 0, 0, 0],        # North: all spades
            [0, 0x7FFC, 0, 0],        # East: all hearts
            [0, 0, 0x7FFC, 0],        # South: all diamonds
            [0, 0, 0, 0x7FFC],        # West: all clubs
        ],
        "current_trick_suit": (0, 0, 0),
        "current_trick_rank": (0, 0, 0),
    }
    result = solve_board(deal)
    assert "nodes" in result
    assert "score" in result
    print("✓ test_solve_board_without_context passed")


def test_solve_board_with_context() -> None:
    """Test solve_board works with context."""
    ctx = SolverContext()
    deal = {
        "trump": 0,  # Spades
        "first": 0,  # North
        "remain_cards": [
            [0x7FFC, 0, 0, 0],        # North: all spades
            [0, 0x7FFC, 0, 0],        # East: all hearts
            [0, 0, 0x7FFC, 0],        # South: all diamonds
            [0, 0, 0, 0x7FFC],        # West: all clubs
        ],
        "current_trick_suit": (0, 0, 0),
        "current_trick_rank": (0, 0, 0),
    }
    result = solve_board(deal, context=ctx)
    assert "nodes" in result
    assert "score" in result
    print("✓ test_solve_board_with_context passed")


def test_solve_board_context_reuse() -> None:
    """Test reusing same context for multiple solve_board calls."""
    ctx = SolverContext()
    deal1 = {
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
    deal2 = {
        "trump": 1,  # Hearts
        "first": 1,  # East
        "remain_cards": [
            [0, 0x7FFC, 0, 0],
            [0x7FFC, 0, 0, 0],
            [0, 0, 0x7FFC, 0],
            [0, 0, 0, 0x7FFC],
        ],
        "current_trick_suit": (0, 0, 0),
        "current_trick_rank": (0, 0, 0),
    }

    # First solve
    result1 = solve_board(deal1, context=ctx)
    assert "nodes" in result1
    assert "score" in result1

    # Second solve with same context
    result2 = solve_board(deal2, context=ctx)
    assert "nodes" in result2
    assert "score" in result2
    print("✓ test_solve_board_context_reuse passed")


def test_solve_board_pbn_with_context() -> None:
    """Test solve_board_pbn with context."""
    ctx = SolverContext()
    pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
    result = solve_board_pbn(pbn, trump=4, first=0, context=ctx)
    assert "nodes" in result
    assert "score" in result
    print("✓ test_solve_board_pbn_with_context passed")


def test_solve_board_pbn_without_context() -> None:
    """Test solve_board_pbn works without context (backward compatibility)."""
    pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
    result = solve_board_pbn(pbn, trump=4, first=0)
    assert "nodes" in result
    assert "score" in result
    print("✓ test_solve_board_pbn_without_context passed")


def test_calc_par_without_context() -> None:
    """Test calc_par works without explicit context (backward compatibility)."""
    # Use the test hands from examples/hands.cpp
    # Note: calc_par works on card distributions
    # cards[hand][suit] where suit order is [Spades, Hearts, Diamonds, Clubs]
    # North: QJ6.K652.J85.T98
    # East: 873.J97.AT764.Q4
    # South: K5.T83.KQ9.A7652
    # West: AT942.AQ4.32.KJ3
    table_deal = {
        "cards": [
            # North: [Spades, Hearts, Diamonds, Clubs]
            [RQ|RJ|R6, RK|R6|R5|R2, RJ|R8|R5, RT|R9|R8],
            # East: [Spades, Hearts, Diamonds, Clubs]
            [R8|R7|R3, RJ|R9|R7, RA|RT|R7|R6|R4, RQ|R4],
            # South: [Spades, Hearts, Diamonds, Clubs]
            [RK|R5, RT|R8|R3, RK|RQ|R9, RA|R7|R6|R5|R2],
            # West: [Spades, Hearts, Diamonds, Clubs]
            [RA|RT|R9|R4|R2, RA|RQ|R4, R3|R2, RK|RJ|R3],
        ]
    }
    result = calc_par(table_deal, vulnerable=0)
    assert isinstance(result, dict)
    assert "dd_table" in result
    assert "par_results" in result
    print("✓ test_calc_par_without_context passed")


def test_calc_par_results_consistency() -> None:
    """Test that repeated calc_par calls produce consistent results."""
    # cards[hand][suit] format
    table_deal = {
        "cards": [
            [RQ|RJ|R6, RK|R6|R5|R2, RJ|R8|R5, RT|R9|R8],
            [R8|R7|R3, RJ|R9|R7, RA|RT|R7|R6|R4, RQ|R4],
            [RK|R5, RT|R8|R3, RK|RQ|R9, RA|R7|R6|R5|R2],
            [RA|RT|R9|R4|R2, RA|RQ|R4, R3|R2, RK|RJ|R3],
        ]
    }

    result1 = calc_par(table_deal, vulnerable=0)
    result2 = calc_par(table_deal, vulnerable=0)

    # Results should be identical
    assert result1["par_results"]["par_score"] == result2["par_results"]["par_score"]
    print("✓ test_calc_par_results_consistency passed")


def run_all_tests() -> None:
    """Run all tests."""
    test_create_solver_context()
    test_multiple_contexts()
    test_solve_board_without_context()
    test_solve_board_with_context()
    test_solve_board_context_reuse()
    test_solve_board_pbn_with_context()
    test_solve_board_pbn_without_context()
    test_calc_par_without_context()
    test_calc_par_results_consistency()
    print("\n✓ All context reuse tests passed!")


if __name__ == "__main__":
    run_all_tests()
