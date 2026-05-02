"""Simple smoke tests for calc_par functions."""

import unittest

from dds3 import calc_all_tables_pbn
from dds3 import calc_par_from_table


class TestCalcParSimple(unittest.TestCase):
    def _get_table_result(self) -> dict:
        pbn_deals = ["N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"]
        tables_result = calc_all_tables_pbn(pbn_deals, mode=-1)
        self.assertIn("tables", tables_result, "calc_all_tables_pbn should return tables")
        self.assertGreater(len(tables_result["tables"]), 0, "Should have at least one table")
        return tables_result["tables"][0]

    def test_calc_par_basic(self) -> None:
        """Basic smoke test for calc_par_from_table using PBN hand."""
        table_result = self._get_table_result()
        par_result = calc_par_from_table(table_result, vulnerable=0)
        self.assertIsInstance(par_result, dict)
        self.assertIn("par_score", par_result)
        self.assertIn("par_contracts_string", par_result)

    def test_calc_par_vulnerabilities(self) -> None:
        """Test calc_par_from_table with different vulnerabilities."""
        table_result = self._get_table_result()

        for vuln in [0, 1, 2, 3]:
            par_result = calc_par_from_table(table_result, vulnerable=vuln)
            self.assertIsInstance(par_result, dict)
            self.assertIn("par_score", par_result)

    def test_calc_par_from_table_basic(self) -> None:
        """Basic smoke test for calc_par_from_table."""
        table_result = self._get_table_result()
        par_result = calc_par_from_table(table_result, vulnerable=0)
        self.assertIsInstance(par_result, dict)
        self.assertIn("par_score", par_result)
        self.assertIn("par_contracts_string", par_result)

    def test_calc_par_consistency(self) -> None:
        """Test that multiple calls with same input give consistent results."""
        table_result = self._get_table_result()
        par_result1 = calc_par_from_table(table_result, vulnerable=0)
        par_result2 = calc_par_from_table(table_result, vulnerable=0)
        self.assertEqual(par_result1["par_score"], par_result2["par_score"])


if __name__ == "__main__":
    unittest.main()
