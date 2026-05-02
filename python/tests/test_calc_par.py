"""Tests for calc_par and calc_par_from_table wrappers."""

import unittest

from dds3 import calc_par, calc_par_from_table, calc_dd_table

# Rank bitmask constants (matching C++ test data)
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


class TestCalcPar(unittest.TestCase):
    """Tests for calc_par (single call for DD table + par)."""

    def _hand0_table_deal(self) -> dict:
        """Test hand 0 from examples/hands.cpp."""
        return {
            "cards": [
                # North: [Spades, Hearts, Diamonds, Clubs]
                [RQ|RJ|R6,   RK|R6|R5|R2, RJ|R8|R5,   RT|R9|R8],
                # East: [Spades, Hearts, Diamonds, Clubs]
                [R8|R7|R3,   RJ|R9|R7,    RA|RT|R7|R6|R4, RQ|R4],
                # South: [Spades, Hearts, Diamonds, Clubs]
                [RK|R5,      RT|R8|R3,    RK|RQ|R9,   RA|R7|R6|R5|R2],
                # West: [Spades, Hearts, Diamonds, Clubs]
                [RA|RT|R9|R4|R2, RA|RQ|R4, R3|R2,     RK|RJ|R3],
            ]
        }

    def _hand1_table_deal(self) -> dict:
        """Test hand 1 from examples/hands.cpp."""
        return {
            "cards": [
                # North: [Spades, Hearts, Diamonds, Clubs]
                [RA|RK|R9|R6,      RK|RQ|R8,         RA|R9|R8,         RK|R6|R3],
                # East: [Spades, Hearts, Diamonds, Clubs]
                [RQ|RJ|RT|R5|R4|R3|R2, RT,           R6,               RQ|RJ|R8|R2],
                # South: [Spades, Hearts, Diamonds, Clubs]
                [0,                RJ|R9|R7|R5|R4|R3, RK|R7|R5|R3|R2, R9|R4],
                # West: [Spades, Hearts, Diamonds, Clubs]
                [R8|R7,            RA|R6|R2,         RQ|RJ|RT|R4,      RA|RT|R7|R5],
            ]
        }

    def test_calc_par_basic_hand0(self) -> None:
        """Test calc_par with hand 0 (vulnerable=0)."""
        table_deal = self._hand0_table_deal()
        result = calc_par(table_deal, vulnerable=0)
        
        # Result should have dd_table and par_results keys
        self.assertTrue(isinstance(result, dict))
        self.assertTrue("dd_table" in result)
        self.assertTrue("par_results" in result)
        
        # DD table should have res_table
        self.assertTrue("res_table" in result["dd_table"])
        self.assertTrue(isinstance(result["dd_table"]["res_table"], list))
        
        # Par results should have par_score and par_contracts_string
        self.assertTrue("par_score" in result["par_results"])
        self.assertTrue("par_contracts_string" in result["par_results"])

    def test_calc_par_vulnerability_variations(self) -> None:
        """Test calc_par with different vulnerability settings."""
        table_deal = self._hand0_table_deal()
        
        for vuln in [0, 1, 2, 3]:
            result = calc_par(table_deal, vulnerable=vuln)
            self.assertTrue(isinstance(result, dict))
            self.assertTrue("dd_table" in result)
            self.assertTrue("par_results" in result)

    def test_calc_par_invalid_vulnerability(self) -> None:
        """Test that invalid vulnerability raises ValueError."""
        table_deal = self._hand0_table_deal()
        
        try:
            calc_par(table_deal, vulnerable=-1)
            self.fail("Expected ValueError for vulnerable=-1")
        except ValueError:
            pass  # Expected

        try:
            calc_par(table_deal, vulnerable=4)
            self.fail("Expected ValueError for vulnerable=4")
        except ValueError:
            pass  # Expected

    def test_calc_par_hand1(self) -> None:
        """Test calc_par with hand 1."""
        table_deal = self._hand1_table_deal()
        result = calc_par(table_deal, vulnerable=2)  # NS vulnerable
        
        self.assertTrue(isinstance(result, dict))
        self.assertTrue("dd_table" in result)
        self.assertTrue("par_results" in result)
        self.assertTrue("res_table" in result["dd_table"])

    def test_calc_par_result_consistency(self) -> None:
        """Test that repeated calls with same input give same results."""
        table_deal = self._hand0_table_deal()
        
        result1 = calc_par(table_deal, vulnerable=0)
        result2 = calc_par(table_deal, vulnerable=0)
        
        # Both should have same structure
        self.assertTrue(result1["par_results"]["par_score"] == result2["par_results"]["par_score"])


