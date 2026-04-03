"""Bazel-native regression tests for calc_all_tables_pbn table row semantics."""

import unittest

from dds3 import calc_all_tables_pbn


class CalcAllTablesPbnRegressionTest(unittest.TestCase):
    def test_nt_row_index_regression(self) -> None:
        """Keep a small Bazel-native regression check for the NT row index contract."""
        deals = ["N:Q87.K932.QJT32.7 AKJ9632.J84.6.Q5 .AQT765.K87.J962 T54..A954.AKT843"]

        all_rows = calc_all_tables_pbn(
            deals, trump_filter=[0, 0, 0, 0, 0]
        )["tables"][0]["res_table"]
        nt_only_rows = calc_all_tables_pbn(
            deals, trump_filter=[1, 1, 1, 1, 0]
        )["tables"][0]["res_table"]

        self.assertEqual(len(all_rows), 5)
        self.assertEqual(len(nt_only_rows), 5)
        self.assertEqual(nt_only_rows[4], all_rows[4])
if __name__ == "__main__":
    unittest.main()
