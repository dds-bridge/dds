"""Search changes must not change results: dtest on hands/list100.txt.

Search-order changes (move ordering, transposition cutoffs) must leave every
value unchanged. list100.txt holds reference results from earlier DDS
releases; dtest exits non-zero on the first mismatch. Three solver modes
cover the search entry points:

- solve: SolveBoard scores and the returned cards;
- calc: full double-dummy tables;
- play: AnalysePlay, which solves positions partway through a trick.
"""
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


def _runfiles_root() -> Path | None:
    for key in ("RUNFILES_DIR", "TEST_SRCDIR"):
        if key in os.environ:
            return Path(os.environ[key])
    return None


def _rlocation_from_manifest(relpath: str) -> Path | None:
    """Look up ``relpath`` in RUNFILES_MANIFEST_FILE (Windows-style runfiles)."""
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
    """Resolve a runfiles path such as ``hands/list100.txt``."""
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


class DtestReferenceListTest(unittest.TestCase):
    def _run_mode(self, mode: str) -> None:
        dtest = _dtest_binary()
        hands = rlocation("hands/list100.txt")
        proc = subprocess.run(
            [str(dtest), "-f", str(hands), "-s", mode],
            capture_output=True,
            text=True,
            check=False,
            timeout=300,
        )
        combined = proc.stdout + proc.stderr
        self.assertEqual(
            proc.returncode,
            0,
            msg=f"mode={mode}\nstdout:\n{proc.stdout}\nstderr:\n{proc.stderr}",
        )
        self.assertRegex(proc.stdout, r"Number of hands\s+100\b")
        self.assertNotIn("ERROR", combined)
        self.assertNotIn("Couldn't read", combined)

    def test_solve_matches_reference(self) -> None:
        self._run_mode("solve")

    def test_calc_matches_reference(self) -> None:
        self._run_mode("calc")

    def test_play_matches_reference(self) -> None:
        self._run_mode("play")


if __name__ == "__main__":
    unittest.main()
