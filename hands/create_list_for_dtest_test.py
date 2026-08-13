#!/usr/bin/env python3
"""Unit tests for create_list_for_dtest."""

import contextlib
import io
import unittest
from pathlib import Path

import create_list_for_dtest as cld
from dds3 import analyse_play_pbn


# list1.txt deal 1.
_DEAL_CARDS = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"

_EXPECTED_TABLE = "TABLE 5 8 5 8 6 6 6 6 5 7 5 7 7 5 7 5 6 6 6 6 \n"
_EXPECTED_PAR = 'PAR "NS -110" "EW 110" "NS:EW 2S" "EW:EW 2S" \n'
_EXPECTED_PAR2 = 'PAR2 "-110" "2S-EW" \n'


class FormatLinesTest(unittest.TestCase):
    def test_format_table_line_suit_major_hand_minor(self):
        res_table = [
            [5, 8, 5, 8],
            [6, 6, 6, 6],
            [5, 7, 5, 7],
            [7, 5, 7, 5],
            [6, 6, 6, 6],
        ]
        self.assertEqual(cld.format_table_line(res_table), _EXPECTED_TABLE)

    def test_format_par_line_quotes_scores_and_contracts(self):
        par = {
            "par_score": ["NS -110", "EW 110"],
            "par_contracts_string": ["NS:EW 2S", "EW:EW 2S"],
        }
        self.assertEqual(cld.format_par_line(par), _EXPECTED_PAR)

    def test_format_par2_line_quotes_score_and_each_contract(self):
        dealer_par = {
            "score": -110,
            "number": 1,
            "contracts": ["2S-EW"],
        }
        self.assertEqual(cld.format_par2_line(dealer_par), _EXPECTED_PAR2)

    def test_format_par2_line_includes_multiple_contracts(self):
        dealer_par = {
            "score": 100,
            "number": 2,
            "contracts": ["3C*-EW-1", "2N*-EW-1"],
        }
        self.assertEqual(
            cld.format_par2_line(dealer_par),
            'PAR2 "100" "3C*-EW-1" "2N*-EW-1" \n',
        )

    def test_format_play_line_counts_cards(self):
        self.assertEqual(
            cld.format_play_line("CTC4CA"),
            'PLAY 3 "CTC4CA" \n',
        )

    def test_format_play_line_empty(self):
        self.assertEqual(cld.format_play_line(""), 'PLAY 0 "" \n')

    def test_format_trace_line_includes_number_and_tricks(self):
        solved = {"number": 3, "tricks": [8, 8, 7]}
        self.assertEqual(cld.format_trace_line(solved), "TRACE 3 8 8 7 \n")


class GenerateDdPlayTest(unittest.TestCase):
    def test_generate_dd_play_has_fifty_two_cards(self):
        play = cld.generate_dd_play(_DEAL_CARDS, trump=0, first=0)
        self.assertEqual(len(play), 104)
        self.assertTrue(all(c in "SHDC23456789TJQKA" for c in play))

    def test_generate_dd_play_is_legal_for_analyse(self):
        play = cld.generate_dd_play(_DEAL_CARDS, trump=0, first=0)
        solved = analyse_play_pbn(_DEAL_CARDS, play=play, trump=0, first=0)
        self.assertGreater(solved["number"], 0)
        self.assertEqual(len(solved["tricks"]), solved["number"])


class CreateListForDtestTest(unittest.TestCase):
    def test_generate_unique_deals_are_unique_by_cards(self):
        deals = cld.generate_unique_deals(50, seed=1)
        self.assertEqual(len(deals), 50)
        cards = [d.cards for d in deals]
        self.assertEqual(len(cards), len(set(cards)))
        for d in deals:
            self.assertRegex(d.cards, r"^[NESW]:")
            self.assertEqual(d.cards.count(" "), 3)
            self.assertIn(d.trump, range(5))
            self.assertIn(d.first, range(4))
            self.assertIn(d.dealer, range(4))
            self.assertIn(d.vul, range(4))

    def test_generate_unique_deals_is_deterministic_for_seed(self):
        a = cld.generate_unique_deals(20, seed=42)
        b = cld.generate_unique_deals(20, seed=42)
        self.assertEqual([d.cards for d in a], [d.cards for d in b])
        self.assertEqual([(d.trump, d.first) for d in a], [(d.trump, d.first) for d in b])

    def test_format_fut_line_uses_only_card_count_entries(self):
        result = {
            "cards": 3,
            "suit": (0, 1, 2, 0, 0),
            "rank": (14, 13, 2, 0, 0),
            "equals": (0, 32, 0, 0, 0),
            "score": (5, 4, 3, 0, 0),
        }
        self.assertEqual(cld.format_fut_line(result), "FUT 3 0 1 2 14 13 2 0 32 0 5 4 3 \n")

    def test_format_deal_block_has_required_tags_in_order(self):
        deal = cld.DealSpec(
            dealer=0,
            vul=1,
            trump=4,
            first=2,
            cards="N:AKQ.AKQ.AKQ.AKQ2 JT9.JT9.JT9.JT93 876.876.876.8765 5432.5432.5432.4",
        )
        fut = {
            "cards": 1,
            "suit": (0,),
            "rank": (14,),
            "equals": (0,),
            "score": (7,),
        }
        block = cld.format_deal_block(deal, fut)
        tags = [line.split()[0] for line in block.splitlines() if line.strip()]
        self.assertEqual(tags, ["PBN", "FUT", "TABLE", "PAR", "PAR2", "PLAY", "TRACE"])
        self.assertIn('PBN 0 1 4 2 "N:AKQ.AKQ.AKQ.AKQ2', block)


class ParseArgsAndOutputTest(unittest.TestCase):
    def test_main_with_no_args_prints_usage_and_returns_2(self):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = cld.main([])
        self.assertEqual(rc, 2)
        help_text = buf.getvalue()
        self.assertRegex(help_text, r"(?i)usage:")
        self.assertIn("-n", help_text)
        self.assertIn("--seed", help_text)

    def test_parse_args_omitting_output_defaults_to_none(self):
        args = cld._parse_args(["-n", "3", "--seed", "1"])
        self.assertIsNone(args.output)
        self.assertEqual(args.count, 3)

    def test_parse_args_accepts_output_path(self):
        args = cld._parse_args(["-o", "hands/out.txt"])
        self.assertEqual(args.output, Path("hands/out.txt"))

    def test_parse_args_default_count_is_10(self):
        args = cld._parse_args(["--seed", "1"])
        self.assertEqual(args.count, 10)

    def test_parse_args_rejects_non_positive_count(self):
        with self.assertRaises(SystemExit):
            cld._parse_args(["-n", "0"])

    def test_parse_args_rejects_count_over_dtest_limit(self):
        with self.assertRaises(SystemExit):
            cld._parse_args(["-n", "100001"])

    def test_parse_args_accepts_count_at_dtest_limit(self):
        args = cld._parse_args(["-n", "100000"])
        self.assertEqual(args.count, 100_000)


if __name__ == "__main__":
    unittest.main()
