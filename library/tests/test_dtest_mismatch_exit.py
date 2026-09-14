"""CI coverage: dtest must exit non-zero on the first expected-result mismatch."""

from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


def _runfiles_root() -> Path | None:
    for key in ("RUNFILES_DIR", "TEST_SRCDIR"):
        if key in os.environ:
            return Path(os.environ[key])
    return None


def _rlocation_from_manifest(relpath: str) -> Path | None:
    manifest = os.environ.get("RUNFILES_MANIFEST_FILE")
    if not manifest:
        return None
    keys = {relpath, f"_main/{relpath}"}
    try:
        with open(manifest, encoding="utf-8") as fh:
            for line in fh:
                line = line.rstrip("\n")
                if not line or line.startswith("[") or line.startswith(" "):
                    continue
                space = line.find(" ")
                if space < 0:
                    continue
                key, value = line[:space], line[space + 1 :]
                if key in keys and value:
                    path = Path(value)
                    if path.exists():
                        return path
    except OSError:
        return None
    return None


def rlocation(relpath: str) -> Path:
    root = _runfiles_root()
    if root is not None:
        for candidate in (root / relpath, root / "_main" / relpath):
            if candidate.exists():
                return candidate

    from_manifest = _rlocation_from_manifest(relpath)
    if from_manifest is not None:
        return from_manifest

    raise FileNotFoundError(relpath)


def _dtest_binary() -> Path:
    for name in ("library/tests/dtest", "library/tests/dtest.exe"):
        try:
            return rlocation(name)
        except FileNotFoundError:
            continue
    raise FileNotFoundError("library/tests/dtest[.exe]")


# Same deal twice with intentional wrong goldens so both would mismatch if
# dtest kept going past the first difference.
_DEAL = (
    'PBN 1 0 2 0 "N:Q87.T8.AKJT64.J6 964.AJ765.Q73.74 AKJT2.Q943..AK95 '
    '53.K2.9852.QT832" \n'
    "FUT 10 0 3 3 2 1 2 1 2 2 0 12 6 11 14 8 4 10 6 11 8 0 0 0 8192 0 0 0 0 "
    "1024 128 10 10 10 10 10 10 10 10 10 9 \n"
    "TABLE 11 2 11 1 9 4 9 4 10 3 10 3 8 5 8 4 10 3 10 3 \n"
    'PAR "NS 450" "EW -450" "NS:NS 45S" "EW:NS 45S" \n'
    'PAR2 "450" "4S-NS+1" \n'
    'PLAY 52 "SQS4S2S3DAD3H4D2DKD7H3D5D6DQC5D9H7H9HKH8H2HTHAHQC4CAC3C6SAS5S8S6'
    'CKC2CJC7C9CQD4HJDJH6SKD8S7S9SJCTSTC8DTH5" \n'
    "TRACE 49 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 "
    "3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 \n"
)


def _two_deal_hands(mutate: str, replacement: str) -> str:
    first = _DEAL.replace(mutate, replacement, 1)
    second = _DEAL.replace(mutate, replacement, 1)
    return "NUMBER 2 \n" + first + second


class DtestMismatchExitTest(unittest.TestCase):
    def _run_mismatch(
        self,
        *,
        solver: str,
        hands_text: str,
        first_marker: str,
        second_marker: str,
        timeout: int = 120,
    ) -> None:
        dtest = _dtest_binary()
        with tempfile.TemporaryDirectory() as tmp:
            hands = Path(tmp) / "mismatch.txt"
            hands.write_text(hands_text, encoding="utf-8")
            proc = subprocess.run(
                [str(dtest), "-f", str(hands), "-s", solver, "-n", "1"],
                capture_output=True,
                text=True,
                check=False,
                timeout=timeout,
            )

        self.assertNotEqual(
            proc.returncode,
            0,
            msg=f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}",
        )
        self.assertIn(first_marker, proc.stdout)
        self.assertNotIn(second_marker, proc.stdout)

    def test_dtest_exits_nonzero_on_first_par_mismatch(self) -> None:
        self._run_mismatch(
            solver="par",
            hands_text=_two_deal_hands('PAR "NS 450"', 'PAR "NS -999"'),
            first_marker="loop_par i 0: Difference",
            second_marker="loop_par i 1:",
        )

    def test_dtest_exits_nonzero_on_first_calc_mismatch(self) -> None:
        self._run_mismatch(
            solver="calc",
            hands_text=_two_deal_hands("TABLE 11 ", "TABLE 0 "),
            first_marker="loop_calc: j 0: Difference",
            second_marker="loop_calc: j 1:",
        )

    def test_dtest_exits_nonzero_on_first_dealerpar_mismatch(self) -> None:
        self._run_mismatch(
            solver="dealerpar",
            hands_text=_two_deal_hands('PAR2 "450"', 'PAR2 "999"'),
            first_marker="loop_dealerpar i 0: Difference",
            second_marker="loop_dealerpar i 1:",
        )

    def test_dtest_exits_nonzero_on_first_solve_mismatch(self) -> None:
        self._run_mismatch(
            solver="solve",
            hands_text=_two_deal_hands(
                "10 10 10 10 10 10 10 10 10 9",
                "10 10 10 10 10 10 10 10 10 0",
            ),
            first_marker="loop_solve: i 0, j 0: Difference",
            second_marker="loop_solve: i 1",
        )

    def test_dtest_exits_nonzero_on_first_play_mismatch(self) -> None:
        self._run_mismatch(
            solver="play",
            hands_text=_two_deal_hands("TRACE 49 3 ", "TRACE 49 0 "),
            first_marker="loop_play: i 0, j 0: Difference",
            second_marker="loop_play: i 1",
        )


if __name__ == "__main__":
    unittest.main()
