"""Tests for par output in dd_table_for_deal."""

from __future__ import annotations

import io
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest import mock

from dd_table_for_deal import (
    _extract_deal_tags,
    _format_par_line,
    _load_deals,
    _parse_cli,
    _parse_vulnerable,
    _print_par,
    _print_usage,
    _unique_deals,
    main,
)

_EXAMPLE_DEAL = (
    "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
    "5.A95432.7632.K6 AKJ9842.K.T8.J93"
)
_HAND0_DEAL = (
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 "
    "K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
)
_MULTI_DEAL_PBN = (
    '{Board 1}\n'
    f'[Deal "{_EXAMPLE_DEAL}"]\n'
    "\n"
    "{Board 2}\n"
    f'[Deal "{_HAND0_DEAL}"]\n'
)


class ParseVulnerableTest(unittest.TestCase):
    def test_accepts_aliases_and_numeric_codes(self) -> None:
        cases = {
            "none": 0,
            "None": 0,
            "0": 0,
            "both": 1,
            "BOTH": 1,
            "1": 1,
            "ns": 2,
            "NS": 2,
            "2": 2,
            "ew": 3,
            "EW": 3,
            "3": 3,
        }
        for text, expected in cases.items():
            with self.subTest(text=text):
                self.assertEqual(_parse_vulnerable(text), expected)

    def test_rejects_unknown_values(self) -> None:
        for text in ("", "maybe", "4", "-1", "north"):
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    _parse_vulnerable(text)


class ParseCliTest(unittest.TestCase):
    def test_deal_only_defaults_vulnerable_to_none(self) -> None:
        deal, vulnerable, limit = _parse_cli(["prog", _EXAMPLE_DEAL])
        self.assertEqual(deal, _EXAMPLE_DEAL)
        self.assertEqual(vulnerable, 0)
        self.assertIsNone(limit)

    def test_vul_flag_before_deal(self) -> None:
        deal, vulnerable, limit = _parse_cli(["prog", "--vul", "ns", _EXAMPLE_DEAL])
        self.assertEqual(deal, _EXAMPLE_DEAL)
        self.assertEqual(vulnerable, 2)
        self.assertIsNone(limit)

    def test_vul_flag_after_deal(self) -> None:
        deal, vulnerable, limit = _parse_cli(["prog", _EXAMPLE_DEAL, "--vul", "both"])
        self.assertEqual(deal, _EXAMPLE_DEAL)
        self.assertEqual(vulnerable, 1)
        self.assertIsNone(limit)

    def test_limit_flag(self) -> None:
        deal, vulnerable, limit = _parse_cli(
            ["prog", "--limit", "3", _EXAMPLE_DEAL]
        )
        self.assertEqual(deal, _EXAMPLE_DEAL)
        self.assertEqual(vulnerable, 0)
        self.assertEqual(limit, 3)

    def test_limit_and_vul_together(self) -> None:
        deal, vulnerable, limit = _parse_cli(
            ["prog", "--vul", "ns", "--limit", "1", "boards.pbn"]
        )
        self.assertEqual(deal, "boards.pbn")
        self.assertEqual(vulnerable, 2)
        self.assertEqual(limit, 1)

    def test_rejects_non_positive_limit(self) -> None:
        for bad in ("0", "-1", "x", ""):
            with self.subTest(bad=bad):
                with self.assertRaises(ValueError):
                    _parse_cli(["prog", "--limit", bad, _EXAMPLE_DEAL])

    def test_help_returns_none(self) -> None:
        self.assertIsNone(_parse_cli(["prog", "-h"]))
        self.assertIsNone(_parse_cli(["prog", "--help"]))

    def test_usage_documents_numeric_vul_codes(self) -> None:
        buf = io.StringIO()
        with redirect_stderr(buf):
            _print_usage("prog")
        text = buf.getvalue()
        self.assertIn("none|both|ns|ew", text)
        self.assertIn("0|1|2|3", text)

    def test_usage_documents_limit(self) -> None:
        buf = io.StringIO()
        with redirect_stderr(buf):
            _print_usage("prog")
        self.assertIn("--limit", buf.getvalue())

    def test_rejects_unknown_flags(self) -> None:
        with self.assertRaises(ValueError):
            _parse_cli(["prog", "--oops", _EXAMPLE_DEAL])


class ExtractDealTagsTest(unittest.TestCase):
    def test_finds_all_deal_tags_in_pbn_text(self) -> None:
        self.assertEqual(
            _extract_deal_tags(_MULTI_DEAL_PBN),
            [_EXAMPLE_DEAL, _HAND0_DEAL],
        )

    def test_returns_empty_list_when_no_tags(self) -> None:
        self.assertEqual(_extract_deal_tags("{comment only}"), [])


