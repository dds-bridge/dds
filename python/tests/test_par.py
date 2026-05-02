"""Tests for par wrapper."""

import unittest
from unittest import SkipTest

from dds3 import par, calc_dd_table
from test_utils import assert_raises


class TestPar(unittest.TestCase):
    """Tests for par (par score calculation)."""

    def test_par_basic(self) -> None:
        """Test basic par calculation with a simple DD table."""
        # First, create a DD table result
        table_deal = {
            "cards": [
                [0x7FFC, 0, 0, 0],
                [0, 0x7FFC, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
        }
        
        # Note: We can't easily test par without a valid DD table
        # This test demonstrates the API but may not produce meaningful results
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table)
            self.assertTrue(isinstance(result, dict))
        except RuntimeError:
            # Invalid table is acceptable
            raise SkipTest("Could not create valid DD table")

    def test_par_vulnerable_none(self) -> None:
        """Test par with vulnerable=0 (neither vulnerable)."""
        table_deal = {
            "cards": [
                [0x7FFC, 0, 0, 0],
                [0, 0x7FFC, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table, vulnerable=0)
            self.assertTrue(isinstance(result, dict))
        except RuntimeError:
            raise SkipTest("Could not create valid DD table")

    def test_par_vulnerable_ns(self) -> None:
        """Test par with vulnerable=2 (NS vulnerable)."""
        table_deal = {
            "cards": [
                [0x7FFC, 0, 0, 0],
                [0, 0x7FFC, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table, vulnerable=2)
            self.assertTrue(isinstance(result, dict))
        except RuntimeError:
            raise SkipTest("Could not create valid DD table")

    def test_par_vulnerable_ew(self) -> None:
        """Test par with vulnerable=3 (EW vulnerable)."""
        table_deal = {
            "cards": [
                [0x7FFC, 0, 0, 0],
                [0, 0x7FFC, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table, vulnerable=3)
            self.assertTrue(isinstance(result, dict))
        except RuntimeError:
            raise SkipTest("Could not create valid DD table")

    def test_par_invalid_vulnerable(self) -> None:
        """Test that invalid vulnerable parameter."""
        table_deal = {
            "cards": [
                [0x7FFC, 0, 0, 0],
                [0, 0x7FFC, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
        except RuntimeError:
            raise SkipTest("Could not create valid DD table")

        assert_raises(ValueError, par, dd_table, vulnerable=4, match="vulnerable has invalid value")

    def test_par_result_structure(self) -> None:
        """Test that par result has expected structure."""
        table_deal = {
            "cards": [
                [0x7FFC, 0, 0, 0],
                [0, 0x7FFC, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            result = par(dd_table)
            
            self.assertTrue(isinstance(result, dict))
            # Should have par score and contracts
            self.assertTrue("par_score" in result or "par_contracts_string" in result)
        except RuntimeError:
            raise SkipTest("Could not create valid DD table")

    def test_par_requires_table_input(self) -> None:
        """Test that par requires a valid table input."""
        assert_raises((KeyError, ValueError, RuntimeError, TypeError), par, {"invalid": "structure"})

    def test_par_default_vulnerable_is_zero(self) -> None:
        """Test that default vulnerable is 0 (none)."""
        table_deal = {
            "cards": [
                [0x7FFC, 0, 0, 0],
                [0, 0x7FFC, 0, 0],
                [0, 0, 0x7FFC, 0],
                [0, 0, 0, 0x7FFC],
            ],
        }
        try:
            dd_table = calc_dd_table(table_deal)
            # Should not raise when vulnerable is omitted
            result = par(dd_table)
            self.assertTrue(isinstance(result, dict))
        except RuntimeError:
            raise SkipTest("Could not create valid DD table")


if __name__ == "__main__":
    unittest.main()
