"""Unit tests for the dtest_wasm Node runner helpers."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest import mock

from run_dtest_wasm import build_node_command, resolve_cwd, rlocation


class RunDtestWasmTest(unittest.TestCase):
    def test_build_node_command_prepends_node_and_js(self) -> None:
        cmd = build_node_command(
            js_path="/runfiles/wasm/dtest.js",
            argv=["-f", "hands/list1.txt", "-s", "solve", "-n", "1"],
        )
        self.assertEqual(
            cmd,
            [
                "node",
                "/runfiles/wasm/dtest.js",
                "-f",
                "hands/list1.txt",
                "-s",
                "solve",
                "-n",
                "1",
            ],
        )

    def test_build_node_command_allows_empty_args(self) -> None:
        cmd = build_node_command(js_path="/runfiles/wasm/dtest.js", argv=[])
        self.assertEqual(cmd, ["node", "/runfiles/wasm/dtest.js"])

    def test_resolve_cwd_prefers_build_working_directory(self) -> None:
        cwd = resolve_cwd(
            {
                "BUILD_WORKING_DIRECTORY": "/Users/me/src/dds",
                "PWD": "/tmp/runfiles",
            }
        )
        self.assertEqual(cwd, "/Users/me/src/dds")

    def test_resolve_cwd_falls_back_to_none(self) -> None:
        self.assertIsNone(resolve_cwd({}))

    def test_rlocation_finds_sibling_data_dep(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            sibling = Path(tmp) / "dtest.js"
            sibling.write_text("// stub\n", encoding="utf-8")
            fake_file = Path(tmp) / "run_dtest_wasm.py"
            fake_file.write_text("#\n", encoding="utf-8")
            with mock.patch("run_dtest_wasm.__file__", str(fake_file)):
                with mock.patch.dict("os.environ", {}, clear=True):
                    found = rlocation("wasm/dtest.js")
            self.assertTrue(found.samefile(sibling))

    def test_rlocation_uses_runfiles_manifest_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            real = root / "actual_dtest.js"
            real.write_text("// stub\n", encoding="utf-8")
            manifest = root / "MANIFEST"
            manifest.write_text(
                f"_main/wasm/dtest.js {real}\n",
                encoding="utf-8",
            )
            # No sibling data dep beside the script (manifest-only platforms).
            fake_script = root / "pkg" / "run_dtest_wasm.py"
            fake_script.parent.mkdir()
            fake_script.write_text("#\n", encoding="utf-8")
            with mock.patch("run_dtest_wasm.__file__", str(fake_script)):
                with mock.patch.dict(
                    "os.environ",
                    {"RUNFILES_MANIFEST_FILE": str(manifest)},
                    clear=True,
                ):
                    found = rlocation("wasm/dtest.js")
            self.assertTrue(found.samefile(real))

    def test_rlocation_manifest_accepts_unprefixed_key(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            real = root / "dtest.js"
            real.write_text("// stub\n", encoding="utf-8")
            manifest = root / "MANIFEST"
            manifest.write_text(f"wasm/dtest.js {real}\n", encoding="utf-8")
            fake_script = root / "pkg" / "run_dtest_wasm.py"
            fake_script.parent.mkdir()
            fake_script.write_text("#\n", encoding="utf-8")
            with mock.patch("run_dtest_wasm.__file__", str(fake_script)):
                with mock.patch.dict(
                    "os.environ",
                    {"RUNFILES_MANIFEST_FILE": str(manifest)},
                    clear=True,
                ):
                    found = rlocation("wasm/dtest.js")
            self.assertTrue(found.samefile(real))


if __name__ == "__main__":
    unittest.main()
