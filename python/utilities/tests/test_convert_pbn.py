"""Tests for PBN-to-bitmask conversion in dd_table_for_deal."""

import unittest

from dd_table_for_deal import _convert_pbn

_EMPTY_REMAIN = [[0] * 4 for _ in range(4)]

_EXAMPLE_DEAL = (
    "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
    "5.A95432.7632.K6 AKJ9842.K.T8.J93"
)


class ConvertPbnTest(unittest.TestCase):
    def test_short_deal_strings_return_empty_remain(self) -> None:
        """Short or malformed PBN strings must not raise IndexError."""
        for deal in ("", "N", "N:", "12", "abc"):
            with self.subTest(deal=deal):
                self.assertEqual(_convert_pbn(deal), _EMPTY_REMAIN)

    def test_valid_deal_parses_card_bitmasks(self) -> None:
        remain = _convert_pbn(_EXAMPLE_DEAL)

        # North's spades: 73
        self.assertEqual(remain[0][0], 0x0080 | 0x0008)
        # North's hearts: QJT
        self.assertEqual(remain[0][1], 0x1000 | 0x0800 | 0x0400)


if __name__ == "__main__":
    unittest.main()
