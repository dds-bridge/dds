"""System tests: run WASM example binaries under Node.js."""
from __future__ import annotations

import os
import shutil
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


@unittest.skipUnless(shutil.which("node"), "node not found")
class WasmExamplesSystemTest(unittest.TestCase):
    def test_calc_dd_table_pbn_wasm(self) -> None:
        js = rlocation("wasm/calc_dd_table_pbn.js")
        proc = subprocess.run(
            ["node", str(js)],
            capture_output=True,
            text=True,
            check=False,
            timeout=120,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stderr)
        for hand in (1, 2, 3):
            self.assertIn(f"CalcDDtable, hand {hand}: OK", proc.stdout)
        self.assertNotIn("ERROR", proc.stdout + proc.stderr)

    def test_dtest_wasm_solve_list1(self) -> None:
        js = rlocation("wasm/dtest.js")
        hands = rlocation("hands/list1.txt")
        proc = subprocess.run(
            ["node", str(js), "-f", str(hands), "-s", "solve", "-n", "1"],
            capture_output=True,
            text=True,
            check=False,
            timeout=120,
            cwd=str(js.parent),
        )
        self.assertEqual(
            proc.returncode,
            0,
            msg=f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}",
        )
        self.assertIn("Number of hands", proc.stdout)
        self.assertNotIn("ERROR", proc.stdout + proc.stderr)


if __name__ == "__main__":
    unittest.main()
