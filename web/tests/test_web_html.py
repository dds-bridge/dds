"""Static checks for web/dds_web.html encoding."""
from __future__ import annotations

import re
import unittest
from pathlib import Path

WEB_ROOT = Path(__file__).resolve().parents[1]
HTML_PATH = WEB_ROOT / "dds_web.html"


class DdsMvpHtmlCharsetTest(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
