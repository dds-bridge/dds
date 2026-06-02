import base64
import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

WEB_ROOT = Path(__file__).resolve().parents[1]


def _load_module(name: str):
    path = WEB_ROOT / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


patch_mvp_wasm = _load_module("patch_mvp_wasm")
gen_wasm_bin_js = _load_module("gen_wasm_bin_js")
verify_wasm_js = _load_module("verify_wasm_js")

UNPATCHED_LINE = (
    "var isFileURI = (filename) => filename.startsWith('file://');\n"
)


class PatchMvpWasmTest(unittest.TestCase):
    def test_patched_line_checks_type(self) -> None:
        line = patch_mvp_wasm.patched_line("filename")
        self.assertIn("typeof filename", line)
        self.assertIn(".startsWith('file://')", line)

    def test_patch_text_replaces_unpatched(self) -> None:
        updated, code = patch_mvp_wasm.patch_text(UNPATCHED_LINE)
        self.assertEqual(code, 0)
        self.assertNotEqual(updated, UNPATCHED_LINE)
        self.assertRegex(updated, patch_mvp_wasm.PATCHED_IS_FILE_URI)

    def test_patch_text_idempotent(self) -> None:
        once, _ = patch_mvp_wasm.patch_text(UNPATCHED_LINE)
        again, code = patch_mvp_wasm.patch_text(once)
        self.assertEqual(code, 0)
        self.assertEqual(again, once)

    def test_patch_text_missing_declaration(self) -> None:
        _, code = patch_mvp_wasm.patch_text("// no isFileURI here\n")
        self.assertEqual(code, 1)

    def test_cli_writes_file(self) -> None:
        with tempfile.NamedTemporaryFile(mode="w", suffix=".js", delete=False) as tmp:
            tmp.write(UNPATCHED_LINE)
            path = Path(tmp.name)
        self.addCleanup(path.unlink, missing_ok=True)
        proc = subprocess.run(
            [sys.executable, str(WEB_ROOT / "patch_mvp_wasm.py"), str(path)],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0)
        self.assertRegex(path.read_text(encoding="utf-8"), patch_mvp_wasm.PATCHED_IS_FILE_URI)


class GenWasmBinJsTest(unittest.TestCase):
    def test_make_bin_js_roundtrip(self) -> None:
        wasm = b"\x00asm\x01\x00\x00\x00" + b"\xab" * 80
        js = gen_wasm_bin_js.make_bin_js(wasm)
        self.assertIn("ddsMvpWasmBytes", js)
        self.assertIn("DDS_MVP_WASM_BASE64", js)
        b64 = base64.b64encode(wasm).decode("ascii")
        self.assertIn(b64[:40], js.replace("\n", "").replace(" ", ""))

    def test_cli_writes_output(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            wasm_path = Path(tmpdir) / "tiny.wasm"
            out_path = Path(tmpdir) / "dds_mvp_wasm_bin.js"
            wasm_path.write_bytes(b"\x00asm\x01\x00\x00\x00")
            proc = subprocess.run(
                [
                    sys.executable,
                    str(WEB_ROOT / "gen_wasm_bin_js.py"),
                    str(wasm_path),
                    str(out_path),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(proc.returncode, 0)
            self.assertTrue(out_path.is_file())
            self.assertIn("DDS_MVP_WASM_BASE64", out_path.read_text(encoding="utf-8"))


class VerifyWasmJsTest(unittest.TestCase):
    def test_wasm_magic_ok(self) -> None:
        self.assertTrue(verify_wasm_js.wasm_magic_ok(b"\x00asm\x01\x00\x00\x00"))
        self.assertFalse(verify_wasm_js.wasm_magic_ok(b"bogus"))
        self.assertFalse(verify_wasm_js.wasm_magic_ok(b"\x00as"))

    def test_cli_rejects_bad_magic(self) -> None:
        with tempfile.NamedTemporaryFile(suffix=".wasm", delete=False) as tmp:
            tmp.write(b"not-wasm")
            path = Path(tmp.name)
        self.addCleanup(path.unlink, missing_ok=True)
        proc = subprocess.run(
            [sys.executable, str(WEB_ROOT / "verify_wasm_js.py"), str(path)],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 1)


if __name__ == "__main__":
    unittest.main()
