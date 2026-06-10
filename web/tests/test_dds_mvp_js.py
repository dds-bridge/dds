"""Unit tests for web/dds_mvp.js via Node's built-in test runner.

Run with: bazel test //web:dds_mvp_js_test
or: python -m unittest web.tests.test_dds_mvp_js
or: node --test web/tests/dds_mvp_test.mjs
"""

from __future__ import annotations

import os
import shutil
import subprocess
import unittest
from pathlib import Path

TESTS_ROOT = Path(__file__).resolve().parent


def _runfiles_root() -> Path:
    for key in ("RUNFILES_DIR", "TEST_SRCDIR"):
        if key in os.environ:
            return Path(os.environ[key])
    return TESTS_ROOT.parent.parent


def rlocation(relpath: str) -> Path:
    root = _runfiles_root()
    for candidate in (root / relpath, root / "_main" / relpath):
        if candidate.exists():
            return candidate
    raise FileNotFoundError(relpath)


class DdsMvpJsTest(unittest.TestCase):
    def test_dds_mvp_js(self) -> None:
        node = shutil.which("node")
        if not node:
            raise unittest.SkipTest("node not found")

        version = subprocess.run(
            [node, "--version"],
            capture_output=True,
            text=True,
            check=False,
        ).stdout.strip()
        try:
            major = int(version.lstrip("v").split(".", 1)[0])
        except ValueError:
            raise unittest.SkipTest(f"could not parse node version: {version!r}")
        if major < 18:
            raise unittest.SkipTest(f"node >= 18 required for `node --test` (found {version})")

        test_script = rlocation("web/tests/dds_mvp_test.mjs")
        dds_mvp_js = rlocation("web/dds_mvp.js")
        env = os.environ.copy()
        env["DDS_MVP_JS"] = str(dds_mvp_js)
        try:
            proc = subprocess.run(
                [node, "--test", str(test_script)],
                capture_output=True,
                text=True,
                check=False,
                env=env,
                timeout=60,
            )
        except subprocess.TimeoutExpired as exc:
            self.fail(f"node --test timed out: {exc}")
        self.assertEqual(
            proc.returncode,
            0,
            msg=proc.stdout + proc.stderr,
        )


if __name__ == "__main__":
    unittest.main()
