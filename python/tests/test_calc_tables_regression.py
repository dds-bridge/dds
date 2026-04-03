"""Bazel-native regression tests for calc_all_tables_pbn table row semantics."""

import unittest

from dds3 import calc_all_tables_pbn


class CalcAllTablesPbnRegressionTest(unittest.TestCase):
    def test_nt_row_and_filter_semantics(self) -> None:
        """NT is row index 4 and filtered strains are returned as zero-filled rows."""
        deals = ["N:Q87.K932.QJT32.7 AKJ9632.J84.6.Q5 .AQT765.K87.J962 T54..A954.AKT843"]

        all_strains = calc_all_tables_pbn(deals, trump_filter=[0, 0, 0, 0, 0])
        all_rows = all_strains["tables"][0]["res_table"]

        nt_only = calc_all_tables_pbn(deals, trump_filter=[1, 1, 1, 1, 0])
        nt_only_rows = nt_only["tables"][0]["res_table"]

        exclude_nt = calc_all_tables_pbn(deals, trump_filter=[0, 0, 0, 0, 1])
        exclude_nt_rows = exclude_nt["tables"][0]["res_table"]

        self.assertEqual(len(all_rows), 5)
        self.assertEqual(len(nt_only_rows), 5)
        self.assertEqual(len(exclude_nt_rows), 5)

        self.assertEqual(nt_only_rows[4], all_rows[4])
        self.assertTrue(all(value == 0 for row in nt_only_rows[:4] for value in row))

        self.assertTrue(all(value == 0 for value in exclude_nt_rows[4]))
        for strain in range(4):
            self.assertEqual(exclude_nt_rows[strain], all_rows[strain])


if __name__ == "__main__":
    unittest.main()
