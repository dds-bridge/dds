#!/usr/bin/env python3
"""Unit tests for create_list_for_dtest."""

import contextlib
import io
import tempfile
import unittest
import unittest.mock
from pathlib import Path

import create_list_for_dtest as cld
from dds3 import analyse_play_pbn, solve_board_pbn


# list1.txt deal 1.
_DEAL_CARDS = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
_EAST_START_CARDS = (
    "E:QJT5432.T.6.QJ82 .J97543.K7532.94 87.A62.QJT4.AT75 AK96.KQ8.A98.K63"
)

_EXPECTED_TABLE = "TABLE 5 8 5 8 6 6 6 6 5 7 5 7 7 5 7 5 6 6 6 6 \n"
_EXPECTED_PAR = 'PAR "NS -110" "EW 110" "NS:EW 2S" "EW:EW 2S" \n'
_EXPECTED_PAR2 = 'PAR2 "-110" "2S-EW" \n'

_SUIT_INDEX = {"S": 0, "H": 1, "D": 2, "C": 3}


def _play_to_cards(play: str) -> list[tuple[int, int]]:
    cards: list[tuple[int, int]] = []
    for i in range(0, len(play), 2):
        cards.append((_SUIT_INDEX[play[i]], cld._char_to_rank(play[i + 1])))
    return cards


def _solve_max(
    trump: int,
    leader: int,
    cur_trick: list[tuple[int, int]],
    hands: list[list[tuple[int, int]]],
) -> int:
    cur_suits = [0, 0, 0]
    cur_ranks = [0, 0, 0]
    for i, (suit, rank) in enumerate(cur_trick):
        cur_suits[i] = suit
        cur_ranks[i] = rank
    fut = solve_board_pbn(
        cld._hands_to_pbn(hands),
        trump=trump,
        first=leader,
        current_trick_suit=tuple(cur_suits),
        current_trick_rank=tuple(cur_ranks),
        target=-1,
        solutions=1,
        mode=1,
    )
    return int(fut["score"][0])


def _check_play_self_consistency(
    test_case: unittest.TestCase,
    remain_cards: str,
    *,
    trump: int,
    first: int,
    play: str,
) -> None:
    """Match analyse_play_consistency.cpp: AnalysePlay vs SolveBoard each ply."""
    hands = cld._parse_remain_cards(remain_cards)
    cards = _play_to_cards(play)
    solved = analyse_play_pbn(remain_cards, play=play, trump=trump, first=first)

    decl_parity = 1 - (first % 2)
    cur_hands = [list(hand) for hand in hands]
    cur: list[tuple[int, int]] = []
    leader = first
    completed = 0
    decl_won = 0

    for k, card in enumerate(cards):
        if k < solved["number"]:
            remaining = 13 - completed
            player_to_act = (leader + len(cur)) % 4
            sb = _solve_max(trump, leader, cur, cur_hands)
            decl_remaining = sb if player_to_act % 2 == decl_parity else remaining - sb
            expected = decl_won + decl_remaining
            test_case.assertEqual(
                expected,
                solved["tricks"][k],
                f"AnalysePlay disagrees with SolveBoard at ply {k}",
            )

        player = (leader + len(cur)) % 4
        cur_hands[player].remove(card)
        cur.append(card)
        if len(cur) == 4:
            winner = cld._trick_winner(cur, trump, leader)
            if winner % 2 == decl_parity:
                decl_won += 1
            completed += 1
            leader = winner
            cur.clear()


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

    def test_format_par2_line_empty_contracts_emits_pass(self):
        dealer_par = {
            "score": 0,
            "number": 0,
            "contracts": [],
        }
        self.assertEqual(cld.format_par2_line(dealer_par), 'PAR2 "0" "pass" \n')

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

    def test_format_trace_line_zero_has_no_trick_field(self):
        solved = {"number": 0, "tricks": []}
        self.assertEqual(cld.format_trace_line(solved), "TRACE 0 \n")


