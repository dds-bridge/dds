"""CI coverage: dtest must parse hands/nothing_makes.txt under `-s par`.

Pass-out PAR lines ("NS:" / "EW:") used to fail dtest parsing through v3.0.0.
"""
from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock


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
    """Resolve a runfiles path such as ``hands/nothing_makes.txt``."""
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


class RlocationTest(unittest.TestCase):
    def test_rlocation_uses_runfiles_manifest_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            real = root / "nothing_makes.txt"
            real.write_text("NUMBER 1\n", encoding="utf-8")
            manifest = root / "MANIFEST"
            manifest.write_text(
                f"_main/hands/nothing_makes.txt {real}\n",
                encoding="utf-8",
            )
            with mock.patch.dict(
                "os.environ",
                {"RUNFILES_MANIFEST_FILE": str(manifest)},
                clear=True,
            ):
                found = rlocation("hands/nothing_makes.txt")
            self.assertTrue(found.samefile(real))

    def test_rlocation_manifest_accepts_unprefixed_key(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            real = root / "dtest.exe"
            real.write_text("", encoding="utf-8")
            manifest = root / "MANIFEST"
            manifest.write_text(
                f"library/tests/dtest.exe {real}\n",
                encoding="utf-8",
            )
            with mock.patch.dict(
                "os.environ",
                {"RUNFILES_MANIFEST_FILE": str(manifest)},
                clear=True,
            ):
                found = rlocation("library/tests/dtest.exe")
            self.assertTrue(found.samefile(real))


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
