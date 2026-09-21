"""Tests for the Deal converters, called directly.

dict_to_deal is covered indirectly by the solve-API tests; deal_to_dict has
no caller in dds3 at all, because it exists for extensions built outside
this repository. Both are driven here through _converters_fixture, a
test-only pybind module, so the dict shape they agree on is pinned by
assertions rather than by whatever the solver happens to accept.
"""

import unittest

import _converters_fixture

EXPECTED_KEYS = {
    "trump",
    "first",
    "current_trick_suit",
    "current_trick_rank",
    "remain_cards",
}

# Mirrors reference_deal() in converters_fixture.cpp.
REFERENCE_REMAIN_CARDS = [
    [(hand * 4 + suit + 1) * 4 for suit in range(4)] for hand in range(4)
]


class TestDealToDict(unittest.TestCase):
    """deal_to_dict() on a Deal that never went through dict_to_deal."""

    def setUp(self) -> None:
        self.result = _converters_fixture.reference_deal_as_dict()

    def test_produces_exactly_the_documented_keys(self) -> None:
        """No missing key, and no extra key a consumer would have to ignore."""
        self.assertEqual(set(self.result.keys()), EXPECTED_KEYS)

    def test_scalar_fields_carry_their_values(self) -> None:
        """trump and first are not swapped, and survive as ints."""
        self.assertEqual(self.result["trump"], 2)
        self.assertEqual(self.result["first"], 3)

    def test_current_trick_is_two_three_tuples(self) -> None:
        """Both trick fields are 3-tuples, in trick order, not reversed."""
        self.assertIsInstance(self.result["current_trick_suit"], tuple)
        self.assertIsInstance(self.result["current_trick_rank"], tuple)
        self.assertEqual(self.result["current_trick_suit"], (0, 1, 2))
        self.assertEqual(self.result["current_trick_rank"], (14, 13, 12))

    def test_remain_cards_is_four_rows_of_four(self) -> None:
        """A list of lists, not tuples, and 4x4 rather than flattened."""
        remain_cards = self.result["remain_cards"]
        self.assertIsInstance(remain_cards, list)
        self.assertEqual(len(remain_cards), 4)
        for row in remain_cards:
            self.assertIsInstance(row, list)
            self.assertEqual(len(row), 4)

    def test_remain_cards_is_indexed_hand_then_suit(self) -> None:
        """The values are distinct per cell, so a transpose would show here."""
        self.assertEqual(self.result["remain_cards"], REFERENCE_REMAIN_CARDS)


class TestRoundTrip(unittest.TestCase):
    """deal_to_dict(dict_to_deal(d)) against the dict a caller supplied."""

    def setUp(self) -> None:
        self.deal = {
            "trump": 1,
            "first": 2,
            "current_trick_suit": (3, 0, 1),
            "current_trick_rank": (14, 2, 0),
            "remain_cards": [
                [0x7FFC, 0x0004, 0x0008, 0x0010],
                [0x0020, 0x0040, 0x0080, 0x0100],
                [0x0200, 0x0400, 0x0800, 0x1000],
                [0x2000, 0x4000, 0x0000, 0x000C],
            ],
        }

    def test_round_trip_preserves_every_field(self) -> None:
        """Nothing is dropped, defaulted or reordered on the way through."""
        result = _converters_fixture.round_trip(self.deal)
        self.assertEqual(result["trump"], self.deal["trump"])
        self.assertEqual(result["first"], self.deal["first"])
        self.assertEqual(result["current_trick_suit"], self.deal["current_trick_suit"])
        self.assertEqual(result["current_trick_rank"], self.deal["current_trick_rank"])
        self.assertEqual(result["remain_cards"], self.deal["remain_cards"])

    def test_round_trip_accepts_lists_and_still_emits_tuples(self) -> None:
        """dict_to_deal takes any sequence; deal_to_dict's output shape is fixed."""
        deal = dict(self.deal)
        deal["current_trick_suit"] = list(deal["current_trick_suit"])
        deal["current_trick_rank"] = list(deal["current_trick_rank"])

        result = _converters_fixture.round_trip(deal)

        self.assertIsInstance(result["current_trick_suit"], tuple)
        self.assertEqual(result["current_trick_suit"], tuple(deal["current_trick_suit"]))
        self.assertEqual(result["current_trick_rank"], tuple(deal["current_trick_rank"]))

    def test_round_trip_output_is_accepted_again_unchanged(self) -> None:
        """The output is a legal input, so a caller can feed it straight back."""
        once = _converters_fixture.round_trip(self.deal)
        twice = _converters_fixture.round_trip(once)
        self.assertEqual(once, twice)

    def test_zero_deal_round_trips(self) -> None:
        """The all-zero edge case, which is what Deal{} leaves behind."""
        deal = {
            "trump": 0,
            "first": 0,
            "current_trick_suit": (0, 0, 0),
            "current_trick_rank": (0, 0, 0),
            "remain_cards": [[0, 0, 0, 0]] * 4,
        }
        result = _converters_fixture.round_trip(deal)
        self.assertEqual(result["remain_cards"], [[0, 0, 0, 0]] * 4)
        self.assertEqual(result["current_trick_suit"], (0, 0, 0))


if __name__ == "__main__":
    unittest.main()