class TrickWinnerTest(unittest.TestCase):
    def test_nt_highest_of_lead_suit_wins(self):
        trick = [(0, 14), (0, 13), (0, 12), (0, 2)]
        self.assertEqual(cld._trick_winner(trick, trump=4, leader=0), 0)

    def test_trump_beats_lead_suit(self):
        trick = [(0, 14), (1, 5), (0, 13), (1, 14)]
        self.assertEqual(cld._trick_winner(trick, trump=1, leader=0), 3)

    def test_higher_trump_beats_lower_trump(self):
        trick = [(1, 10), (1, 14), (0, 2), (0, 3)]
        self.assertEqual(cld._trick_winner(trick, trump=1, leader=1), 2)

    def test_off_suit_discard_does_not_win(self):
        trick = [(2, 14), (2, 13), (0, 2), (1, 5)]
        self.assertEqual(cld._trick_winner(trick, trump=4, leader=2), 2)


class ParseRemainCardsTest(unittest.TestCase):
    def test_parses_north_start_deal_into_four_hands(self):
        hands = cld._parse_remain_cards(_DEAL_CARDS)
        self.assertEqual(len(hands), 4)
        for hand in hands:
            self.assertEqual(len(hand), 13)

    def test_east_start_rotates_first_hand_to_east(self):
        hands = cld._parse_remain_cards(_EAST_START_CARDS)
        self.assertIn((0, 12), hands[1])  # East holds spade queen from first chunk

    def test_roundtrip_normalizes_to_north_start(self):
        hands = cld._parse_remain_cards(_EAST_START_CARDS)
        self.assertTrue(cld._hands_to_pbn(hands).startswith("N:"))

    def test_rejects_missing_seat_prefix(self):
        with self.assertRaises(ValueError) as ctx:
            cld._parse_remain_cards("bad deal")
        message = str(ctx.exception)
        self.assertIn("remain cards must start with N:/E:/S:/W:, got", message)
        self.assertNotIn("W::", message)


class ParsePbnLineTest(unittest.TestCase):
    def test_parses_valid_line_with_trailing_whitespace(self):
        line = 'PBN 0 1 4 2 "N:AKQ.AKQ.AKQ.AKQ2 JT98.JT9.JT9.JT98 T765.T876.T876.T7 432.5432.5432.543"  \n'
        dealer, vul, trump, first, cards = cld._parse_pbn_line(line)
        self.assertEqual((dealer, vul, trump, first), (0, 1, 4, 2))
        self.assertTrue(cards.startswith("N:"))

    def test_rejects_trailing_garbage_after_quoted_cards(self):
        line = 'PBN 0 0 0 0 "N:AKQ.AKQ.AKQ.AKQ2 JT98.JT9.JT9.JT98 T765.T876.T876.T7 432.5432.5432.543" extra\n'
        with self.assertRaisesRegex(ValueError, "unrecognized PBN line"):
            cld._parse_pbn_line(line)


class FillDealBlockTest(unittest.TestCase):
    def test_raises_when_pbn_line_missing(self):
        stub = "FUT 0 \nTABLE 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 \n"
        with self.assertRaisesRegex(ValueError, "deal block missing PBN line"):
            cld.fill_deal_block(stub)


class GenerateDdPlayTest(unittest.TestCase):
    def test_generate_dd_play_has_fifty_two_cards(self):
        play = cld.generate_dd_play(_DEAL_CARDS, trump=0, first=0)
        self.assertEqual(len(play), 104)
        self.assertTrue(all(c in "SHDC23456789TJQKA" for c in play))

    def test_generate_dd_play_passes_same_solver_context_to_each_solve(self):
        contexts: list[object | None] = []

        def tracking_solve(*args, **kwargs):  # type: ignore[no-untyped-def]
            contexts.append(kwargs.get("context"))
            return solve_board_pbn(*args, **kwargs)

        with unittest.mock.patch.object(cld, "solve_board_pbn", side_effect=tracking_solve):
            cld.generate_dd_play(_DEAL_CARDS, trump=0, first=0)

        self.assertEqual(len(contexts), 52)
        self.assertIsNotNone(contexts[0])
        self.assertTrue(all(ctx is contexts[0] for ctx in contexts))

    def test_generate_dd_play_matches_solve_board_at_each_ply(self):
        play = cld.generate_dd_play(_DEAL_CARDS, trump=0, first=0)
        _check_play_self_consistency(
            self,
            _DEAL_CARDS,
            trump=0,
            first=0,
            play=play,
        )