class TestCalcParFromTable(unittest.TestCase):
    """Tests for calc_par_from_table (par from pre-computed DD table)."""

    def _hand0_table_deal(self) -> dict:
        """Test hand 0 from examples/hands.cpp."""
        return {
            "cards": [
                # North: [Spades, Hearts, Diamonds, Clubs]
                [RQ|RJ|R6,   RK|R6|R5|R2, RJ|R8|R5,   RT|R9|R8],
                # East: [Spades, Hearts, Diamonds, Clubs]
                [R8|R7|R3,   RJ|R9|R7,    RA|RT|R7|R6|R4, RQ|R4],
                # South: [Spades, Hearts, Diamonds, Clubs]
                [RK|R5,      RT|R8|R3,    RK|RQ|R9,   RA|R7|R6|R5|R2],
                # West: [Spades, Hearts, Diamonds, Clubs]
                [RA|RT|R9|R4|R2, RA|RQ|R4, R3|R2,     RK|RJ|R3],
            ]
        }

    def test_calc_par_from_table_basic(self) -> None:
        """Test calc_par_from_table with pre-computed DD table."""
        # First compute DD table
        table_deal = self._hand0_table_deal()
        dd_result = calc_dd_table(table_deal)
        
        # Then compute par from that table
        par_result = calc_par_from_table(dd_result, vulnerable=0)
        
        # Result should have par_score and par_contracts_string
        self.assertTrue(isinstance(par_result, dict))
        self.assertTrue("par_score" in par_result)
        self.assertTrue("par_contracts_string" in par_result)

    def test_calc_par_from_table_vulnerability_variations(self) -> None:
        """Test calc_par_from_table with different vulnerability."""
        table_deal = self._hand0_table_deal()
        dd_result = calc_dd_table(table_deal)
        
        # Test different vulnerability settings
        for vuln in [0, 1, 2, 3]:
            par_result = calc_par_from_table(dd_result, vulnerable=vuln)
            self.assertTrue(isinstance(par_result, dict))
            self.assertTrue("par_score" in par_result)

    def test_calc_par_from_table_invalid_vulnerability(self) -> None:
        """Test that invalid vulnerability raises ValueError."""
        table_deal = self._hand0_table_deal()
        dd_result = calc_dd_table(table_deal)
        
        try:
            calc_par_from_table(dd_result, vulnerable=-1)
            self.fail("Expected ValueError for vulnerable=-1")
        except ValueError:
            pass  # Expected

        try:
            calc_par_from_table(dd_result, vulnerable=4)
            self.fail("Expected ValueError for vulnerable=4")
        except ValueError:
            pass  # Expected


class TestCalcParIntegration(unittest.TestCase):
    """Integration tests comparing calc_par and calc_par_from_table."""

    def _hand0_table_deal(self) -> dict:
        """Test hand 0 from examples/hands.cpp."""
        return {
            "cards": [
                # North: [Spades, Hearts, Diamonds, Clubs]
                [RQ|RJ|R6,   RK|R6|R5|R2, RJ|R8|R5,   RT|R9|R8],
                # East: [Spades, Hearts, Diamonds, Clubs]
                [R8|R7|R3,   RJ|R9|R7,    RA|RT|R7|R6|R4, RQ|R4],
                # South: [Spades, Hearts, Diamonds, Clubs]
                [RK|R5,      RT|R8|R3,    RK|RQ|R9,   RA|R7|R6|R5|R2],
                # West: [Spades, Hearts, Diamonds, Clubs]
                [RA|RT|R9|R4|R2, RA|RQ|R4, R3|R2,     RK|RJ|R3],
            ]
        }

    def test_calc_par_vs_from_table_consistency(self) -> None:
        """Test that calc_par and calc_par_from_table give same par results."""
        table_deal = self._hand0_table_deal()
        
        # Method 1: calc_par (combined DD table + par)
        cp_result = calc_par(table_deal, vulnerable=0)
        
        # Method 2: calc_par_from_table (par from DD table)
        dd_result = calc_dd_table(table_deal)
        cfp_result = calc_par_from_table(dd_result, vulnerable=0)
        
        # Par scores should match
        self.assertTrue(cp_result["par_results"]["par_score"] == cfp_result["par_score"])
        
        # Par contract strings should match
        self.assertTrue(cp_result["par_results"]["par_contracts_string"] == cfp_result["par_contracts_string"])

    def test_calc_par_dd_table_matches_calc_dd_table(self) -> None:
        """Test that calc_par's DD table matches calc_dd_table result."""
        table_deal = self._hand0_table_deal()
        
        # Method 1: calc_par (gets DD table as side effect)
        cp_result = calc_par(table_deal, vulnerable=0)
        
        # Method 2: direct calc_dd_table
        dd_result = calc_dd_table(table_deal)
        
        # DD tables should match
        self.assertTrue(cp_result["dd_table"]["res_table"] == dd_result["res_table"])

    def test_calc_par_multiple_vulnerabilities(self) -> None:
        """Test calc_par and calc_par_from_table with multiple vulnerabilities."""
        table_deal = self._hand0_table_deal()
        dd_result = calc_dd_table(table_deal)
        
        for vuln in [0, 1, 2, 3]:
            # calc_par
            cp_result = calc_par(table_deal, vulnerable=vuln)
            
            # calc_par_from_table
            cfp_result = calc_par_from_table(dd_result, vulnerable=vuln)
            
            # Par results should match
            self.assertTrue(cp_result["par_results"]["par_score"] == cfp_result["par_score"])


if __name__ == "__main__":
    unittest.main()
