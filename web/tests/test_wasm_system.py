"""End-to-end system tests for the web MVP WASM build and Node smoke harness."""
from __future__ import annotations

import importlib.util
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

WEB_ROOT = Path(__file__).resolve().parents[1]
TESTS_ROOT = Path(__file__).resolve().parent


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


def _load_module(name: str):
    path = WEB_ROOT / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def _require_node() -> str:
    node = shutil.which("node")
    if not node:
        raise unittest.SkipTest("node not found")
    return node


@unittest.skipUnless(shutil.which("node"), "node not found")
class DdsMvpWasmSystemTest(unittest.TestCase):
    def test_update_pipeline_and_node_smoke(self) -> None:
        node = _require_node()
        js_src = rlocation("web/dds_mvp_wasm.js")
        wasm_src = rlocation("web/dds_mvp_wasm.wasm")

        patch_mvp_wasm = _load_module("patch_mvp_wasm")
        gen_wasm_bin_js = _load_module("gen_wasm_bin_js")
        verify_wasm_js = _load_module("verify_wasm_js")

        with tempfile.TemporaryDirectory() as tmpdir:
            tmp = Path(tmpdir)
            js_path = tmp / "dds_mvp_wasm.js"
            wasm_path = tmp / "dds_mvp_wasm.wasm"
            bin_js_path = tmp / "dds_mvp_wasm_bin.js"

            shutil.copyfile(js_src, js_path)
            shutil.copyfile(wasm_src, wasm_path)
            js_path.chmod(0o644)
            wasm_path.chmod(0o644)

            updated, code = patch_mvp_wasm.patch_text(js_path.read_text(encoding="utf-8"))
            self.assertEqual(code, 0)
            js_path.write_text(updated, encoding="utf-8")

            again, code2 = patch_mvp_wasm.patch_text(js_path.read_text(encoding="utf-8"))
            self.assertEqual(code2, 0)
            self.assertEqual(again, updated)

            bin_js_path.write_text(
                gen_wasm_bin_js.make_bin_js(wasm_path.read_bytes()),
                encoding="utf-8",
            )
            self.assertTrue(verify_wasm_js.wasm_magic_ok(wasm_path.read_bytes()))
            self.assertIn("ddsMvpWasmBytes", bin_js_path.read_text(encoding="utf-8"))

            proc = subprocess.run(
                [
                    node,
                    str(TESTS_ROOT / "dds_mvp_wasm_node.mjs"),
                    str(js_path),
                    str(wasm_path),
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
            self.assertIn("dds_mvp_wasm_node: OK", proc.stdout)


if __name__ == "__main__":
    unittest.main()