class UniqueDealsTest(unittest.TestCase):
    def test_preserves_first_seen_order_and_drops_duplicates(self) -> None:
        deals = ["deal-a", "deal-b", "deal-a", "deal-c", "deal-b", "deal-a"]
        self.assertEqual(_unique_deals(deals), ["deal-a", "deal-b", "deal-c"])


class ReadPbnFileTest(unittest.TestCase):
    def test_reads_entire_file_past_old_8k_limit(self) -> None:
        """Regression: large PBN files must not be truncated at 8192 bytes."""
        from dd_table_for_deal import _read_pbn_file

        deals = [
            f"N:{i:02d}.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
            f"5.A95432.7632.K6 AKJ9842.K.T8.J93"
            for i in range(10)
        ]
        chunks: list[str] = []
        for deal in deals:
            pad = "x" * max(0, 900 - len(deal))
            chunks.append(f'{{pad {pad}}}\n[Deal "{deal}"]\n')
        text = "".join(chunks)
        self.assertGreater(len(text), 8192)

        with tempfile.NamedTemporaryFile(
            "w", suffix=".pbn", delete=False, encoding="utf-8"
        ) as tmp:
            tmp.write(text)
            path = tmp.name
        try:
            loaded = _read_pbn_file(path)
            self.assertIsNotNone(loaded)
            assert loaded is not None
            self.assertEqual(len(loaded), len(text))
            self.assertEqual(_extract_deal_tags(loaded), deals)
        finally:
            Path(path).unlink(missing_ok=True)


class ReadPbnStreamTest(unittest.TestCase):
    def test_allows_input_exactly_at_max(self) -> None:
        import dd_table_for_deal as mod

        with mock.patch.object(mod, "PBN_FILE_MAX", 16):
            text = mod._read_pbn_stream(io.StringIO("x" * 16))
        self.assertEqual(text, "x" * 16)

    def test_rejects_input_over_max(self) -> None:
        import dd_table_for_deal as mod

        with mock.patch.object(mod, "PBN_FILE_MAX", 16):
            with self.assertRaises(ValueError):
                mod._read_pbn_stream(io.StringIO("x" * 17))


class LoadDealsTest(unittest.TestCase):
    def test_raw_string_returns_single_deal(self) -> None:
        self.assertEqual(_load_deals(_EXAMPLE_DEAL), [_EXAMPLE_DEAL])

    def test_pbn_file_returns_all_deals(self) -> None:
        with mock.patch(
            "dd_table_for_deal._read_pbn_file", return_value=_MULTI_DEAL_PBN
        ):
            self.assertEqual(
                _load_deals("boards.pbn"),
                [_EXAMPLE_DEAL, _HAND0_DEAL],
            )

    def test_missing_pbn_path_reports_file_not_found(self) -> None:
        with mock.patch("dd_table_for_deal._read_pbn_file", return_value=None):
            with self.assertRaisesRegex(ValueError, r"Cannot read file: boards\.pbn"):
                _load_deals("boards.pbn")

    def test_missing_path_with_slash_reports_file_not_found(self) -> None:
        with mock.patch("dd_table_for_deal._read_pbn_file", return_value=None):
            with self.assertRaisesRegex(ValueError, r"Cannot read file: hands/x\.pbn"):
                _load_deals("hands/x.pbn")


class FormatParLineTest(unittest.TestCase):
    def test_single_sacrifice_contract(self) -> None:
        par_results = {
            "par_score": ["NS -300", "EW 300"],
            "par_contracts_string": ["NS:NS 5Hx", "EW:NS 5Hx"],
        }
        self.assertEqual(
            _format_par_line(par_results, vulnerable=0),
            "Par: NS 5Hx -2 -300",
        )

    def test_single_making_contract(self) -> None:
        par_results = {
            "par_score": ["NS -110", "EW 110"],
            "par_contracts_string": ["NS:EW 2S", "EW:EW 2S"],
        }
        self.assertEqual(
            _format_par_line(par_results, vulnerable=0),
            "Par: EW 2S = 110",
        )

    def test_making_contract_expands_multi_level_encoding(self) -> None:
        """DDS Par strings encode overtricks as concatenated levels (e.g. 45S)."""
        par_results = {
            "par_score": ["NS -450", "EW 450"],
            "par_contracts_string": ["NS:EW 45S", "EW:EW 45S"],
        }
        self.assertEqual(
            _format_par_line(par_results, vulnerable=0),
            "Par: EW 4S +1 450",
        )

    def test_multiple_sacrifice_contracts_on_one_line(self) -> None:
        par_results = {
            "par_score": ["NS 100", "EW -100"],
            "par_contracts_string": ["NS:EW 3Dx,EW 3Cx", "EW:EW 3Dx,EW 3Cx"],
        }
        self.assertEqual(
            _format_par_line(par_results, vulnerable=0),
            "Par: EW 3Dx, 3Cx -1 -100",
        )

    def test_omits_repeated_declaring_side_when_seats_differ(self) -> None:
        par_results = {
            "par_score": ["NS 100", "EW -100"],
            "par_contracts_string": ["NS:EW 4Hx,E 5Cx", "EW:EW 4Hx,E 5Cx"],
        }
        self.assertEqual(
            _format_par_line(par_results, vulnerable=0),
            "Par: EW 4Hx, 5Cx -1 -100",
        )

    def test_passed_out(self) -> None:
        par_results = {
            "par_score": ["NS 0", "EW 0"],
            "par_contracts_string": ["NS:", "EW:"],
        }
        self.assertEqual(_format_par_line(par_results, vulnerable=0), "Par: 0")