class CreateListForDtestTest(unittest.TestCase):
    def test_generate_deals_returns_requested_count_and_valid_fields(self):
        deals = cld.generate_deals(50, seed=1)
        self.assertEqual(len(deals), 50)
        for d in deals:
            self.assertRegex(d.cards, r"^[NESW]:")
            self.assertEqual(d.cards.count(" "), 3)
            self.assertIn(d.trump, range(5))
            self.assertIn(d.first, range(4))
            self.assertIn(d.dealer, range(4))
            self.assertIn(d.vul, range(4))

    def test_generate_deals_is_deterministic_for_seed(self):
        a = cld.generate_deals(20, seed=42)
        b = cld.generate_deals(20, seed=42)
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

    def test_parse_args_count_help_mentions_cost(self):
        help_text = cld._build_parser().format_help()
        self.assertRegex(help_text, r"(?i)slow|long|expensive|time")

    def test_large_count_warning_message_for_big_n(self):
        warning = cld._large_count_warning(1000)
        self.assertIsNotNone(warning)
        self.assertIn("1000", warning)
        self.assertRegex(warning, r"(?i)slow|long|time")

    def test_large_count_warning_message_none_for_small_n(self):
        self.assertIsNone(cld._large_count_warning(10))

    def test_main_prints_large_count_warning(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "out.txt"
            err = io.StringIO()
            fake_deal = cld.DealSpec(
                dealer=0, vul=0, trump=0, first=0, cards=_DEAL_CARDS
            )
            with contextlib.redirect_stderr(err), unittest.mock.patch.object(
                cld,
                "iter_deals",
                return_value=iter([fake_deal]),
            ), unittest.mock.patch.object(
                cld,
                "solve_fut",
                return_value={
                    "cards": 0,
                    "suit": (),
                    "rank": (),
                    "equals": (),
                    "score": (),
                },
            ), unittest.mock.patch.object(
                cld,
                "format_filled_deal_block",
                return_value=(
                    'PBN 0 0 0 0 "N:..." \n'
                    "FUT 0 \n"
                    "TABLE 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 \n"
                    'PAR "NS 0" "EW 0" "NS:EW 2S" "EW:EW 2S" \n'
                    'PAR2 "0" "pass" \n'
                    'PLAY 0 "" \n'
                    "TRACE 1 0 \n"
                ),
            ):
                # NUMBER header uses args.count (1000); we only generate one mocked deal.
                rc = cld.main(["-n", "1000", "--seed", "1", "-o", str(out)])
            self.assertEqual(rc, 0)
            self.assertRegex(err.getvalue(), r"(?i)slow|long|time")

    def test_main_writes_one_filled_deal_to_output_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "list1.txt"
            err = io.StringIO()
            with contextlib.redirect_stderr(err):
                rc = cld.main(["-n", "1", "--seed", "1", "-o", str(out)])
            self.assertEqual(rc, 0)
            text = out.read_text(encoding="utf-8")
            self.assertTrue(text.startswith("NUMBER 1 \n"))
            tags = [line.split()[0] for line in text.splitlines() if line.strip()]
            self.assertEqual(
                tags,
                ["NUMBER", "PBN", "FUT", "TABLE", "PAR", "PAR2", "PLAY", "TRACE"],
            )
            play_line = next(line for line in text.splitlines() if line.startswith("PLAY "))
            self.assertRegex(play_line, r'^PLAY 52 "')
            self.assertIn(f"Wrote 1 deals -> {out}", err.getvalue())


if __name__ == "__main__":
    unittest.main()
