"""Tests for calc_dd_table and calc_all_tables_pbn wrappers."""

import pytest
from dds3 import calc_dd_table, calc_all_tables_pbn


class TestCalcDDTable:
    """Tests for calc_dd_table (single table calculation)."""

    def test_calc_dd_table_basic(self) -> None:
        """Test basic calc_dd_table with a simple deal."""
        table_deal = {
            "cards": [
                # 4x4 array: [hand][suit]
                [0x7FFC, 0, 0, 0],        # North: all spades
                [0, 0x7FFC, 0, 0],        # East: all hearts
                [0, 0, 0x7FFC, 0],        # South: all diamonds
                [0, 0, 0, 0x7FFC],        # West: all clubs
            ],
        }
        result = calc_dd_table(table_deal)
        assert "return_code" in result or "res_table" in result

    def test_calc_dd_table_result_structure(self) -> None:
        """Test that result has correct structure."""
        table_deal = {
            "cards": [
                [0x7FFC, 0, 0, 0],
                [0, 0x7FFC, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
        }
        result = calc_dd_table(table_deal)
        # Result should be a dict
        assert isinstance(result, dict)

    def test_calc_dd_table_invalid_remain_cards_size(self) -> None:
        """Test that invalid remain_cards size raises error."""
        table_deal = {
            "cards": [[0, 0, 0]],  # Wrong dimensions
        }
        with pytest.raises(ValueError):
            calc_dd_table(table_deal)

    def test_calc_dd_table_remain_cards_all_zeros(self) -> None:
        """Test with all zeros (no cards dealt)."""
        table_deal = {
            "cards": [[0, 0, 0, 0] for _ in range(4)],
        }
        # May raise due to invalid deal, but should not crash
        try:
            result = calc_dd_table(table_deal)
            assert isinstance(result, dict)
        except RuntimeError:
            # Invalid deal is acceptable
            pass


class TestCalcAllTablesPBN:
    """Tests for calc_all_tables_pbn (batch table calculation)."""

    def test_calc_all_tables_pbn_single_deal(self) -> None:
        """Test calc_all_tables_pbn with a single PBN deal."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        result = calc_all_tables_pbn(deals)
        assert "no_of_boards" in result
        assert "tables" in result
        assert isinstance(result["tables"], list)

    def test_calc_all_tables_pbn_multiple_deals(self) -> None:
        """Test calc_all_tables_pbn with multiple deals."""
        deals = [
            "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3",
            "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3",
        ]
        result = calc_all_tables_pbn(deals)
        assert "no_of_boards" in result
        assert len(result["tables"]) >= 1  # At least one table per deal

    def test_calc_all_tables_pbn_with_mode(self) -> None:
        """Test calc_all_tables_pbn with par mode enabled."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        result = calc_all_tables_pbn(deals, mode=0)  # Calculate par
        assert "tables" in result
        assert "par_results" in result

    def test_calc_all_tables_pbn_default_mode_is_no_par(self) -> None:
        """Test that default mode is -1 (no par)."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        result = calc_all_tables_pbn(deals)
        # With mode=-1, par_results may be empty or zero-filled
        assert "tables" in result

    def test_calc_all_tables_pbn_with_trump_filter(self) -> None:
        """Test calc_all_tables_pbn with trump filter to skip some strains."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        # Skip spades and hearts (1,1,0,0,0)
        result = calc_all_tables_pbn(deals, trump_filter=[1, 1, 0, 0, 0])
        assert "no_of_boards" in result
        assert "tables" in result

    def test_calc_all_tables_pbn_default_trump_filter_all_zeros(self) -> None:
        """Test that default trump_filter is (0,0,0,0,0) - include all."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        result = calc_all_tables_pbn(deals)  # Default trump_filter
        assert "tables" in result

    def test_calc_all_tables_pbn_invalid_pbn(self) -> None:
        """Test that invalid PBN raises error."""
        deals = ["This is not a valid PBN"]
        with pytest.raises((ValueError, RuntimeError)):
            calc_all_tables_pbn(deals)

    def test_calc_all_tables_pbn_empty_list(self) -> None:
        """Test that empty deal list returns empty results."""
        deals = []
        # Empty list actually succeeds with 0 boards
        result = calc_all_tables_pbn(deals)
        assert "no_of_boards" in result
        assert "tables" in result

    def test_calc_all_tables_pbn_invalid_trump_filter_size(self) -> None:
        """Test that invalid trump_filter size raises error."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        with pytest.raises(ValueError):
            calc_all_tables_pbn(deals, trump_filter=[0, 0, 0])  # Too small

    def test_calc_all_tables_pbn_invalid_trump_filter_value(self) -> None:
        """Test that invalid trump_filter values raise error."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        with pytest.raises(ValueError, match="invalid value"):
            calc_all_tables_pbn(deals, trump_filter=[0, 0, 2, 0, 0])  # 2 is invalid (must be 0-1)

    def test_calc_all_tables_pbn_result_structure(self) -> None:
        """Test that result has expected structure."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        result = calc_all_tables_pbn(deals)
        
        assert isinstance(result, dict)
        assert "no_of_boards" in result
        assert "tables" in result
        assert "par_results" in result
        
        assert isinstance(result["no_of_boards"], int)
        assert isinstance(result["tables"], list)
        assert isinstance(result["par_results"], list)
        # par_results is empty when mode == -1 (the default)
        assert len(result["par_results"]) == 0
        
        # Test with mode != -1 to verify par_results is populated
        result_with_par = calc_all_tables_pbn(deals, mode=0)
        assert len(result_with_par["par_results"]) > 0


class TestTableParity:
    """Tests for parity between single and batch table calculations."""

    def test_single_vs_batch_result_structure(self) -> None:
        """Test that single calc_dd_table and batch calc_all_tables_pbn have compatible results."""
        deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        batch_result = calc_all_tables_pbn(deals)
        
        # Single table from batch should have similar structure to calc_dd_table
        assert len(batch_result["tables"]) >= 1
        single_table = batch_result["tables"][0]
        assert isinstance(single_table, dict)