class PrintParTest(unittest.TestCase):
    def test_single_par_uses_compact_line(self) -> None:
        par_results = {
            "par_score": ["NS -300", "EW 300"],
            "par_contracts_string": ["NS:NS 5Hx", "EW:NS 5Hx"],
        }
        buf = io.StringIO()
        with redirect_stdout(buf):
            _print_par(par_results, vulnerable=0)
        self.assertEqual(buf.getvalue(), "Par: NS 5Hx -2 -300\n")

    def test_multiple_pars_use_compact_line(self) -> None:
        par_results = {
            "par_score": ["NS 100", "EW -100"],
            "par_contracts_string": ["NS:EW 3Dx,EW 3Cx", "EW:EW 3Dx,EW 3Cx"],
        }
        buf = io.StringIO()
        with redirect_stdout(buf):
            _print_par(par_results, vulnerable=0)
        self.assertEqual(buf.getvalue(), "Par: EW 3Dx, 3Cx -1 -100\n")
        self.assertNotIn("NS score:", buf.getvalue())


class MainParOutputTest(unittest.TestCase):
    def test_main_prints_compact_par_for_example_deal(self) -> None:
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = main(["dd_table_for_deal", _EXAMPLE_DEAL])
        self.assertEqual(rc, 0)
        out = buf.getvalue()
        self.assertIn("Par: NS 5Hx -2 -300", out)
        self.assertNotIn("NS score:", out)
        self.assertLess(out.index("North"), out.index("Par:"))

    def test_main_passes_vulnerable_to_par(self) -> None:
        fake_tables = {
            "tables": [
                {
                    "res_table": [[0] * 4 for _ in range(5)],
                }
            ]
        }
        fake_par = {
            "par_score": ["NS 0", "EW 0"],
            "par_contracts_string": ["NS:", "EW:"],
        }
        with mock.patch(
            "dd_table_for_deal.calc_all_tables_pbn", return_value=fake_tables
        ), mock.patch(
            "dd_table_for_deal.calc_par_from_table", return_value=fake_par
        ) as par_mock, mock.patch(
            "dd_table_for_deal._print_pbn_hand"
        ), mock.patch(
            "dd_table_for_deal._print_table"
        ), redirect_stdout(io.StringIO()):
            rc = main(["dd_table_for_deal", "--vul", "ew", _EXAMPLE_DEAL])

        self.assertEqual(rc, 0)
        par_mock.assert_called_once()
        args, kwargs = par_mock.call_args
        self.assertEqual(kwargs.get("vulnerable", args[1] if len(args) > 1 else None), 3)

    def test_main_returns_error_when_par_fails(self) -> None:
        fake_tables = {
            "tables": [{"res_table": [[0] * 4 for _ in range(5)]}],
        }
        with mock.patch(
            "dd_table_for_deal.calc_all_tables_pbn", return_value=fake_tables
        ), mock.patch(
            "dd_table_for_deal.calc_par_from_table",
            side_effect=RuntimeError("par failed"),
        ), mock.patch(
            "dd_table_for_deal._print_pbn_hand"
        ), mock.patch(
            "dd_table_for_deal._print_table"
        ), redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()) as err:
            rc = main(["dd_table_for_deal", _EXAMPLE_DEAL])

        self.assertEqual(rc, 1)
        self.assertIn("DDS error:", err.getvalue())

    def test_main_processes_all_deals_from_pbn_file(self) -> None:
        fake_tables = {
            "tables": [
                {"res_table": [[0] * 4 for _ in range(5)]},
                {"res_table": [[1] * 4 for _ in range(5)]},
            ]
        }
        fake_par = {
            "par_score": ["NS 0", "EW 0"],
            "par_contracts_string": ["NS:", "EW:"],
        }
        with mock.patch(
            "dd_table_for_deal._read_pbn_file", return_value=_MULTI_DEAL_PBN
        ), mock.patch(
            "dd_table_for_deal.calc_all_tables_pbn", return_value=fake_tables
        ) as calc_mock, mock.patch(
            "dd_table_for_deal.calc_par_from_table", return_value=fake_par
        ) as par_mock, mock.patch(
            "dd_table_for_deal._print_pbn_hand"
        ) as hand_mock, mock.patch(
            "dd_table_for_deal._print_table"
        ), redirect_stdout(io.StringIO()) as buf:
            rc = main(["dd_table_for_deal", "boards.pbn"])

        self.assertEqual(rc, 0)
        self.assertEqual(
            [call.args[0] for call in calc_mock.call_args_list],
            [[_EXAMPLE_DEAL], [_HAND0_DEAL]],
        )
        self.assertEqual(par_mock.call_count, 2)
        self.assertEqual(hand_mock.call_count, 2)
        titles = [call.args[0] for call in hand_mock.call_args_list]
        self.assertEqual(titles[0], "Deal 1:\n")
        self.assertEqual(titles[1], "Deal 2:\n")
        out = buf.getvalue()
        self.assertIn("Par: 0\n\n", out)
        self.assertEqual(out.count("Par: 0\n\n"), 2)

    def test_main_limit_solves_only_first_n_unique_deals(self) -> None:
        duplicate_pbn = (
            f'[Deal "{_EXAMPLE_DEAL}"]\n'
            f'[Deal "{_EXAMPLE_DEAL}"]\n'
            f'[Deal "{_HAND0_DEAL}"]\n'
            f'[Deal "{_EXAMPLE_DEAL}"]\n'
        )
        fake_tables = {
            "tables": [{"res_table": [[0] * 4 for _ in range(5)]}],
        }
        fake_par = {
            "par_score": ["NS 0", "EW 0"],
            "par_contracts_string": ["NS:", "EW:"],
        }
        with mock.patch(
            "dd_table_for_deal._read_pbn_file", return_value=duplicate_pbn
        ), mock.patch(
            "dd_table_for_deal.calc_all_tables_pbn", return_value=fake_tables
        ) as calc_mock, mock.patch(
            "dd_table_for_deal.calc_par_from_table", return_value=fake_par
        ), mock.patch(
            "dd_table_for_deal._print_pbn_hand"
        ) as hand_mock, mock.patch(
            "dd_table_for_deal._print_table"
        ), redirect_stdout(io.StringIO()):
            rc = main(["dd_table_for_deal", "--limit", "1", "dupes.pbn"])

        self.assertEqual(rc, 0)
        self.assertEqual(
            [call.args[0] for call in calc_mock.call_args_list],
            [[_EXAMPLE_DEAL]],
        )
        self.assertEqual(hand_mock.call_count, 1)
        self.assertEqual(hand_mock.call_args.args[0], "dd_table_for_deal:\n")

    def test_main_solves_duplicate_deals_only_once(self) -> None:
        duplicate_pbn = (
            f'[Deal "{_EXAMPLE_DEAL}"]\n'
            f'[Deal "{_EXAMPLE_DEAL}"]\n'
            f'[Deal "{_HAND0_DEAL}"]\n'
            f'[Deal "{_EXAMPLE_DEAL}"]\n'
        )
        fake_tables = {
            "tables": [{"res_table": [[0] * 4 for _ in range(5)]}],
        }
        fake_par = {
            "par_score": ["NS 0", "EW 0"],
            "par_contracts_string": ["NS:", "EW:"],
        }
        with mock.patch(
            "dd_table_for_deal._read_pbn_file", return_value=duplicate_pbn
        ), mock.patch(
            "dd_table_for_deal.calc_all_tables_pbn", return_value=fake_tables
        ) as calc_mock, mock.patch(
            "dd_table_for_deal.calc_par_from_table", return_value=fake_par
        ), mock.patch(
            "dd_table_for_deal._print_pbn_hand"
        ) as hand_mock, mock.patch(
            "dd_table_for_deal._print_table"
        ), redirect_stdout(io.StringIO()):
            rc = main(["dd_table_for_deal", "dupes.pbn"])

        self.assertEqual(rc, 0)
        self.assertEqual(
            [call.args[0] for call in calc_mock.call_args_list],
            [[_EXAMPLE_DEAL], [_HAND0_DEAL]],
        )
        self.assertEqual(hand_mock.call_count, 2)


if __name__ == "__main__":
    unittest.main()
