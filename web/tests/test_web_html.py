"""Static checks for web/dds_web.html encoding and GitHub Pages COI wiring."""
from __future__ import annotations

import re
import unittest
from pathlib import Path

WEB_ROOT = Path(__file__).resolve().parents[1]
HTML_PATH = WEB_ROOT / "dds_web.html"
COI_PATH = WEB_ROOT / "coi-serviceworker.js"


class DdsWebHtmlCharsetTest(unittest.TestCase):
    def test_declares_utf8_charset_within_first_1024_bytes(self) -> None:
        # Browsers only honor <meta charset> in the first 1024 bytes. Without it,
        # servers that send Content-Type: text/html (no charset) may decode
        # suit glyphs (♠♥♦♣) as Latin-1/Windows-1252 mojibake.
        raw = HTML_PATH.read_bytes()
        prefix = raw[:1024].lower()
        self.assertRegex(
            prefix,
            rb'<meta\s+charset\s*=\s*["\']?utf-8["\']?\s*/?>',
            msg="dds_web.html must declare UTF-8 early so suit symbols render",
        )

    def test_meta_charset_is_first_in_head(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        head_match = re.search(r"<head\b[^>]*>(.*?)</head>", text, re.I | re.S)
        self.assertIsNotNone(head_match)
        head_inner = head_match.group(1).lstrip()
        self.assertRegex(
            head_inner,
            r'(?is)^<meta\s+charset\s*=\s*["\']?utf-8["\']?\s*/?>',
            msg="<meta charset=utf-8> must be the first tag in <head>",
        )

    def test_html_contains_utf8_suit_glyphs(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        for glyph in ("♠", "♥", "♦", "♣"):
            self.assertIn(glyph, text)

    def test_import_deal_button_and_file_input(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertIn('onclick="chooseDealFile()"', text)
        self.assertRegex(
            text,
            r'<input[^>]*id="import-deal-file"[^>]*type="file"',
        )
        self.assertIn("handleDealFileSelected(this)", text)
        self.assertIn(".pbn,.lin,.dlm,.txt", text)

    def test_sample_deals_dropdown_replaces_three_test_deal_buttons(self) -> None:
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertNotIn("fillFormWithGrandSlamTestData()", text)
        self.assertNotIn("fillFormWithEveryoneMakes3nTestData()", text)
        self.assertNotIn("fillFormWithPartScoreTestData()", text)
        self.assertNotIn("NS make 7 test deal", text)
        self.assertNotIn("Everyone makes 3N test deal", text)
        self.assertNotIn("Part-score test deal", text)
        self.assertRegex(
            text,
            r'<select[^>]*id="sample-deals"[^>]*>',
        )
        self.assertIn("handleSampleDealSelected(this)", text)
        self.assertIn("Sample deals…", text)
        self.assertIn("NS make 7", text)
        self.assertIn("Everyone makes 3N", text)
        self.assertIn("Part-score", text)


class DdsWebHtmlCoiTest(unittest.TestCase):
    def test_loads_coi_serviceworker_in_head_before_app_scripts(self) -> None:
        # GitHub Pages cannot set COOP/COEP; coi-serviceworker.js injects them.
        # Load it in <head> before the large WASM scripts so the first-visit
        # reload does not waste a full module download.
        text = HTML_PATH.read_text(encoding="utf-8")
        head_match = re.search(r"<head\b[^>]*>(.*?)</head>", text, re.I | re.S)
        self.assertIsNotNone(head_match)
        self.assertRegex(
            head_match.group(1),
            r'<script\s+src="coi-serviceworker\.js"\s*>\s*</script>',
        )
        coi_at = text.index('src="coi-serviceworker.js"')
        wasm_at = text.index('src="dds_web_wasm.js"')
        import_at = text.index('src="dds_web_deal_import.js"')
        app_at = text.index('src="dds_web.js"')
        self.assertLess(coi_at, wasm_at)
        self.assertLess(coi_at, app_at)
        self.assertLess(import_at, app_at)

    def test_loads_deal_import_script_before_dds_web_js(self) -> None:
        # File-format parsers live in dds_web_deal_import.js so dds_web.js stays
        # focused on UI and solver glue (#382).
        text = HTML_PATH.read_text(encoding="utf-8")
        self.assertRegex(
            text,
            r'<script\s+src="dds_web_deal_import\.js"\s*>\s*</script>',
        )
        import_at = text.index('src="dds_web_deal_import.js"')
        wasm_at = text.index('src="dds_web_wasm.js"')
        app_at = text.index('src="dds_web.js"')
        self.assertLess(wasm_at, import_at)
        self.assertLess(import_at, app_at)

    def test_disables_coep_credentialless_before_coi_serviceworker(self) -> None:
        # coi-serviceworker defaults to COEP: credentialless. Safari / iOS WebKit
        # (including Firefox on iPhone) do not honor that value for isolation, so
        # SharedArrayBuffer stays unavailable. DDS Web is same-origin only, so
        # require-corp is correct — configure window.coi before the worker script.
        text = HTML_PATH.read_text(encoding="utf-8")
        head_match = re.search(r"<head\b[^>]*>(.*?)</head>", text, re.I | re.S)
        self.assertIsNotNone(head_match)
        head = head_match.group(1)
        config_at = head.find("coepCredentialless")
        worker_at = head.find('src="coi-serviceworker.js"')
        self.assertNotEqual(config_at, -1, msg="must set window.coi.coepCredentialless")
        self.assertNotEqual(worker_at, -1)
        self.assertLess(config_at, worker_at)
        self.assertRegex(
            head,
            r"coepCredentialless\s*:\s*\(\)\s*=>\s*false",
        )

    def test_coi_serviceworker_sets_cross_origin_isolation_headers(self) -> None:
        self.assertTrue(COI_PATH.is_file(), msg="vendored coi-serviceworker.js missing")
        text = COI_PATH.read_text(encoding="utf-8")
        self.assertIn("Cross-Origin-Opener-Policy", text)
        self.assertIn("same-origin", text)
        self.assertIn("Cross-Origin-Embedder-Policy", text)
        self.assertIn("require-corp", text)


if __name__ == "__main__":
    unittest.main()
