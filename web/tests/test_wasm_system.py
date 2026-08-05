"""End-to-end system tests for the DDS Web WASM build and Node smoke harness."""
from __future__ import annotations

import shutil
import subprocess
import unittest
from pathlib import Path

from web_site import stage_web_site

TESTS_ROOT = Path(__file__).resolve().parent


def _require_node() -> str:
    node = shutil.which("node")
    if not node:
        raise unittest.SkipTest("node not found")
    return node


@unittest.skipUnless(shutil.which("node"), "node not found")
class DdsWebWasmSystemTest(unittest.TestCase):
    def test_update_pipeline_and_node_smoke(self) -> None:
        import tempfile

        node = _require_node()

        with tempfile.TemporaryDirectory() as tmpdir:
            tmp = Path(tmpdir)
            stage_web_site(tmp)

            proc = subprocess.run(
                [
                    node,
                    str(TESTS_ROOT / "dds_web_wasm_node.mjs"),
                    str(tmp / "dds_web_wasm.js"),
                    str(tmp / "dds_web_wasm.wasm"),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(
                proc.returncode,
                0,
                msg=proc.stdout + proc.stderr,
            )
            self.assertIn("dds_web_wasm_node: OK", proc.stdout)


if __name__ == "__main__":
    unittest.main()
