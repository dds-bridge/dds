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

    def test_malformed_deal_in_batch_raises_not_crashes(self) -> None:
        """A malformed deal (a hand without 13 cards) must raise cleanly, even
        inside a multi-deal batch. Regression: such a deal previously reached
        the multi-threaded solver and segfaulted the process (intermittently,
        depending on batch size) instead of returning RETURN_CARD_COUNT (-14).
        """
        valid = "N:976.2.QJ74.JT985 83.JT53.K.AQ7643 QJ4.AQ984.T65.K2 AKT52.K76.A9832.-"
        # North holds only 12 cards (empty clubs): A5.QJ953.KQ962.(void)
        malformed = "N:A5.QJ953.KQ962. KJT8.K74..AJ62 632.A6.T74.KQ953 Q974.T82.AJ5.T74"

        # Solo: already rejected by the single-board card-count check.
        with self.assertRaises(RuntimeError):
            calc_all_tables_pbn([malformed])

        # In a batch: must also raise, not crash the interpreter.
        with self.assertRaises(RuntimeError):
            calc_all_tables_pbn([valid] * 20 + [malformed])


if __name__ == "__main__":
    unittest.main()
