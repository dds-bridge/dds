"""CI coverage: dtest must parse and solve hands/nothing_makes.txt.

Pass-out PAR lines ("NS:" / "EW:") used to fail dtest parsing through v3.0.0.
"""
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


def _runfiles_root() -> Path:
    for key in ("RUNFILES_DIR", "TEST_SRCDIR"):
        if key in os.environ:
            return Path(os.environ[key])
    raise RuntimeError("not running under Bazel test")


def rlocation(relpath: str) -> Path:
    root = _runfiles_root()
    for candidate in (root / relpath, root / "_main" / relpath):
        if candidate.exists():
            return candidate
    raise FileNotFoundError(relpath)


def _dtest_binary() -> Path:
    for name in ("library/tests/dtest", "library/tests/dtest.exe"):
        try:
            return rlocation(name)
        except FileNotFoundError:
            continue
    raise FileNotFoundError("library/tests/dtest[.exe]")


class DtestNothingMakesTest(unittest.TestCase):
    def test_dtest_par_completes_on_nothing_makes(self) -> None:
        dtest = _dtest_binary()
        hands = rlocation("hands/nothing_makes.txt")
        proc = subprocess.run(
            [str(dtest), "-f", str(hands), "-s", "par", "-n", "1"],
            capture_output=True,
            text=True,
            check=False,
            timeout=60,
        )
        combined = proc.stdout + proc.stderr
        self.assertEqual(
            proc.returncode,
            0,
            msg=f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}",
        )
        self.assertIn("Number of hands", proc.stdout)
        self.assertNotIn("ERROR", combined)
        self.assertNotIn("Couldn't read", combined)


if __name__ == "__main__":
    unittest.main()
