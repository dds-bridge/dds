"""Tests for par output in dd_table_for_deal."""

from __future__ import annotations

import io
import unittest
from contextlib import redirect_stdout
from unittest import mock

from dd_table_for_deal import (
    _format_par_line,
    _parse_cli,
    _parse_vulnerable,
    _print_par,
    main,
)

_EXAMPLE_DEAL = (
    "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
    "5.A95432.7632.K6 AKJ9842.K.T8.J93"
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
        deal, vulnerable = _parse_cli(["prog", _EXAMPLE_DEAL])
        self.assertEqual(deal, _EXAMPLE_DEAL)
        self.assertEqual(vulnerable, 0)

    def test_vul_flag_before_deal(self) -> None:
        deal, vulnerable = _parse_cli(["prog", "--vul", "ns", _EXAMPLE_DEAL])
        self.assertEqual(deal, _EXAMPLE_DEAL)
        self.assertEqual(vulnerable, 2)

    def test_vul_flag_after_deal(self) -> None:
        deal, vulnerable = _parse_cli(["prog", _EXAMPLE_DEAL, "--vul", "both"])
        self.assertEqual(deal, _EXAMPLE_DEAL)
        self.assertEqual(vulnerable, 1)

    def test_help_returns_none(self) -> None:
        self.assertIsNone(_parse_cli(["prog", "-h"]))
        self.assertIsNone(_parse_cli(["prog", "--help"]))

    def test_rejects_unknown_flags(self) -> None:
        with self.assertRaises(ValueError):
            _parse_cli(["prog", "--oops", _EXAMPLE_DEAL])


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
            "Par: EW 2S +0 -110",
        )

    def test_multiple_contracts_returns_none(self) -> None:
        par_results = {
            "par_score": ["NS -600", "EW 600"],
            "par_contracts_string": ["NS:EW 2S,EW 3S", "EW:EW 2S,EW 3S"],
        }
        self.assertIsNone(_format_par_line(par_results, vulnerable=0))

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

    def test_multiple_pars_use_verbose_format(self) -> None:
        par_results = {
            "par_score": ["NS -600", "EW 600"],
            "par_contracts_string": ["NS:EW 2S,EW 3S", "EW:EW 2S,EW 3S"],
        }
        buf = io.StringIO()
        with redirect_stdout(buf):
            _print_par(par_results, vulnerable=0)
        self.assertIn("NS score:", buf.getvalue())
        self.assertIn("NS list :", buf.getvalue())


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


if __name__ == "__main__":
    unittest.main()
