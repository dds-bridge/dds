"""Tests for staging DDS Web artifacts for GitHub Pages."""
from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path

WEB_ROOT = Path(__file__).resolve().parents[1]


def _load_stage_github_pages():
    path = WEB_ROOT / "stage_github_pages.py"
    spec = importlib.util.spec_from_file_location("stage_github_pages", path)
    if spec is None or spec.loader is None:
        raise ImportError(f"Unable to load {path}")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class StageGithubPagesTest(unittest.TestCase):
    def test_stage_copies_site_wasm_coi_and_index(self) -> None:
        stage = _load_stage_github_pages()
        with tempfile.TemporaryDirectory() as src_dir, tempfile.TemporaryDirectory() as dest_dir:
            src = Path(src_dir)
            dest = Path(dest_dir)
            for name in (
                "dds_web.html",
                "dds_web.css",
                "dds_web.js",
                "coi-serviceworker.js",
                "dds_web_wasm.js",
                "dds_web_wasm.wasm",
                "dds_web_wasm_bin.js",
            ):
                (src / name).write_text(f"content:{name}", encoding="utf-8")

            # Act
            out = stage.stage_github_pages(src, dest)

            # Assert
            self.assertEqual(out, dest)
            for name in (
                "dds_web.html",
                "dds_web.css",
                "dds_web.js",
                "coi-serviceworker.js",
                "dds_web_wasm.js",
                "dds_web_wasm.wasm",
                "dds_web_wasm_bin.js",
                "index.html",
            ):
                path = dest / name
                self.assertTrue(path.is_file(), msg=f"missing {name}")
            self.assertEqual(
                (dest / "index.html").read_text(encoding="utf-8"),
                (dest / "dds_web.html").read_text(encoding="utf-8"),
            )
            self.assertEqual(
                (dest / "coi-serviceworker.js").read_text(encoding="utf-8"),
                "content:coi-serviceworker.js",
            )

    def test_stage_fails_when_required_file_missing(self) -> None:
        stage = _load_stage_github_pages()
        with tempfile.TemporaryDirectory() as src_dir, tempfile.TemporaryDirectory() as dest_dir:
            src = Path(src_dir)
            dest = Path(dest_dir)
            (src / "dds_web.html").write_text("<html></html>", encoding="utf-8")
            with self.assertRaises(FileNotFoundError) as ctx:
                stage.stage_github_pages(src, dest)
            self.assertIn("dds_web.css", str(ctx.exception))
            self.assertEqual(list(dest.iterdir()), [])

    def test_deploy_file_list_matches_static_plus_wasm(self) -> None:
        stage = _load_stage_github_pages()
        self.assertIn("coi-serviceworker.js", stage.DEPLOY_FILES)
        self.assertIn("dds_web_wasm_bin.js", stage.DEPLOY_FILES)
        self.assertNotIn("index.html", stage.DEPLOY_FILES)


if __name__ == "__main__":
    unittest.main()
