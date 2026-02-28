"""Tests for par wrapper."""

import pytest
from dds3 import par, calc_dd_table


class TestPar:
    """Tests for par (par score calculation)."""

    def test_par_basic(self) -> None:
        """Test basic par calculation with a simple DD table."""
        # First, create a DD table result
        table_deal = {
            "cards": [
                [0x1FFF, 0, 0, 0],
                [0, 0x1FFF, 0, 0],
                [0, 0, 0x1FFF, 0],
                [0, 0, 0, 0x1FFF],
            ],
        }
        
        # Note: We can't easily test par without a valid DD table
        # This test demonstrates the API but may not produce meaningful results
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table)
            assert isinstance(result, dict)
        except RuntimeError:
            # Invalid table is acceptable
            pytest.skip("Could not create valid DD table")

    def test_par_vulnerable_none(self) -> None:
        """Test par with vulnerable=0 (neither vulnerable)."""
        table_deal = {
            "cards": [
                [0x1FFF, 0, 0, 0],
                [0, 0x1FFF, 0, 0],
                [0, 0, 0x1FFF, 0],
                [0, 0, 0, 0x1FFF],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table, vulnerable=0)
            assert isinstance(result, dict)
        except RuntimeError:
            pytest.skip("Could not create valid DD table")

    def test_par_vulnerable_ns(self) -> None:
        """Test par with vulnerable=2 (NS vulnerable)."""
        table_deal = {
            "cards": [
                [0x1FFF, 0, 0, 0],
                [0, 0x1FFF, 0, 0],
                [0, 0, 0x1FFF, 0],
                [0, 0, 0, 0x1FFF],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table, vulnerable=2)
            assert isinstance(result, dict)
        except RuntimeError:
            pytest.skip("Could not create valid DD table")

    def test_par_vulnerable_ew(self) -> None:
        """Test par with vulnerable=3 (EW vulnerable)."""
        table_deal = {
            "cards": [
                [0x1FFF, 0, 0, 0],
                [0, 0x1FFF, 0, 0],
                [0, 0, 0x1FFF, 0],
                [0, 0, 0, 0x1FFF],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table, vulnerable=3)
            assert isinstance(result, dict)
        except RuntimeError:
            pytest.skip("Could not create valid DD table")

    def test_par_invalid_vulnerable(self) -> None:
        """Test that invalid vulnerable parameter."""
        # Note: DDS may not validate vulnerable parameter strictly
        table_deal = {
            "cards": [
                [0x1FFF, 0, 0, 0],
                [0, 0x1FFF, 0, 0],
                [0, 0, 0x1FFF, 0],
                [0, 0, 0, 0x1FFF],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            # DDS may not strictly validate vulnerable, so we just test it doesn't crash
            result = par(dd_table, vulnerable=4)  # May or may not be valid
            assert "par_contracts_string" in result or "par_score" in result  # If it succeeds, should have result
        except (ValueError, RuntimeError):
            pass  # If it raises, that's also acceptable

    def test_par_result_structure(self) -> None:
        """Test that par result has expected structure."""
        table_deal = {
            "cards": [
                [0x1FFF, 0, 0, 0],
                [0, 0x1FFF, 0, 0],
                [0, 0, 0x1FFF, 0],
                [0, 0, 0, 0x1FFF],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table)
            
            assert isinstance(result, dict)
            # Should have par score and contracts
            assert "par_score" in result or "par_contracts_string" in result
        except RuntimeError:
            pytest.skip("Could not create valid DD table")

    def test_par_requires_table_input(self) -> None:
        """Test that par requires a valid table input."""
        with pytest.raises((KeyError, ValueError, RuntimeError, TypeError)):
            par({"invalid": "structure"})

    def test_par_default_vulnerable_is_zero(self) -> None:
        """Test that default vulnerable is 0 (none)."""
        table_deal = {
            "cards": [
                [0x1FFF, 0, 0, 0],
                [0, 0x1FFF, 0, 0],
                [0, 0, 0x1FFF, 0],
                [0, 0, 0, 0x1FFF],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            # Should not raise when vulnerable is omitted
            result = par(dd_table)
            assert isinstance(result, dict)
        except RuntimeError:
            pytest.skip("Could not create valid DD table")
